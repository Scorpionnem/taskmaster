/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ehode <ehode@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 12:30:09 by mbatty            #+#    #+#             */
/*   Updated: 2026/05/23 17:12:52 by ehode            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Logger.hpp"
#include "Process.hpp"
#include "TaskConfig.hpp"

#include <map>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <sstream>
#include <utility>
#include <vector>

class	Taskmaster
{
	public:
		Taskmaster() {
			_running = true;
		}
		~Taskmaster() {}

		void	start(const std::string &config_file)
		{
			std::map<std::string, TaskConfig *> configs = TaskConfig::get_configs(config_file);

			for (auto config : configs)
			{
				std::vector<Process *> processes;
				
				for (uint i = 0; i < config.second->num_procs; i++)
				{
					Process *process = new Process(config.second);
					if (config.second->auto_start)
						process->start();
					processes.push_back(process);
				}

				_tasks[config.first] = {config.second, processes};
			}
			
			_user = std::thread([this](){this->_user_loop();});

			_loop();
		}

	private:
		void	_loop()
		{
			while (_running)
			{
				_lock.lock();
				for (auto task : _tasks)
					for (auto process : task.second.second)
						process->update();
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
				std::cout << '\r' << "Available commands:\n  start <task_name>\n  stop <task_name>\n  restart <task_name>" << std::endl;
				return ;
			}
			if (command == "start")
			{
				// start [task name]
				return ;
			}
			if (command == "stop")
			{
				// stop [task name]
			}
			if (command == "restart")
			{
				// restart [task name]
				return ;
			}
		}

	private:
		void	_stop()
		{
			_running = false;
		}

		std::map<std::string, std::pair<TaskConfig *, std::vector<Process *>>> _tasks;
		std::mutex	_lock;
		std::thread	_user;

		std::atomic_bool	_running;
};

int	main(int ac, char **av, char **envp)
{
	if (ac != 2)
	{
		logger << Logger::ERROR << "No config file provided!" << ENDL;
		return (1);
	} 
	
	Taskmaster	systemd_wanna_be;
	try
	{
		systemd_wanna_be.start(av[1]);
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return (1);
	}
	return (0);
}
