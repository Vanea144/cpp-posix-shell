#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include <sstream>

void notFound(std::string s) {
	std::cout << s + ": command not found\n";
}

bool file_exists(const std::string& filename) {
	return std::filesystem::exists(filename);
}

bool is_executable(const std::string& path) {
	return (access(path.c_str(), X_OK) == 0);
}

int main() {
  	// Flush after every std::cout / std:cerr
  	std::cout << std::unitbuf;
  	std::cerr << std::unitbuf;

	std::string s;
	std::vector<std::string> commands = {"exit", "type", "echo"};
	std::sort(commands.begin(), commands.end());

	char *rawPath = std::getenv("PATH");

	auto builtin_type = [&](std::string cmnd) {
		int l = 0, r = commands.size()-1;
		while(l <= r) {
			int mid = (l+r)/2;
			if(commands[mid] == cmnd) return true;
			else if(commands[mid] > cmnd) {
				r = mid-1;
			}
			else {
				l = mid+1;
			}
		}
		return false;
	};

	while(std::cout << "$ ", std::getline(std::cin, s)) {
		std::string command = "";
		int i;
		for(i = 0; i < (int)s.length(); i++) {
			if(s[i] == ' ') {
				if(command.length()) break;
				continue;
			}
			command += s[i];
		}
		std::string message = (i+1 >= (int)s.length() ? "" : s.substr(i+1));
		if(command == "exit") return 0;
		if(command == "echo") {
			std::cout << message << '\n';
			continue;
		}
		if(command == "type") {
			bool ans = builtin_type(message);
			if(ans) {
				std::cout << message + " is a shell builtin\n";
			}
			else {
				if(rawPath != nullptr) {
					std::string pathString(rawPath);
					
					std::stringstream ss(pathString);
					std::string directory;

					bool fnd = false;
					while(std::getline(ss, directory, ':')) {
						std::string fullPath = directory + '/' + message;
						if(file_exists(fullPath) && 
						  is_executable(fullPath)) {
							std::cout << message + " is " + fullPath << '\n';
							fnd = true;
							break;
						}
					}
					if(!fnd) std::cout << message << ": not found\n";
				}	
			}
			continue;
		}
		notFound(s);	
	}
	return 0;
}
