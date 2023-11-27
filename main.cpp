#include <fstream>
#include <iostream>
#include <random>
#include <regex>
#include <set>
#include <unordered_map>
#include <vector>

using Messages = std::unordered_map<std::string, std::vector<std::string>>;

std::random_device rd;
std::mt19937 gen(rd());

std::string read_file(std::string const &filename) {
  std::ifstream stream(filename);

  // move the pointer to the end of the file
  stream.seekg(0, std::ios_base::end);
  int len = stream.tellg();
  
  std::cout << "Length -> " << len << std::endl;
  
  std::string data(len, ' ');
  // bring the pointer to the beggining
  stream.seekg(0);
  stream.read(&data[0], len);

  return data;
}

Messages filter_data(std::string const &data, std::string const &user_1, std::string const &user_2, bool filter_attachments = true, bool filter_embeds = true) {
  std::string regex_str = R"(\[([0-9]+\/?)+ [0-9]+:[0-9]+\] ()" + user_1 + "|" + user_2 + ")"; 
  std::regex new_msg_header_regex(regex_str);
  auto it_begin = std::sregex_iterator(std::begin(data), std::end(data), new_msg_header_regex);
  auto it_end = std::sregex_iterator();

  std::cout << "msgs: " << std::distance(it_begin, it_end) << std::endl;
  
  Messages messages;
  while (it_begin != it_end) {
    auto match = *it_begin;
    auto next_it = std::next(it_begin);

    int msg_start = match.position() + match.length();
    int msg_end = (next_it == it_end ? data.size() : next_it->position());
    
    // pattern is: `[date time] user`, so we find ] and then increment by 2 to get first character of user
    int pos = data.find("]", match.position()) + 2;
    std::string user = data.substr(pos, match.length() - (pos - match.position()));

    // TODO: why +2 on msg_start gets rid of '\n'?
    std::string msg = data.substr(msg_start + 2, msg_end - (msg_start + 2));

    bool skip = false;
    if (filter_attachments && msg.find("{Attachments}") != std::string::npos)
      skip = true;
    if (filter_embeds && msg.find("{Embed}") != std::string::npos)
      skip = true;
    
    if (!skip)
      messages[user].push_back(msg);
    it_begin++;
  }

  return messages;
} 

template<int n>
struct Ngram {
  struct PrefixGram {
    PrefixGram() {}
    PrefixGram(int character) {
      for (int i = 0; i < n - 1; i++)
        gram.push_back(character);
    }

    bool operator<(PrefixGram const &other) const {
      return this->gram < other.gram;
    }
    
    void add(int character) {
      gram.push_back(character);
      if (gram.size() == n)
        gram.pop_front();
    }

    std::deque<int> gram;
  };


  Ngram() {}
  Ngram(Messages &messages, std::string const &user) : user(user) {
    std::set<char> vocab;
    for (std::string const &msg : messages[user])
      for (char c : msg)
        vocab.insert(c);
    std::cout << "Vocab size -> " << vocab.size() << std::endl;

    // mappings char -> int
    int index = 0;
    for (char c : vocab)
      char_to_int[c] = index++;

    // mappings int -> char
    for (auto [c, i] : char_to_int)
      int_to_char[i] = c;

    for (std::string const &msg : messages[user]) {
      // starting character <S> n - 1 times
      PrefixGram previous{(int)vocab.size()}; 
      for (char c : msg) {
        if (counts[previous].empty())
          counts[previous].resize(vocab.size() + 2);
        counts[previous][char_to_int[c]]++;
        previous.add(char_to_int[c]);
      }
      // ending character <E>
      if (counts[previous].empty())
        counts[previous].resize(vocab.size() + 2);
      counts[previous][vocab.size() + 1]++;
    }
  }

  int next_character(PrefixGram previous_character_index) {
    std::discrete_distribution<> d(std::begin(counts[previous_character_index]), std::end(counts[previous_character_index]));
    return d(gen);
  }

  std::string generate_sentence() {
    std::string sentence;
    PrefixGram previous_character{(int)char_to_int.size()};
    int next_char = 0;
    while ((next_char = next_character(previous_character)) != char_to_int.size() + 1) { 
      sentence += int_to_char[next_char];
      previous_character.add(next_char);
    }
    return sentence; 
  }

  std::map<PrefixGram, std::vector<int>> counts;
  std::unordered_map<char, int> char_to_int;
  std::unordered_map<int, char> int_to_char;
  std::string user;
};

int main(int argc, char *argv[]) {
  std::string user_1{argv[1]};
  std::string user_2{argv[2]};
  auto messages = filter_data(read_file("data.txt"), user_1, user_2);

  std::cout << "msgs by " << user_1 << ": " << messages[user_1].size() << std::endl;
  std::cout << "msgs by " << user_2 << ": " << messages[user_2].size() << std::endl;

  Ngram<2> ngram1(messages, user_1);
  Ngram<2> ngram2(messages, user_2);
  for (int i = 0; i < 50; i++) {
    std::cout << i << ". " << user_1 << " says: " << std::endl;
    std::cout << ngram1.generate_sentence() << std::endl;
    std::cout << i << ". " << user_2 << " says: " << std::endl;
    std::cout << ngram2.generate_sentence() << std::endl;
  }
}
