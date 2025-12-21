#include <iostream>
#include <string>

int main() {
  	// Flush after every std::cout / std:cerr
  	std::cout << std::unitbuf;
  	std::cerr << std::unitbuf;

	std::string s;
	std::cout << "$ ";

	while(std::getline(std::cin, s)) {
		std::cout << s + ": command not found\n"; 	
		std::cout << "$ ";
	}
}
