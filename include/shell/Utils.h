#pragma once
#include "Command.h"
#include <vector>
#include <utility>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <filesystem>

namespace shell {

	inline const std::vector<std::string> BUILTIN = {
		"exit", "echo", "type", "pwd", "cd", "history"
	};

	inline bool is_builtin(const std::string& program) {
		return std::find(BUILTIN.begin(), BUILTIN.end(), program) != BUILTIN.end();
	}

	inline bool is_executable(const std::string& path) {
		return (access(path.c_str(), X_OK) == 0);
	}

	inline bool runPipeCompatibleBuiltin(const Command& cmd, const std::vector<std::string>& commands_history) {
		if(cmd.program == "pwd") {
			std::cout << std::filesystem::current_path().string() << '\n';
			return true;
		}
		else if(cmd.program == "echo") {
			for(size_t i = 0; i < cmd.args.size(); i++) {
				std::cout << cmd.args[i];
				if(i != cmd.args.size()-1) std::cout << ' ';
			}
			std::cout << '\n';
			return true;
		}
		else if(cmd.program == "type") {
			if(cmd.args.empty()) { 
				std::cerr << "type: missing operand\n";
				return true;
			}

			std::string target = cmd.args[0];
			if(is_builtin(target)) {
				std::cout << target << " is a shell builtin\n";
			}
			else {
				char* raw_path = std::getenv("PATH");
				if(raw_path) {
					std::string name(raw_path);
					std::stringstream ss(name);
					std::string dir;
					bool found = false;
					while(std::getline(ss, dir, ':')) {
						std::string full_path = dir + "/" + target;
						if(is_executable(full_path)) {
							std::cout << target << " is " << full_path << "\n";
							found = true;
							break;
						}
					}
					if(!found) std::cout << target << ": not found\n";
				}
			}
			return true;
		}
		else if(cmd.program == "history") {
			if(!cmd.args.empty()) return false;
			for(size_t i = 0; i < commands_history.size(); i++) {
				std::cout << '\t' << (i+1) << ' ';
				std::cout << commands_history[i] << '\n';
			}
			return true;
		}
		return false;
	}

	inline bool applyRedirections(const std::vector<Redirection>& redirects, 
			std::vector<std::pair<int, int>>* saved_fds = nullptr) 
	{
		for (const auto& redir : redirects) {
			int flags = O_WRONLY | O_CREAT;

			if(redir.type == RedirectionType::Append) {
				flags |= O_APPEND;
			}
			else {
				flags |= O_TRUNC;
			}

			int fd = open(redir.filename.c_str(), flags, 0644);
			if(fd < 0) {
				perror("open");
				return false;
			}

			if(saved_fds) {
				bool already_saved = false;
				for(const auto& pair : *saved_fds) {
					if(pair.first == redir.fd) {
						already_saved = true;
						break;
					}
				}
				if(!already_saved) {
					int original = dup(redir.fd);
					if(original != -1) {
						saved_fds->push_back({redir.fd, original});
					}
				}
			}
			if(dup2(fd, redir.fd) == -1) {
				perror("dup2");
				close(fd);
				return false;
			}
			close(fd);
		}
		return true;
	}

	inline void restoreRedirections(const std::vector<std::pair<int, int>>& saved_fds)
	{
		for(const auto& pair : saved_fds) {
			dup2(pair.second, pair.first);
			close(pair.second);
		}
	}
	
}
