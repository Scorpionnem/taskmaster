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

#include "ProcessDefinition.hpp"
#include "Chrono.hpp"

inline const char	**c_str_array(const std::vector<std::string> &vec)
{
	const char	**res = new const char *[vec.size() + 1];

	int	i = 0;
	for (const std::string &s : vec)
		res[i++] = s.c_str();
	res[i] = NULL;
	return (res);
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

			STOPPED:
				start command				->	STARTING
				autostart 					->	STARTING

			BACKOFF:
				retry timer expires			->	STARTING
				too many retries			->	FATAL

			FATAL:
				start command				->	STARTING
		*/
	public:
		Process(ProcessDefinition *def)
		{
			_def = def;

			_def->register_process(this);

			_time.start();
		}
		~Process() {}

		int	start()
		{
			_transition(State::STARTING);

			return (_start());
		}
		int	_start()
		{
			_start_timestamp = _time.get();

			_pid = fork();
			if (_pid == -1)
				return (-1);
			if (_pid != 0)
				return (_pid);

			execve(_def->cmd.c_str(), (char *const *)c_str_array(_def->av), (char *const *)c_str_array(_def->env));
			exit(EXIT_FAILURE);
		}

		void	update();

	private:
		void	_update_stopped();
		void	_update_starting();
		void	_update_running();
		void	_update_exited();
		void	_update_stopping();
		void	_transition(Process::State next_state)
		{
			std::cout << "Process " << _def->name << " transition : " << _state << " -> " << next_state << std::endl;
			_state = next_state;
		}
	private:
		ProcessDefinition	*_def = NULL;
		pid_t					_pid = 0;
		int						_id = 0;

		Process::State		_state = State::STOPPED;
		int		_retry_count;
		
		Chrono	_time;
		double	_start_timestamp;
		double	_stop_timestamp;
		bool	_expected_exit;

		int			_return = 0;
		bool		_exited = false;
};
