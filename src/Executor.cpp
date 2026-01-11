#include "shell/Executor.h"
#include "shell/Utils.h"
#include <iostream>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

namespace shell {
	//Public Manager, Handles pipes and waits for children
	int Executor::execute(const Pipeline& pipeline, const std::vector<std::string>& commands_history) {
		if(pipeline.commands.empty()) {
			return 0;
		}
	
		int input_fd = STDIN_FILENO;
		int pipe_fds[2];
		std::vector<pid_t> pids;

		for(size_t i = 0; i < pipeline.commands.size(); i++) {
			const auto& cmd = pipeline.commands[i];
			int output_fd = STDOUT_FILENO;
			bool is_last = (i == pipeline.commands.size()-1);

			if(!is_last) {
				if(pipe(pipe_fds) == -1) {
					perror("pipe");
					return 1;
				}
				output_fd = pipe_fds[1];
			}

			pid_t pid = executeCommand(cmd, input_fd, output_fd, commands_history);
			if(pid > 0) {
				pids.push_back(pid);
			}
			if(input_fd != STDIN_FILENO) {
				close(input_fd);
			}

			if(!is_last) {
				close(output_fd);
				input_fd = pipe_fds[0];
			}
		}
		
		int last_exit_status = 0;
		for(pid_t pid : pids) {
			int status;
			waitpid(pid, &status, 0);
			if(WIFEXITED(status)) {
				last_exit_status = WEXITSTATUS(status);
			}
		}
		
		return last_exit_status;
	}

	int Executor::executeCommand(const Command& cmd, int input_fd, int output_fd, const std::vector<std::string>& commands_history) {
		//spawn child process
		pid_t pid = fork();

		if(pid == -1) {
			perror("fork");
			return -1;
		}

		if(pid == 0) {
			//child process
			
			//rewire the input
			if(input_fd != STDIN_FILENO) {
				dup2(input_fd, STDIN_FILENO);
				close(input_fd);
			}

			//rewire the output
			if(output_fd != STDOUT_FILENO) {
				dup2(output_fd, STDOUT_FILENO);
				close(output_fd);
			}

			//handle redirections
			if(!applyRedirections(cmd.redirect, nullptr)) {
				_exit(1);
			}

			if(runPipeCompatibleBuiltin(cmd, commands_history)) {
				_exit(0);
			}

			//prepare arguments for execvp
			std::vector<char*> args;
			args.push_back(const_cast<char*>(cmd.program.c_str()));
			for(const auto& arg : cmd.args) {
				args.push_back(const_cast<char*>(arg.c_str()));
			}
			args.push_back(nullptr);

			execvp(cmd.program.c_str(), args.data());

			std::cerr << cmd.program << ": command not found\n";
			_exit(127); //exit code for command not found
		}

		//Parent process
		return pid;
	}
}
