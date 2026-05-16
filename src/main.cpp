/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbatty <mbatty@student.42angouleme.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 12:30:09 by mbatty            #+#    #+#             */
/*   Updated: 2026/05/16 11:52:58 by mbatty           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Process.hpp"

#include <thread>
#include <atomic>
#include <mutex>
#include <sstream>

void	shell_loop()
{
	while (1)
	{
		std::cout << "caca" << std::endl;
	}
}

class	Taskmaster
{
	public:
		Taskmaster() {}
		~Taskmaster() {}

		void	start(const std::string &config_file)
		{
			ProcessDefinition	def;

			def.name = "listener";
			def.cmd = "./test.sh";
			def.av = {"./test.sh"};
			def.env = {};

			def.processes_count = 1;

			def.start_at_launch = true;
			def.restart_mode = ProcessDefinition::RestartMode::ALWAYS;
			def.expected_exit_code = {0};
			def.run_time_validity = 0;
			def.restart_tries = 0;
			def.stop_signal = SIGINT;
			def.max_stop_time = 1;

			_running = true;

			proc = new Process(&def);

			proc->start();

			_user = std::thread([this](){this->_user_loop();});

			_loop();
		}

	private:
		void	_loop()
		{
			while (_running)
			{
				_lock.lock();
				proc->update();
				_lock.unlock();
			}
		}
		void	_user_loop()
		{
			while (_running)
			{
				std::string	input;

				std::cout << "\r$> " << std::flush;
				if (std::getline(std::cin, input))
				{
					_lock.lock();
					_user_command(input);
					_lock.unlock();
				}
				else
					break ;
			}
		}
		void	_user_command(const std::string &input)
		{
			std::istringstream	s(input);

			std::string	command;
			s >> command;

			if (command == "help")
			{
				std::cout << '\r' << "Available commands:\n  start <process_name>\n  stop <process_name>\n  restart <process_name>" << std::endl;
				return ;
			}
			if (command == "start")
			{
				proc->start();
				return ;
			}
			if (command == "stop")
			{
				proc->stop();
				return ;
			}
			if (command == "restart")
			{
				proc->restart();
				return ;
			}
		}

	private:
		void	_stop()
		{
			_running = false;
		}
		Process	*proc;

		std::mutex	_lock;
		std::thread	_user;

		std::atomic_bool	_running;
};

int	main(int ac, char **av, char **envp)
{
	Taskmaster	systemd_wanna_be;

	try
	{
		systemd_wanna_be.start("");
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return (1);
	}
	return (0);
}
