#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <unordered_map>
#include <unistd.h>
#include <cstdint>
#include <sys/wait.h>
#include <ctime>
#include <algorithm>
#include <iostream>
#include <string>

class	Process;

/*
	Definition of a process from the config file
*/
struct	ProcessDefinition
{
	ProcessDefinition() {};
	~ProcessDefinition() {};

	std::string	name;

	std::string					cmd;
	std::vector<std::string>	av;
	std::vector<std::string>	env;

	// A working directory to set before launching the program
	std::string	work_dir = ".";

	// How many processes to run for this process
	uint64_t					processes_count = 0;

	bool						start_at_launch = false;

	enum class	RestartMode
	{
		ALWAYS,
		NEVER,
		ON_ERROR, // Restarts if exit code is not in accepted list
	};
	RestartMode				restart_mode;
	std::vector<uint8_t>	expected_exit_code;

	// How long the program should be running after it’s started for it to be considered "successfully started"
	double	run_time_validity = 0;
	// How many times a restart should be attempted before aborting
	int		restart_tries = 0;

	// Which signal should be used to stop (i.e. exit gracefully) the program
	int		stop_signal = SIGINT;

	// How long to wait after a graceful stop before killing the program
	double	max_stop_time = 0;

	bool		redirect_stdout = false;
	bool		redirect_stderr = false;
	std::string	stdout_redirect = "/dev/null";
	std::string	stderr_redirect = "/dev/null";

	// An umask to set before launching the program
	int	umask = 0;

	void	register_process(Process *p)
	{
		_processes.push_back(p);
	}

	private:
		std::vector<Process*>	_processes;
};
