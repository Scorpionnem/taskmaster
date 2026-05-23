#pragma once

#include <map>
#include <string>
#include <sys/types.h>
#include <vector>


class TaskConfig
{
    public:
		enum class	RestartMode
		{
			ALWAYS,
			NEVER,
			ON_ERROR, // Restarts if exit code is not in accepted list
		};

		TaskConfig(void);

		std::string							name = "task";
		std::vector<std::string>			cmds;
		uint								num_procs = 0;
		bool								auto_start = true;
		RestartMode							auto_restart = RestartMode::ON_ERROR;
		std::vector<uint>					exit_codes;
		uint								start_time = 1;
		uint								start_retries = 3;
		int									stop_signal;
		uint								stop_time = 3;
		std::string							stdout_ = "/dev/null";
		std::string							stderr_ = "/dev/null";
		std::map<std::string, std::string>	env;
		std::string							working_dir = ".";
		uint								umask = 0;

		static std::map<std::string, TaskConfig *>	get_configs(std::string file);
};