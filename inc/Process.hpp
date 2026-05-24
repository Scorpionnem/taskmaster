#pragma once

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <ctime>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <string.h>

#include "Chrono.hpp"
#include "Logger.hpp"
#include "TaskConfig.hpp"

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
		~Process()
		{
		}

		int		start(bool reset_try = false);
		int		stop();
		int		force_stop();
		int		restart();
		void	restart_backoff();
		int		status();
		void	update();
		bool	is_alive();

		Process::State	state() { return _state; }
	private:
		void	_update_stopped();
		void	_update_starting();
		void	_update_running();
		void	_update_exited();
		void	_update_stopping();
		void	_update_backoff();
		void	_transition(Process::State next_state);

		int	_start();
	private:
		TaskConfig	*_config = NULL;
		pid_t					_pid = 0;
		int						_id = 0;

		Process::State		_state = State::STOPPED;
		int		_retry_count = 0;

		Chrono	_time;
		double	_start_timestamp = 0;
		double	_stop_timestamp = 0;
		bool	_expected_exit;

		int			_return = 0;
		bool		_exited = false;

		bool		_restart = false;
		bool		_force_stop = false;
};
