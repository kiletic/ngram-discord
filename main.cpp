#include <fstream>
#include <iostream>
#include <regex>
#include <unordered_map>
#include <vector>

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

std::unordered_map<std::string, std::vector<std::string>> filter_data(std::string const &data, bool filter_attachments = true, bool filter_embeds = true) {
  std::regex new_msg_header_regex(R"(\[([0-9]+\/?)+ [0-9]+:[0-9]+\] (heon5|kadak9))");
  auto it_begin = std::sregex_iterator(std::begin(data), std::end(data), new_msg_header_regex);
  auto it_end = std::sregex_iterator();

  std::cout << "msgs: " << std::distance(it_begin, it_end) << std::endl;
  
  std::unordered_map<std::string, std::vector<std::string>> messages;
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

int main() {
  auto messages = filter_data(read_file("data.txt"));
  std::cout << "msgs by heon5: " << messages["heon5"].size() << std::endl;
  std::cout << "msgs by kadak9: " << messages["kadak9"].size() << std::endl;
}
