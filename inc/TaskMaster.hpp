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
		TaskMaster(void);
		~TaskMaster(void);

		void	start(const std::string &config_file);

	private:
		void	_loop();
		void	_user_loop();
		void	_user_command(const std::string &input);
		void	_stop();

	private:

		std::map<std::string, std::pair<TaskConfig *, std::vector<Process *>>> _tasks;
		std::mutex	_lock;
		std::thread	_user;

		std::atomic_bool	_running;
};