#pragma once
#include <string>
#include <vector>

namespace shell {

	// - Overwrite vs append
	enum class RedirectionType {
		Output, // > or 2> (trunc)
		Append, // >> or 2>> (append)
	};

	// - Represents ONE redirection directive
	struct Redirection {
		RedirectionType type;
		int fd;			// target file descriptor
		std::string filename;	// where is it going
	};

	// - Command Structure
	// Represents a single executable command 
	struct Command {
		std::string program;	
		std::vector<std::string> args;	
		std::vector<Redirection> redirect;
	};

	// - Pipeline Structure
	// Represents a chain of commands
	struct Pipeline {
		std::vector<Command> commands;
		bool isBackground = false;
	};

} 
