#include "shell/Parser.h"
#include <iostream>

namespace shell {

	Pipeline Parser::parse(const std::string& input) {
		Pipeline pipeline;

		// Get raw tokens
		std::vector<std::string> raw_tokens = tokenize(input);

		if(raw_tokens.empty()) return pipeline;

		// Split by Pipe '|'
		std::vector<std::string> curr_command;
		for(const auto& token : raw_tokens) {
			if(token == "|") {
				pipeline.commands.push_back(parseSingleCommands(curr_command));
				curr_command.clear();
			}
			else {
				curr_command.push_back(token);
			}
		} 

		// Final command after final pipe
		if(!curr_command.empty()) {
			pipeline.commands.push_back(parseSingleCommands(curr_command));
		}

		return pipeline;
	}

	Command Parser::parseSingleCommands(const std::vector<std::string>& tokens) {
		Command cmd;

		for(size_t i = 0; i < tokens.size(); i++) {
			const std::string& token = tokens[i];
		
			// There can be a redirection
			Redirection redir;
			bool is_redirection = false;

			if(token == ">" || token == "1>") {
				redir.fd = 1;
				redir.type = RedirectionType::Output;
				is_redirection = true;
			}
			else if(token == ">>" || token == "1>>") {
				redir.fd = 1;
				redir.type = RedirectionType::Append;
				is_redirection = true;
			}
			else if(token == "2>") {
				redir.fd = 2;
                                redir.type = RedirectionType::Output;
                                is_redirection = true;
			}
			else if(token == "2>>") {
				redir.fd = 2;
                                redir.type = RedirectionType::Append;
                                is_redirection = true;
			}

			if(is_redirection) {
				//found redirection, check if filename given
				if(i+1 < tokens.size()) {
					redir.filename = tokens[i+1];
					cmd.redirect.push_back(redir);
					++i;
				}
				else {
					// invalid syntax : e.g. ls > 
				}
			}
			else {
				if(cmd.program.empty()) {
					cmd.program = token;
				}
				else {
					cmd.args.push_back(token);
				}
			}
		}
		return cmd;
	}

	std::vector<std::string> Parser::tokenize(const std::string& input) {
		std::vector<std::string> tokens;
		std::string current_string = "";
		bool in_squotes = false;
		bool in_dquotes = false;
		// Special characters in double quotes
		std::vector<char> escaped = {'"', '\\', '`', '$'};
		
		for(size_t i = 0; i < input.size(); i++) {
			char c = input[i];

			if(c == '\\') {
				if(i + 1 < input.size()) {
					char next_c = input[i+1];
					if(in_dquotes) {
						bool is_special = false;
						for(char e : escaped) {
							if(next_c == e) {
								is_special = true;
							}
						} 
						if(!is_special) {
							//not special
							current_string += '\\';
						}
						else {
							//special, skip /
							++i;
							current_string += next_c;
						}
					}
					else if(!in_squotes) {
						//outside quotes
						//backslash escapes
						i++;
						current_string += input[i];
					}
					else {
						//in single quotes
						//literal
						current_string += c;
					}
				}
				else {
					current_string += c;
				}
			}
			else if(c == '"' && !in_squotes) {
				in_dquotes = !in_dquotes;
			}
			else if(c == '\'' && !in_dquotes) {
				in_squotes = !in_squotes;
			}
			else if(c == ' ' && !in_squotes && !in_dquotes) {
				if(!current_string.empty()) {
					tokens.push_back(current_string);
					current_string.clear();
				}
			}
			else {
				current_string += c;
			}
		}
		if(!current_string.empty()) {
			tokens.push_back(current_string);
		}
		return tokens;
	}
}
