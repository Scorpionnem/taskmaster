#include "TaskMaster.hpp"
#include "Process.hpp"
#include <atomic>
#include <unistd.h>
#include <vector>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>

int	sig = 0;

TaskMaster::TaskMaster(const std::string &configFile) {
	_configFile = configFile;
	_all_stopped = false;
}

TaskMaster::~TaskMaster(void) {
	for (auto task : _tasks)
	{
		for (auto process : task.second.second)
			process->force_stop();
	}
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

std::atomic_bool	sighup_fired;

void	sighup_handler(int s)
{
	(void)s;
	sighup_fired = true;
}

void	sig_handler(int s)
{
	sig = s;
}

void TaskMaster::_loop()
{
	sighup_fired = false;

	signal(SIGHUP, sighup_handler);

	while (_running)
	{
		_lock.lock();
		if (sighup_fired)
		{
			sighup_fired = false;
			_reload_config();
		}
		for (auto task : _tasks)
			for (auto process : task.second.second)
				process->update();
		_lock.unlock();
		// usleep(100); # DEBUG PURPOSE
	}
	_user.join(); // wait for user to finish
	while (!_all_stopped)
	{
		_all_stopped = true;
		for (auto task : _tasks)
			for (auto process : task.second.second)
			{
				process->force_stop();
				process->update();
				if (process->state() != Process::State::STOPPED)
					_all_stopped = false;
			}
	}
}

void TaskMaster::_user_loop()
{
	signal(SIGINT, SIG_IGN);

	while (_running)
	{
		char	*line;

		line = readline("$>");
		if (line)
		{
			std::string	input(line);

			_lock.lock();
			_user_command(input);
			_lock.unlock();

			add_history(line);
			free(line);

			if (sig != 0)
				_running = false;
		}
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
		this->_start_task(s);
	else if (command == "stop")
		this->_stop_task(s);
	else if (command == "restart")
		this->_restart_task(s);
	else if (command == "status")
		this->_status(s);
	else if (command == "quit")
		this->_stop();
	else if (command == "reload")
		this->_reload_config();
	else {
		if (!command.empty())
			std::cout << "Command not found!" << std::endl;
	}
}

void TaskMaster::_stop()
{
	_running = false;
}

void TaskMaster::_reload_config()
{
	std::cout << "Reloading config..." << std::endl;
	logger << Logger::INFO << "Reloading config..." << ENDL;
	std::map<std::string, TaskConfig *> configs;
	try {
		configs = TaskConfig::get_configs(_configFile);
	} catch (std::exception &e)
	{
		logger << Logger::ERROR << "Unable to reload config! (" << e.what() << ")" << ENDL;
		std::cerr << "Unable to reload config! (" << e.what() << ")" << std::endl;
		return;
	}

	std::vector<std::string> taskToRemove;

	// Check for existing tasks
	for (auto task : _tasks)
	{
		std::string taskName = task.first;

		// if the task is not in the new config
		if (configs.find(taskName) == configs.end())
		{
			logger << Logger::DEBUG << "Removing task '" << taskName << "'..." << ENDL;
			// stopping and free all processes and config
			logger << Logger::DEBUG << "Stopping all processes of task '" << taskName << "'..." << ENDL;
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
			logger << Logger::DEBUG << "Stopping all processes of task '" << taskName << "'..." << ENDL;
			for (auto process : task.second.second)
			{
				if (process->is_alive())
					process->stop();
				delete process;
			}
			_tasks[taskName].second.clear();
			delete task.second.first;

			// Recreating processes
			logger << Logger::DEBUG << "Recreate processes of task '" << taskName << "'..." << ENDL;
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
		else {
			std::cout << "Task '" << taskName << "' nothing to do." << std::endl;
			delete configs[taskName];
		}
	}

	// Check for the new task
	for (auto config: configs)
	{
		std::string taskName = config.first;
		
		if (_tasks.find(taskName) != _tasks.end())
			continue;

		// New Task
		logger << Logger::DEBUG << "Creating new task '" << taskName << "'..." << ENDL;
		std::vector<Process *> processes;
		
		logger << Logger::DEBUG << "Starting processes of task '" << taskName << "'..." << ENDL;
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
	logger << Logger::INFO << "Config reloaded!" << ENDL;
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

void TaskMaster::_start_task(std::istringstream &s) {
	std::string taskName;

	if (!(s >> taskName))
	{
		std::cerr << "Please provide an argument!" << std::endl;
		return ;
	}

	if (_tasks.find(taskName) == _tasks.end())
	{
		std::cerr << "Task '" << taskName << "' not found!" << std::endl;
		return ;
	}

	uint startedProcesses = 0;
	for (auto process: _tasks[taskName].second)
	{
		if (!process->is_alive())
		{
			process->start(true);
			startedProcesses++;
		}
	}
	if (startedProcesses)
		std::cout << "Started " << startedProcesses << " processes for task '" << taskName << "'!" << std::endl;
	else
		std::cout << "No processes started, " << _tasks[taskName].second.size() << " processes already running!" << std::endl;
}

void TaskMaster::_restart_task(std::istringstream &s) {
	std::string taskName;

	if (!(s >> taskName))
	{
		std::cerr << "Please provide an argument!" << std::endl;
		return ;
	}

	if (_tasks.find(taskName) == _tasks.end())
	{
		std::cerr << "Task '" << taskName << "' not found!" << std::endl;
		return ;
	}

	uint stoppedProcesses = 0;
	for (auto process: _tasks[taskName].second)
		process->restart(true);
	std::cout << _tasks[taskName].second.size() << " processes restarted!" << std::endl;
}
void TaskMaster::_stop_task(std::istringstream &s) {
	std::string taskName;

	if (!(s >> taskName))
	{
		std::cerr << "Please provide an argument!" << std::endl;
		return ;
	}

	if (_tasks.find(taskName) == _tasks.end())
	{
		std::cerr << "Task '" << taskName << "' not found!" << std::endl;
		return ;
	}

	uint stoppedProcesses = 0;
	for (auto process: _tasks[taskName].second)
	{
		if (process->is_alive())
		{
			process->stop();
			stoppedProcesses++;
		}
	}
	if (stoppedProcesses)
		std::cout << "Stopped " << stoppedProcesses << " processes for task '" << taskName << "'!" << std::endl;
	else
		std::cout << "No processes stopped, " << _tasks[taskName].second.size() << " processes already stopped!" << std::endl;
}
