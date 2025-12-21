#include <iostream>
#include <string>

int main() {
  	// Flush after every std::cout / std:cerr
  	std::cout << std::unitbuf;
  	std::cerr << std::unitbuf;

	std::string s;

	while(std::cout << "$ ", std::getline(std::cin, s)) {
		if(s.substr(0, 4) == "exit") return 0;
		if(s.substr(0, 4) == "echo") {
			std::string message = s.substr(5);
			std::cout << message << '\n';
			continue;
		}
		std::cout << s + ": command not found\n"; 	
	}
	return 0;
}
