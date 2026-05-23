#pragma once

#include <map>
#include <string>
#include <vector>


class TaskConfig
{
    public:
		std::vector<std::string>			cmds;
		uint								num_procs;
		bool								auto_start;
		std::string							auto_restart;
		std::vector<uint>					exitcodes;
		uint								start_time;
		uint								start_retries;
		std::string							stop_signal;
		uint								stop_time;
		std::string							stdout_;
		std::string							stderr_;
		std::map<std::string, std::string>	env;
		std::string							working_dir;
		std::string							umask;

		static std::vector<TaskConfig *>	get_configs(std::string file);
};