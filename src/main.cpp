#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <sstream>

std::string get_current_dir() {
	return std::filesystem::current_path().string();
}

void change_directory(const std::string& target_path) {
	try {
		std::filesystem::current_path(target_path);
	} catch(const std::filesystem::filesystem_error& e) {
		std::cerr << "cd: " << target_path << ": " << e.code().message() << '\n';
	}
}

void notFound(const std::string& s) {
	std::cout << s + ": command not found\n";
}

bool file_exists(const std::string& filename) {
	return std::filesystem::exists(filename);
}

bool is_executable(const std::string& path) {
	return (access(path.c_str(), X_OK) == 0);
}

std::vector<std::string> tokenize(const std::string& input) {
	std::vector<std::string> tokens;
	std::istringstream iss(input);
	std::string temp;

	iss >> temp;
	tokens.push_back(temp);
	getline(iss >> std::ws, temp);
	tokens.push_back(temp);

	bool quoted = false;
        std::string message = tokens[1];
	tokens.pop_back();
	std::string current_string = "";

	for(int i = 0; i < (int)message.size(); i++) {
		if(message[i] == '\'') {
			quoted = !quoted;
			continue;
		}
		if(!quoted && message[i] == ' ') {
			if(!current_string.empty())
				tokens.push_back(current_string);
			current_string = "";
			continue;
		}
		current_string += message[i];
	}
	tokens.push_back(current_string);

	return tokens;
}

std::string find_path(const char* path, const std::string& program_name){
	if(!path) return "";
	std::string name(path);
	std::stringstream ss(name);
	std::string dir;

	while(std::getline(ss, dir, ':')) {
		std::string full_path = dir + "/" + program_name;
		if(file_exists(full_path) && is_executable(full_path)) {
			return full_path;
		}
	}
	return "";
}

void execute_program(const std::string& path, const std::vector<std::string>& tokens) {
	
	std::vector<char*> args;
	for(const std::string& s : tokens) {
		args.push_back(const_cast<char*>(s.c_str()));
	}
	args.push_back(nullptr);
	
	pid_t pid = fork();

	if(pid == 0) {
		execv(path.c_str(), args.data());
		perror("execv");
		exit(1);
	}
	else if(pid > 0) {
		int status;
		waitpid(pid, &status, 0);
	}
	else {
		perror("fork");
	}
}

int main() {
  	// Flush after every std::cout / std:cerr
  	std::cout << std::unitbuf;
  	std::cerr << std::unitbuf;

	std::string s;
	std::vector<std::string> commands = {"exit", "type", "echo", "pwd", "cd"};
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
		std::vector<std::string> tokens = tokenize(s);

		if(tokens[0] == "exit") return 0;
		else if(tokens[0] == "echo") {
			for(int i = 1; i < (int)tokens.size(); i++) {
				std::cout << tokens[i];
				if(i == (int)tokens.size()-1) std::cout << '\n';
				else std::cout << ' ';
			}
		}
		else if(tokens[0] == "type") {
			bool is_builtin = builtin_type(tokens[1]);
			if(is_builtin) {
				std::cout << tokens[1] + " is a shell builtin\n";
			}
			else {
				if(rawPath != nullptr) {
					std::string full_path = find_path(rawPath, tokens[1]);
					if(full_path == "") {
						std::cout << tokens[1] + ": not found\n";
					}
					else {
						std::cout << tokens[1] + " is " + full_path + "\n"; 
					}
				}
			}
		}
		else if(tokens[0] == "pwd") {
			std::cout << get_current_dir() << '\n';
		}
		else if(tokens[0] == "cd") {
			if(tokens[1] == "~") {
				const char* home = std::getenv("HOME");
				if(home) tokens[1] = home;
			}
			change_directory(tokens[1]);
		}
		else {
			std::string full_path = find_path(rawPath, tokens[0]);
			if(!full_path.empty()) {
				execute_program(full_path, tokens);
			}
			else {
				std::cout << tokens[0] + ": command not found\n";
			}
		}
	}
	return 0;
}
