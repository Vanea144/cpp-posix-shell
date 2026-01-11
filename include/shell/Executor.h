#pragma once
#include "Command.h"
#include <string>
#include <iostream>

namespace shell {

	class Executor {
		public:
			//API - takes the full pipeline
                	//returns exit code of last command
			int execute(const Pipeline& pipeline, 
			const std::vector<std::string>& commands_history);
		private:
			//helper function since I treat every line as a pipeline
			//input_fd -> command -> output_fd
			//returns the PID of the spawned process
			int executeCommand(const Command& command, 
					int input_fd, int output_fd, 
			const std::vector<std::string>& commands_history);
	};
}
