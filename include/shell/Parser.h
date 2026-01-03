#pragma once
#include <string> 
#include <vector>
#include "Command.h"

namespace shell {
	
	// API: Takes a raw command line and returns a structured Pipeline
	class Parser {
		public:
			Pipeline parse(const std::string& input);	
	
		private:
			// Helper 1: Break string into tokens
			std::vector<std::string> tokenize(const std::string& input);
			// Helper 2: Converts tokens into a 
			// single Command object
			Command parseSingleCommands(const std::vector<std::string>& tokens);
	};

} 
