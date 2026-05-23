#include "TaskMaster.hpp"
#include "Process.hpp"
#include <unistd.h>
#include <vector>

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
		usleep(1000);
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
		return ;
	}
	else if (command == "restart")
	{
		// restart [task name]
		return ;
	}
	else if (command == "status")
		this->_status(s);
	else if (command == "quit")
		this->_stop();
	else if (command == "reload")
		this->_reload();
	else {
		if (!command.empty())
			std::cout << "Command not found!" << std::endl;
	}
}

void TaskMaster::_stop()
{
	_running = false;
}

void TaskMaster::_reload()
{
	std::cout << "Reloading config..." << std::endl;
	std::map<std::string, TaskConfig *> configs = TaskConfig::get_configs(_configFile);

	std::vector<std::string> taskToRemove;

	// Check for existing tasks
	for (auto task : _tasks)
	{
		std::string taskName = task.first;

		// if the task is not in the new config
		if (configs.find(taskName) == configs.end())
		{
			// stopping and free all processes and config
			for (auto process : task.second.second)
			{
				if (process->is_alive())
					process->stop();
				delete process;
			}
			delete task.second.first;
			taskToRemove.push_back(taskName);
		}
		// if the config have change
		else if (*task.second.first != *configs[taskName])
		{
			// Deleting old processes
			for (auto process : task.second.second)
			{
				if (process->is_alive())
					process->stop();
				delete process;
			}
			_tasks[taskName].second.clear();
			delete task.second.first;

			// Recreating processes
			_tasks[taskName].first = configs[taskName];
			for (uint i = 0; i < configs[taskName]->num_procs; i++)
			{
				Process *process = new Process(_tasks[taskName].first);
				if (_tasks[taskName].first->auto_start)
					process->start();
				_tasks[taskName].second.push_back(process);
			}
			std::cout << "Task '" << taskName << "' restarted!" << std::endl;
		}
		else
			std::cout << "Task '" << taskName << "' nothing to do." << std::endl;
	}

	// Check for the new task
	for (auto config: configs)
	{
		std::string taskName = config.first;

		if (_tasks.find(taskName) != _tasks.end())
			continue;
		
		// New Task
		std::vector<Process *> processes;
		
		for (uint i = 0; i < config.second->num_procs; i++)
		{
			Process *process = new Process(config.second);
			if (config.second->auto_start)
				process->start();
			processes.push_back(process);
		}

		_tasks[config.first] = {config.second, processes};
		std::cout << "Task '" << taskName << "' created!" << std::endl;
	}

	for (auto taskName : taskToRemove)
	{
		_tasks.erase(_tasks.find(taskName));
		std::cout << "Task '" << taskName << "' removed!" << std::endl;
	}
	std::cout << "Config reloaded!" << std::endl;
}

void TaskMaster::_status(std::istringstream &s)
{
	if (_tasks.size() == 0)
	{
		std::cout << "No task registered!" << std::endl;
		return;
	}

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