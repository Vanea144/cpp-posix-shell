#include <iostream>
#include <string>

int main() {
  	// Flush after every std::cout / std:cerr
  	std::cout << std::unitbuf;
  	std::cerr << std::unitbuf;

	std::string s;

	std::vector<std::string> commands = {"exit", "type", "echo"};

	while(std::cout << "$ ", std::getline(std::cin, s)) {
		if(s.substr(0, 4) == "exit") return 0;
		std::string message = s.substr(5);
		if(s.substr(0, 4) == "echo") {
			std::cout << message << '\n';
			continue;
		}
		if(s.substr(0, 4) == "type") {
			std::cout << message;
			for(std::string cmnd : commands) {
				if(cmnd == message) {
					std::cout << " is a shell builtin\n";
					break;
				}
				if(cmnd == commands.back()) {
					std::cout << ": not found\n";
				}
			}
			continue;
		}
		std::cout << s + ": command not found\n"; 	
	}
	return 0;
}
