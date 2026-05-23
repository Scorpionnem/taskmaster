#pragma once

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <ctime>
#include <iostream>
#include <string>

#include "Chrono.hpp"
#include "TaskConfig.hpp"

inline const char	**c_str_array(const std::vector<std::string> &vec)
{
	const char	**res = new const char *[vec.size() + 1];

	int	i = 0;
	for (const std::string &s : vec)
		res[i++] = s.c_str();
	res[i] = NULL;
	return (res);
}

inline const char	**get_env(const std::map<std::string, std::string> env)
{
	std::vector<std::string> new_env;

	for (auto key : env)
		new_env.push_back(key.first + "=" + key.second);
	return (c_str_array(new_env));
}

class	Process
{
	public:
		enum class	State
		{
			STOPPED,
			STARTING,
			RUNNING,
			STOPPING,
			EXITED,
			// Start failed too quickly, retry pending
			BACKOFF,
			FATAL,
			UNKNOWN
		};
		friend std::ostream	&operator<<(std::ostream &s, const Process::State &state);
		/*
			Transitions:

			FATAL:
				start command				->	STARTING
		*/
	public:
		Process(TaskConfig *config)
		{
			_config = config;

			_time.start();
		}
		~Process() {}

		int	start()
		{
			if (_state != State::STOPPED && _state != State::FATAL)
			{
				std::cout << "Process already running" << std::endl;
				return (-1);
			}

			_transition(State::STARTING);

			return (_start());
		}
		int	stop()
		{
			if (_pid == 0)
			{
				std::cout << "Process not running" << std::endl;
				return (-1);
			}

			kill(_pid, _config->stop_signal);

			_transition(State::STOPPING);
			return (0);
		}
		int	restart()
		{
			_restart = true;
			stop();
			return (0);
		}
		void	restart_backoff()
		{
			_retry_count++;
			restart();
		}
		int	status()
		{
			return (0);
		}

		void	update();

		Process::State	state()
		{
			return (_state);
		}
	private:
		void	_update_stopped();
		void	_update_starting();
		void	_update_running();
		void	_update_exited();
		void	_update_stopping();
		void	_update_backoff();
		void	_transition(Process::State next_state)
		{
			std::cout << "\rProcess " << _config->name << " transition : " << _state << " -> " << next_state << std::endl;
			_state = next_state;
		}

		int	_start()
		{
			_start_timestamp = _time.get();

			_pid = fork();
			if (_pid == -1)
				return (-1);
			if (_pid != 0)
				return (_pid);

			execve(_config->cmds[0].c_str(), (char *const *)c_str_array(_config->cmds), (char *const *)get_env(_config->env));
			exit(EXIT_FAILURE);
		}
	private:
		TaskConfig	*_config = NULL;
		pid_t					_pid = 0;
		int						_id = 0;

		Process::State		_state = State::STOPPED;
		int		_retry_count = 0;

		Chrono	_time;
		double	_start_timestamp;
		double	_stop_timestamp;
		bool	_expected_exit;

		int			_return = 0;
		bool		_exited = false;

		bool		_restart = false;
};
