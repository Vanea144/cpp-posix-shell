#include "shell/Shell.h"
#include "shell/Utils.h"
#include <iostream>
#include <unistd.h>
#include <filesystem>
#include <algorithm>
#include <termios.h>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cctype>

namespace shell {

	//Constructor
	Shell::Shell() {
		char* histfile_env = std::getenv("HISTFILE");
		if(histfile_env) {
			std::ifstream hist(histfile_env);
			if(hist.is_open()) {
				std::string s;
				while(std::getline(hist, s)) {
					commands_history.push_back(s);
				}
			}
			history_file_sync_index = commands_history.size();
		}
		history_index = commands_history.size();
	}

	//Destructor
	Shell::~Shell() {
		disableRawMode();
	}

	void Shell::enableRawMode() {
		tcgetattr(STDIN_FILENO, &orig_termios);
		
		static struct termios* saved_termios = nullptr;
		saved_termios = &orig_termios;

		atexit([] {
				if(saved_termios) {
					tcsetattr(STDIN_FILENO, TCSAFLUSH, saved_termios);
				}
		});
		
		struct termios raw = orig_termios;
		raw.c_lflag &= ~(ECHO | ICANON);
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
	}

	void Shell::disableRawMode() {
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
	}

	void Shell::run() {
		enableRawMode();
		std::cout << std::unitbuf;

		printPrompt();

		char c;
		while(read(STDIN_FILENO, &c, 1) == 1) {
			if(c == 9) {
				handleTab();
				last_tab = true;
				continue;
			} else {
				last_tab = false;
			}

			if(c == '\n') {
				std::cout << '\n';
				bool executed = false;

				if(!input_buffer.empty()) {
					if(commands_history.empty() || commands_history.back() != input_buffer) {
						commands_history.push_back(input_buffer);
					}
					history_index = commands_history.size();

					Pipeline pipeline = parser.parse(input_buffer);

					if(!pipeline.commands.empty()) {
						if(pipeline.commands.size() == 1) {
							executed = executeBuiltin(pipeline.commands[0]);
						}
						if(!executed) {
							executor.execute(pipeline, commands_history);
						}
					}
				}

				input_buffer.clear();
				printPrompt();
				continue;
			}

			if(c == 127) {
				if(!input_buffer.empty()) {
					input_buffer.pop_back();
					std::cout << "\b \b";
				}
				continue;
			}

			if(c == 27) {
				char seq[2];
				if(read(STDIN_FILENO, &seq[0], 1) == 1 && seq[0] == '[') {
					if(read(STDIN_FILENO, &seq[1], 1) == 1) {
						if(seq[1] == 'A') {
							if(history_index > 0) {
								--history_index;
								std::cout << "\33[2K\r";
								printPrompt();
								input_buffer = commands_history[history_index];
								std::cout << input_buffer;
							}
						} else if(seq[1] == 'B') {
							if(history_index < (int)commands_history.size()) {
								++history_index;
								std::cout << "\33[2K\r";
								printPrompt();
								if(history_index == (int)commands_history.size()) {
									input_buffer.clear();
								} else {
									input_buffer = commands_history[history_index];
									std::cout << input_buffer;
								}
							}
						}
					}
				}
				continue;
			}

			if(c == '\r') continue;
			if(c == 4) break;

			if(isprint(c)) {
				input_buffer += c;
				std::cout << c;
			}
		}
	}

	bool Shell::executeBuiltin(const Command& cmd) {
		std::vector<std::pair<int, int>> saved_fds;

		if(!cmd.redirect.empty()) {
			if(!applyRedirections(cmd.redirect, &saved_fds)) {
				restoreRedirections(saved_fds);
				return true;
			}
		}

		bool executed = false;

		if(cmd.program == "exit") {
			char* hist = std::getenv("HISTFILE");
			if(hist) {
				std::ofstream writeTo(hist, std::ios::app);
				for(size_t i = history_file_sync_index; i < commands_history.size(); i++) {
					writeTo << commands_history[i] << '\n';
				}
			}
			restoreRedirections(saved_fds);
			disableRawMode();
			exit(0);
		}
		else if(cmd.program == "cd") {
			if(!cmd.args.empty()) {
				std::string path = cmd.args[0];
				if(path == "~") {
					const char* home = std::getenv("HOME");
					if(home) path = home;
				}
				try {
					std::filesystem::current_path(path);
				} catch(const std::filesystem::filesystem_error& e) {
					std::cerr << "cd: " << path << ": " << e.code().message() << '\n';
				}
			} else {
				const char* home = std::getenv("HOME");
				if(home) std::filesystem::current_path(home);
			}
			executed = true;
		}
else if(cmd.program == "history") {
	if(cmd.args.empty()) {
		for(size_t i = 0; i < commands_history.size(); i++) {
			std::cout << '\t' << (i + 1) << ' ' << commands_history[i] << '\n';
		}
		executed = true;
	}
	else if(std::isdigit(cmd.args[0][0])) {
		int count = std::stoi(cmd.args[0]);
		count = std::min(count, (int)commands_history.size());
		if(count <= 0) executed = true;

		for(size_t i = commands_history.size() - count; i < commands_history.size() && !executed; i++) {
			std::cout << '\t' << (i + 1) << ' ' << commands_history[i] << '\n';
		}
		executed = true;
	}
	else if(cmd.args[0] == "-r" && cmd.args.size() > 1) {
		std::ifstream readFrom(cmd.args[1]);
		std::string s;
		while(std::getline(readFrom, s)) {
			commands_history.push_back(s);
		}
		history_index = commands_history.size();
		history_file_sync_index = commands_history.size();
		executed = true;
	}
	else if(cmd.args[0] == "-w" && cmd.args.size() > 1) {
		std::ofstream writeTo(cmd.args[1]);
		for(const auto& s : commands_history) {
			writeTo << s << '\n';
		}
		executed = true;
	}
	else if(cmd.args[0] == "-a" && cmd.args.size() > 1) {
		std::ofstream writeTo(cmd.args[1], std::ios::app);
		for(size_t i = history_file_sync_index; i < commands_history.size(); i++) {
			writeTo << commands_history[i] << '\n';
		}
		history_file_sync_index = commands_history.size();
		executed = true;
	}
}

		if(!executed) {
			executed = runPipeCompatibleBuiltin(cmd, commands_history);
		}

		if(!saved_fds.empty()) {
			restoreRedirections(saved_fds);
		}
		return executed;
	}

	void Shell::printPrompt() {
		std::cout << "$ ";
	}

	bool Shell::is_executable(const std::string& path) {
		return access(path.c_str(), X_OK) == 0;
	}

	bool Shell::is_prefix(const std::string& pref, const std::string& word) {
		if(word.size() < pref.size()) return false;
		return std::equal(pref.begin(), pref.end(), word.begin());
	}

	void Shell::handleTab() {
		std::vector<std::string> commands = BUILTIN;
		std::sort(commands.begin(), commands.end());

		std::vector<std::string> matches;
		for(const auto& s : commands) {
			if(s.find(input_buffer) == 0) {
				matches.push_back(s);
			}
		}

		char* raw_path = std::getenv("PATH");
		if(raw_path) {
			std::stringstream ss(raw_path);
			std::string dir;

			while(std::getline(ss, dir, ':')) {
				std::error_code ec;
				for(const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
					if(ec || !entry.is_regular_file(ec)) continue;

					std::string file_path = entry.path().string();
					if(!is_executable(file_path)) continue;

					std::string file_name = entry.path().filename().string();
					if(file_name.find(input_buffer) == 0) {
						matches.push_back(file_name);
					}
				}
			}
		}

		std::sort(matches.begin(), matches.end());
		matches.erase(std::unique(matches.begin(), matches.end()), matches.end());

		if(matches.empty()) {
			std::cout << '\a';
		}
		else if(matches.size() == 1) {
			std::string suffix = matches[0].substr(input_buffer.size());
			std::cout << suffix << " ";
			input_buffer = matches[0] + " ";
		}
		else {
			bool is_first_prefix = true;
			for(size_t i = 1; i < matches.size(); i++) {
				if(!is_prefix(matches[0], matches[i])) {
					is_first_prefix = false;
					break;
				}
			}

			if(is_first_prefix) {
				std::string suffix = matches[0].substr(input_buffer.size());
				std::cout << suffix;
				input_buffer = matches[0];
			}
			else if(last_tab) {
				std::cout << '\n';
				for(const auto& m : matches) {
					std::cout << m << "  ";
				}
				std::cout << "\n$ " << input_buffer;
			}
			else {
				std::cout << '\a';
			}
		}
	}
}

