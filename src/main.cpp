/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbatty <mbatty@student.42angouleme.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 12:30:09 by mbatty            #+#    #+#             */
/*   Updated: 2026/04/25 17:26:17 by mbatty           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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

class	Chrono
{
	public:
		Chrono() {}
		~Chrono() {}

		void	start()
		{
			_start = getTime();
		}
		double	get()
		{
			return (getTime() - _start);
		}

		static double getTime()
		{
			double	res;
			struct timespec	current;
			clock_gettime(CLOCK_MONOTONIC, &current);
			res = (current.tv_sec) + (current.tv_nsec) * 1e-9;
			return (res);
		}
	private:
		double		_start = 0;
};

const char	**c_str_array(const std::vector<std::string> &vec)
{
	const char	**res = new const char *[vec.size() + 1];

	int	i = 0;
	for (const std::string &s : vec)
		res[i++] = s.c_str();
	res[i] = NULL;
	return (res);
}

/*
	Definition of a process from the config file
*/
struct	ProcessDefinition
{
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
	int		stop_signal = 0;

	// How long to wait after a graceful stop before killing the program
	double	max_stop_time = 0;

	bool		redirect_stdout = false;
	bool		redirect_stderr = false;
	std::string	stdout_redirect = "/dev/null";
	std::string	stderr_redirect = "/dev/null";

	// An umask to set before launching the program
	int	umask = 0;
};

class	Process
{
	public:
		Process(ProcessDefinition *def)
		{
			_def = def;
		}
		~Process() {}

		int	start()
		{
			std::cout << "Starting process \"" << _def->name << "\"" << std::endl;

			_pid = fork();
			if (_pid == -1)
				return (-1);
			if (_pid != 0)
				return (_pid);

			execve(_def->cmd.c_str(), (char *const *)c_str_array(_def->av), (char *const *)c_str_array(_def->env));
			exit(EXIT_FAILURE);
		}

		pid_t	pid()
		{
			return (_pid);
		}
	private:
		ProcessDefinition	*_def = NULL;
		pid_t					_pid = 0;
};

class	SystemDWannaBe
{
	public:
		SystemDWannaBe() {}
		~SystemDWannaBe() {}

		void	run(const std::string &config_file)
		{
			_process_definitions.insert({"sleep_1_sec", {
				.name = "sleep_1_sec",
				.cmd = "./test.sh",
				.av = {"./test.sh", "1"},
				.env = {},

				.processes_count = 1,
				.start_at_launch = true,
				.restart_mode = ProcessDefinition::RestartMode::ALWAYS,
				.expected_exit_code = {0},

				.run_time_validity = 0,
				.restart_tries = 0,
				.stop_signal = SIGINT,
				.max_stop_time = 1,
			}});

			_loop();
		}
	private:
		void	_loop()
		{
			_start_launch_processes();

			_running = true;
			while (_running)
			{
				_update_processes();
			}
		}
		void	_start_launch_processes()
		{
			for (auto &[name, proc_def] : _process_definitions)
			{
				if (proc_def.start_at_launch)
				{
					std::vector<Process>	&procs = _running_processes[name];

					for (uint64_t i = 0; i < proc_def.processes_count; i++)
					{
						procs.push_back({&proc_def});

						Process	&p = procs.back();
						p.start();
					}
				}
			}
		}
		void	_update_processes()
		{
			for (auto &[name, proc_def] : _process_definitions)
			{
				std::vector<Process>	&procs = _running_processes[name];

				for (auto it = procs.begin(); it != procs.end();)
				{
					Process	&p = *it;

					int	status = 0;
					int	wpid = waitpid(p.pid(), &status, WNOHANG);
					int	ret = WEXITSTATUS(status);
					if (wpid == 0)
						continue ;
					if (WIFEXITED(status))
					{
						std::cout << "Process \"" << proc_def.name << "\" (PID: " << p.pid() << ") exited (" << ret << ")" << std::endl;
						if (proc_def.restart_mode == ProcessDefinition::RestartMode::NEVER)
						{
							it = procs.erase(it);
							continue ;
						}
						else if (proc_def.restart_mode == ProcessDefinition::RestartMode::ALWAYS
							|| (proc_def.restart_mode == ProcessDefinition::RestartMode::ON_ERROR
								&& std::find(proc_def.expected_exit_code.begin(), proc_def.expected_exit_code.end(), ret) == proc_def.expected_exit_code.end()))
						{
							p.start();
						}
					}

					it++;
				}
			}
		}
		bool					_running;
		std::string				_config_file;

		std::unordered_map<std::string, ProcessDefinition>				_process_definitions;
		std::unordered_map<std::string, std::vector<Process>>			_running_processes;
};

int	main(int ac, char **av, char **envp)
{
	if (ac != 2)
	{
		std::cerr << "\nUsage:\n  ./taskmaster <config_file>\n" << std::endl;
		return (1);
	}

	try
	{
		SystemDWannaBe	taskmaster;

		taskmaster.run(av[1]);
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return (1);
	}
	return (0);
}
