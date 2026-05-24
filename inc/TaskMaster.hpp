#include "Process.hpp"
#include "TaskConfig.hpp"

#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <thread>

class	TaskMaster
{
	public:
		TaskMaster(const std::string &configFile);
		~TaskMaster(void);

		void	start(void);

	private:
		void	_loop();
		void	_user_loop();
		void	_user_command(const std::string &input);
		void	_stop();
		void	_reload();
		void	_status(std::istringstream &s);

	private:
		std::string _configFile;
		std::map<std::string, std::pair<TaskConfig *, std::vector<Process *>>> _tasks;
		std::mutex	_lock;
		std::thread	_user;
		
		std::atomic_bool	_running;
		std::atomic_bool	_all_stopped;
};
