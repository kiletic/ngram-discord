#include <atomic>
#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <regex>
#include <set>
#include <unordered_map>
#include <vector>
#include <thread>
#include <cassert>
#include <chrono>

using Messages = std::unordered_map<std::string, std::vector<std::string>>;

std::random_device rd;
std::mt19937 gen(rd());

std::string read_file(std::string const &filename) {
  std::ifstream stream(filename);

  // move the pointer to the end of the file
  stream.seekg(0, std::ios_base::end);
  int len = stream.tellg();
  
  std::cout << "Total length is " << len << " characters" << std::endl;
  
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

  int const total_msg_count = std::distance(it_begin, it_end);
  std::cout << "Message count is " << total_msg_count << std::endl;
  
  Messages messages;
  messages[user_1].reserve(total_msg_count);
  messages[user_2].reserve(total_msg_count);
  while (it_begin != it_end) {
    auto match = *it_begin;
    auto next_it = ++it_begin;

    int msg_start = match.position() + match.length();
    int msg_end = (next_it == it_end ? data.size() : next_it->position());

    // pattern is: `[date time] user`, the [date time] part is always 17 characters long so at offset 19 user name starts 
    int user_start_pos = match.position() + 19;
    assert(data[user_start_pos - 1] == ' ' && data[user_start_pos - 2] == ']');
    std::string user = data.substr(user_start_pos, match.length() - (user_start_pos - match.position()));

    // TODO: why +2 on msg_start gets rid of '\n'?
    std::string msg = data.substr(msg_start + 2, msg_end - (msg_start + 2));

    bool skip = false;
    if (filter_embeds && msg.find("{Embed}") != std::string::npos)
      skip = true;
    if (!skip && filter_attachments && msg.find("{Attachments}") != std::string::npos)
      skip = true;
    if (!skip)
      messages[user].push_back(std::move(msg));
  }

  return messages;
} 

template<int n>
class Ngram {
public:
  class ContextWindow {
  public:
    ContextWindow() = default;

    ContextWindow(int character) {
      window.assign(n - 1, character);
    }

    bool operator==(ContextWindow const &other) const {
      return this->window == other.window;
    }

    size_t size() const {
      return window.size(); 
    }
    
    void add(int character) {
      window.push_back(character);
      if (window.size() == n)
        window.pop_front();
    }

    std::deque<int> window;
  };

  // https://stackoverflow.com/a/72073933 
  struct ContextWindow_hash {
    std::size_t operator()(ContextWindow const &cw) const {
      std::size_t seed = cw.size(); 
      for (auto x : cw.window) {
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = (x >> 16) ^ x;
        seed ^= x + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      }
      return seed;
    };
  };

  Ngram() = default; 

  Ngram(Messages &messages, std::string const &user) : m_user(user), m_messages(messages) {}

  void init() {
    std::set<char> vocab;
    for (std::string const &msg : m_messages[m_user])
      vocab.insert(std::begin(msg), std::end(msg));

    std::cout << "Vocab size for " << m_user << " is " << vocab.size() << std::endl;

    // mappings char -> int
    for (char c : vocab)
      m_char_to_int[c] = m_char_to_int.size();

    // mappings int -> char
    for (auto [c, i] : m_char_to_int)
      m_int_to_char[i] = c;
    
    int const START_CHAR = vocab.size();
    int const END_CHAR = vocab.size() + 1;
    // we add 2 fake characters - start, end
    int const VOCAB_SIZE = vocab.size() + 2;

    for (auto const &msg : m_messages[m_user]) {
      ContextWindow context_window{START_CHAR}; 

      for (char c : msg) {
        if (m_counts[context_window].empty())
          m_counts[context_window].resize(VOCAB_SIZE);

        int val = m_char_to_int[c];
        m_counts[context_window][val]++;
        context_window.add(val);
      }

      if (m_counts[context_window].empty())
        m_counts[context_window].resize(VOCAB_SIZE);
      m_counts[context_window][END_CHAR]++;
    }
  }

  int next_character(ContextWindow const &context_window) {
    return std::discrete_distribution{std::begin(m_counts[context_window]), std::end(m_counts[context_window])}(gen);
  }

  std::string generate_sentence() {
    std::string sentence;
    int const START_CHAR = m_char_to_int.size();
    ContextWindow context_window{START_CHAR};
    int next_char = 0;
    while ((next_char = next_character(context_window)) != m_char_to_int.size() + 1) { 
      sentence += m_int_to_char[next_char];
      context_window.add(next_char);
    }
    return sentence; 
  }

private:
  Messages m_messages;
  std::unordered_map<ContextWindow, std::vector<int>, ContextWindow_hash> m_counts;
  std::unordered_map<char, int> m_char_to_int;
  std::unordered_map<int, char> m_int_to_char;
  std::string m_user;
};

int main(int argc, char *argv[]) {
  std::string user_1{argv[1]};
  std::string user_2{argv[2]};

  std::chrono::steady_clock::steady_clock::time_point a = std::chrono::steady_clock::now();
  auto messages = filter_data(read_file("data.txt"), user_1, user_2);
  std::chrono::steady_clock::steady_clock::time_point b = std::chrono::steady_clock::now();
  std::cout << "Time taken for filter_data is " << std::chrono::duration_cast<std::chrono::milliseconds>(b - a).count() << " ms." << std::endl;

  std::cout << "Message count by " << user_1 << " is " << messages[user_1].size() << std::endl;
  std::cout << "Message count by " << user_2 << " is " << messages[user_2].size() << std::endl;

  int const n = 8;
  Ngram<n> user_1_ngram(messages, user_1);
  Ngram<n> user_2_ngram(messages, user_2);
  
  std::jthread t1([&]{ user_1_ngram.init(); });
  std::jthread t2([&]{ user_2_ngram.init(); });

  t1.join();
  t2.join();

  for (int i = 0; i < 15; i++) {
    std::cout << i << ". " << user_1 << " says: " << std::endl;
    std::cout << user_1_ngram.generate_sentence() << std::endl;
    std::cout << i << ". " << user_2 << " says: " << std::endl;
    std::cout << user_2_ngram.generate_sentence() << std::endl;
  }
}
