#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <sstream>
#include <fcntl.h>
#include <termios.h>

struct termios orig_termios;

void disableRawMode() {
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
	tcgetattr(STDIN_FILENO, &orig_termios);
	atexit(disableRawMode);

	struct termios raw = orig_termios;
	raw.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void handleTab(std::string& current_input, const std::vector<std::string>& commands) {
	std::vector<std::string> matches;
	for(const std::string& s : commands) {
		if(s.find(current_input) == 0) {
			matches.push_back(s);
		}
	}
	if(matches.size() == 1) {
		std::string suffix = matches[0].substr(current_input.length());
		std::cout << suffix << ' ';
		current_input = matches[0] + " ";
	}
	else if(matches.size() > 1) {
		std::cout << "\a";
	}
}

int open_file_redirection(const std::string& filename, int target_fd, bool append = false) {	
	std::cout.flush();
	std::cerr.flush();

	int flags = O_WRONLY | O_CREAT;
	if(append) flags |= O_APPEND;
	else flags |= O_TRUNC;
	int file_fd = open(filename.c_str(), flags, 0644);
	if(file_fd < 0) {
		perror("Open");
		return -1;
	}
	int saved_stdout = dup(target_fd);
	dup2(file_fd, target_fd);
	close(file_fd);
	return saved_stdout;
}

void restore_file_redirection(const int saved_stdout, int target_fd) {
	std::cout.flush();
	std::cerr.flush();
	dup2(saved_stdout, target_fd);
	close(saved_stdout);
}

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
	std::string current_token = "";
	bool in_squotes = false, in_dquotes = false;
	std::vector<char> escaped = {'"', '\\'};

	for(int i = 0; i < (int)input.size(); i++) {
		char c = input[i];
		if(c == '\\' && !in_squotes && !in_dquotes) {
			if(i + 1 < (int)input.size()) {
				++i;
				current_token += input[i];
			}
		}
		else if(c == '\\' && in_dquotes) {
			++i;
			char next_c = input[i];
			for(auto it : escaped) {
				if(next_c == it) {
					current_token += it;
					break;
				}
				if(it == escaped.back()) {
					--i;
					current_token += '\\';
				}
			}
		}
		else if(c == '"' && !in_squotes) {
			in_dquotes = !in_dquotes;
		}
		else if(c == '\'' && !in_dquotes) {
			in_squotes = !in_squotes;
		}
		else if(!in_squotes && !in_dquotes && c == ' ') {
			if(!current_token.empty()) {
				tokens.push_back(current_token);
				current_token.clear();
			}
		}
		else {
			current_token += c;
		} 
	}

	if(!current_token.empty()) {
		tokens.push_back(current_token);
	}
	//
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
	char c;
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

	enableRawMode();
	std::string input_buffer = "";
	std::cout << "$ ";
	while(read(STDIN_FILENO, &c, 1) == 1) {
		if(c == '\n') {
			std::cout << '\n';
			std::vector<std::string> tokens = tokenize(input_buffer);
			input_buffer = "";
			if(tokens.empty()) {
				std::cout << "$ ";
				continue;
			}

			bool redirect = false;
			int saved_stdout, target_fd;
			for(int i = 0; i < (int)tokens.size(); i++) {
				if(tokens[i] == ">" || tokens[i] == "1>" || tokens[i] == "2>" || tokens[i] == ">>" || tokens[i] == "1>>" || tokens[i] == "2>>") {
					std::string filename;
					if(i+1 < (int)tokens.size()) {
						filename = tokens[i+1];
						redirect = true;
						target_fd = (tokens[i][0] == '2' ? STDERR_FILENO : STDOUT_FILENO);
						bool append = tokens[i].find(">>") != std::string::npos;
						saved_stdout = open_file_redirection(filename, target_fd, append);	
						tokens.erase(tokens.begin()+i, tokens.begin()+i+2);
						--i;
					}
				}
			}
			if(tokens[0] == "exit") return 0;
			else if(tokens[0] == "echo") {
				for(int i = 1; i < (int)tokens.size(); i++) {
					std::cout << tokens[i];
					if(i != (int)tokens.size()-1) std::cout << ' ';
				}
				std::cout << '\n';
			}
			else if(tokens[0] == "type") {
				if(tokens.size() < 2) {
					std::cerr << "type: missing argument\n";
					continue;
				}
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
				if(tokens.size() < 2) {
					change_directory(std::getenv("HOME"));
					continue;
				}
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
			if(redirect) restore_file_redirection(saved_stdout, target_fd);
			std::cout << "$ ";
		}
		else if(c == 9) {
			handleTab(input_buffer, commands);
		}
		else if(c == 4) {
			break;
		}
		else if(c == 127) {
			if(!input_buffer.empty()) {
				input_buffer.pop_back();
				std::cout << "\b \b";
			}
		}
		else {
			input_buffer += c;
			std::cout << c;
		}
	}
	return 0;
}
