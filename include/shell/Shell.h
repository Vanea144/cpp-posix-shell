#pragma once

#include "Parser.h"
#include "Executor.h"
#include <string> 
#include <vector>
#include <termios.h>

namespace shell {
	class Shell {

	public:
		Shell(); // Constructor to initialize variable
		~Shell(); // Desctrucotr to disable raw mode on exit

		//The main loop, prompts, read input, runs commands
		void run();

	private:
		Parser parser;
		Executor executor;
		std::vector<std::string> commands_history;

		//State for history & input;
		std::string input_buffer;
		int history_index;
		int history_file_sync_index = 0;
		bool last_tab = false;

		//Raw mode internals
		struct termios orig_termios;
		void enableRawMode();
		void disableRawMode();

		//Helper functions:
		//Display promptt
		void printPrompt();

		//Execute builtin commands
		bool executeBuiltin(const Command& cmd);
	
		//Handle autocompletion
		void handleTab();

		//Utilities for Tab Completion
		bool is_executable(const std::string& path);
		bool is_prefix(const std::string& pref, const std::string& word);
	};
}
