#include "TaskMaster.hpp"

TaskMaster::TaskMaster(const std::string &configFile) {
	_configFile = configFile;
}

TaskMaster::~TaskMaster(void) {
	for (auto task : _tasks)
	{
		delete task.second.first;
		for (auto process : task.second.second)
			delete process;
	}
}

void TaskMaster::start(void)
{
	_running = true;

	std::map<std::string, TaskConfig *> configs = TaskConfig::get_configs(_configFile);

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

void TaskMaster::_loop()
{
	while (_running)
	{
		_lock.lock();
		for (auto task : _tasks)
			for (auto process : task.second.second)
				process->update();
		_lock.unlock();
	}
	_user.join(); // wait for user to finish
}

void TaskMaster::_user_loop()
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

void TaskMaster::_user_command(const std::string &input)
{
	std::istringstream	s(input);

	std::string	command;
	s >> command;

	if (command == "help")
	{
		std::cout << '\r' << "Available commands:\n\tstart <task_name>\n\tstop <task_name>\n\trestart <task_name>" << std::endl;
		return ;
	}
	else if (command == "start")
	{
		// start [task name]
		return ;
	}
	else if (command == "stop")
	{
		// stop [task name]
	}
	else if (command == "restart")
	{
		// restart [task name]
		return ;
	}
	else if (command == "status")
	{
		std::string arg;
		
		if (s >> arg)
		{
			if (_tasks.find(arg) == _tasks.end())
			{
				std::cerr << "No task '" << arg << "' found!" << std::endl;
				return ;
			}
			std::cout << "- " << arg << std::endl;
			uint i = 0;
			for (auto process: _tasks[arg].second)
			{
				std::cout << "\t" << i << ": " << process->state() << std::endl;
				i++;
			}
		}
		else {
			for (auto task : _tasks)
			{
				std::cout << "- " << task.first << std::endl;
				uint i = 0;
				for (auto process: task.second.second)
				{
					std::cout << "\t" << i << ": " << process->state() << std::endl;
					i++;
				}
			}
		}
	}
	else if (command == "quit")
		this->_stop();
	else {
		if (!command.empty())
			std::cout << "Command not found!" << std::endl;
	}
}

void TaskMaster::_stop()
{
	_running = false;
}