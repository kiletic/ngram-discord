#include <fstream>
#include <iostream>

std::string read_file(std::string filename) {
  std::ifstream stream(filename);

  // move pointer to the end of the file
  stream.seekg(0, std::ios_base::end);
  int len = stream.tellg();
  
  std::cout << "Length -> " << len << std::endl;
  
  std::string data(len, ' ');
  // bring the pointer to the beggining
  stream.seekg(0);
  stream.read(&data[0], len);

  return data;
}

int main() {
  std::string data = read_file("data.txt");
  for (int i = 0; i < 125; i++)
    std::cout << data[i];
}
