#include "TaskConfig.hpp"
#include "JSONReader.hpp"
#include "Logger.hpp"

#include <csignal>
#include <exception>
#include <signal.h>
#include <fstream>
#include <map>
#include <stdexcept>


TaskConfig::TaskConfig(void) {}

bool TaskConfig::operator==(const TaskConfig &other)
{
    return (
        this->cmds == other.cmds
        && this->num_procs == other.num_procs
        && this->auto_start == other.auto_start
        && this->auto_restart == other.auto_restart
        && this->exit_codes == other.exit_codes
        && this->start_time == other.start_time
        && this->start_retries == other.start_retries
        && this->stop_signal == other.stop_signal
        && this->stop_time == other.stop_time
        && this->stdout_ == other.stdout_
        && this->stderr_ == other.stderr_
        && this->env == other.env
        && this->working_dir == other.working_dir
        && this->umask == other.umask
    );
}

bool TaskConfig::operator!=(const TaskConfig &other)
{
    return (!(*this == other));
}

std::map<std::string, TaskConfig *> TaskConfig::get_configs(std::string fileName)
{
    std::map<std::string, TaskConfig *> configs;

	std::fstream file(fileName.c_str());
	if (!file.is_open())
		throw std::runtime_error("Unable to open file.");

	std::string content;
	std::getline(file, content, '\0');

    JSONReader reader(content);

    JSONReader tasks = reader["tasks"];

    // Map for Restart Modes
    std::map<std::string, TaskConfig::RestartMode> restartModeMap;
    restartModeMap["always"] = TaskConfig::RestartMode::ALWAYS;
    restartModeMap["never"] = TaskConfig::RestartMode::NEVER;
    restartModeMap["on_error"] = TaskConfig::RestartMode::ON_ERROR;

    // Map for Signals
    std::map<std::string, int> signalMap;
    signalMap["sigkill"] = SIGKILL;
    signalMap["sigint"] = SIGINT;
    signalMap["sigterm"] = SIGTERM;
    signalMap["sighup"] = SIGHUP;
    signalMap["sigint"] = SIGINT;
    signalMap["sigquit"] = SIGQUIT;
    signalMap["sigill"] = SIGILL;
    signalMap["sigtrap"] = SIGTRAP;
    signalMap["sigabrt"] = SIGABRT;
    signalMap["sigiot"] = SIGIOT;
    signalMap["sigbus"] = SIGBUS;
    signalMap["sigfpe"] = SIGFPE;
    signalMap["sigusr1"] = SIGUSR1;
    signalMap["sigsegv"] = SIGSEGV;
    signalMap["sigusr2"] = SIGUSR2;
    signalMap["sigpipe"] = SIGPIPE;
    signalMap["sigalrm"] = SIGALRM;
    signalMap["sigterm"] = SIGTERM;
    signalMap["sigstkflt"] = SIGSTKFLT;
    signalMap["sigcont"] = SIGCONT;
    signalMap["sigtstp"] = SIGTSTP;
    signalMap["sigttin"] = SIGTTIN;
    signalMap["sigttou"] = SIGTTOU;
    signalMap["sigurg"] = SIGURG;
    signalMap["sigxcpu"] = SIGXCPU;
    signalMap["sigxfsz"] = SIGXFSZ;
    signalMap["sigvtalrm"] = SIGVTALRM;
    signalMap["sigprof"] = SIGPROF;
    signalMap["sigwinch"] = SIGWINCH;
    signalMap["sigpoll"] = SIGPOLL;
    signalMap["sigio"] = SIGIO;
    signalMap["sigpwr"] = SIGPWR;
    signalMap["sigsys"] = SIGSYS;

    for (auto taskName : tasks.keys())
    {
        JSONReader configData = tasks.get(taskName);
        TaskConfig *config = new TaskConfig;

        try {
            config->name = taskName;
            for (auto cmd : configData["cmds"].values())
                config->cmds.push_back(cmd.toString());
            if (config->cmds.size() == 0)
                throw std::runtime_error("cmds cannot be empty!");
            config->num_procs = configData["num_procs"].toInt();
            config->auto_start = configData["auto_start"].toBool();
            
            if (restartModeMap.find(configData["auto_restart"].toString()) == restartModeMap.end())
                throw std::runtime_error("Invalid restart mode!");
            config->auto_restart = restartModeMap[configData["auto_restart"].toString()];
            for (auto exit_code : configData["exit_codes"].values())
                config->exit_codes.push_back(exit_code.toInt());
            config->start_time = configData["start_time"].toInt();
            config->start_retries = configData["start_retries"].toInt();
            if (signalMap.find(configData["stop_signal"].toString()) == signalMap.end())
                throw std::runtime_error("Invalid stop signal!");
            config->stop_signal = signalMap[configData["stop_signal"].toString()];
            config->stop_time = configData["stop_time"].toInt();
            if (configData["stdout"].isString())
                config->stdout_ = configData["stdout"].toString();
            if (configData["stderr"].isString())
                config->stderr_ = configData["stderr"].toString();
            for (auto key : configData["env"].keys())
                config->env[key] = configData["env"][key].toString();
            config->working_dir = configData["working_dir"].toString();
            config->umask = configData["umask"].toInt();

            configs[taskName] = config;
        } catch (std::exception &e) {
            logger << Logger::ERROR << "Unable to load task '" << taskName << "'! (" << e.what() << ")." << ENDL;
            delete config;
        }
    }

    return configs;
}