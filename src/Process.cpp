#include "Process.hpp"
#include "Logger.hpp"
#include "TaskConfig.hpp"
#include <fcntl.h>

inline const char	**c_str_array(const std::vector<std::string> &vec)
{
	const char	**res = new const char *[vec.size() + 1];

	int	i = 0;
	for (const std::string &s : vec)
		res[i++] = strdup(s.c_str());
	res[i] = NULL;
	return (res);
}

inline const char	**get_env(const std::map<std::string, std::string> env)
{
	std::vector<std::string> new_env;

	for (auto key : env)
		new_env.push_back(key.first + "=" + key.second);
	return (c_str_array(new_env));
}

int Process::start()
{
	if (_state != State::STOPPED && _state != State::FATAL)
	{
		logger << Logger::ERROR << "Process already running!" << ENDL;
		return (-1);
	}

	_transition(State::STARTING);

	return (_start());
}

int	Process::stop()
{
	if (_pid == 0)
	{
		logger << Logger::ERROR << "Process not running" << ENDL;
		return (-1);
	}

	kill(_pid, _config->stop_signal);

	_transition(State::STOPPING);
	return (0);
}

int	Process::restart()
{
	_restart = true;
	stop();
	return (0);
}

void Process::restart_backoff()
{
	_retry_count++;
	restart();
}

int		status()
{
	return (0);
}

void Process::update()
{
    int		status = 0;

    int		wpid = waitpid(_pid, &status, WNOHANG);

    if (wpid != 0)
    {
        _return = WEXITSTATUS(status);
        _exited = WIFEXITED(status);
    }

    switch (_state)
    {
        case State::STOPPED:
            _update_stopped(); break;
        case State::STARTING:
            _update_starting(); break;
        case State::RUNNING:
            _update_running(); break;
        case State::EXITED:
            _update_exited(); break;
        case State::STOPPING:
            _update_stopping(); break;
		case State::BACKOFF:
            _update_backoff(); break;
    }
}

/*
	active for > starttime			->	RUNNING
	exited before starttime			->	BACKOFF
	exec failed (timer exceeded...)	->	BACKOFF
	stop command					->	STOPPING
*/
void	Process::_update_starting()
{
    if (_exited)
    {
        _transition(State::BACKOFF);
        return ;
    }
    if (_time.get() - _start_timestamp > _config->start_time)
    {
        _transition(State::RUNNING);
        return ;
    }
}

/*
	retry timer expires			->	STARTING
	too many retries			->	FATAL
*/
void	Process::_update_backoff()
{
	if (_retry_count >= _config->start_retries)
	{
        _transition(State::EXITED);
		return ;
	}
	else
		restart_backoff();
}

/*
    autorestart					->	STARTING
    unexpected exit				->	STARTING
    else						->	STOPPED
*/
void	Process::_update_exited()
{
    bool	expected = std::find(_config->exit_codes.begin(), _config->exit_codes.end(), _return) != _config->exit_codes.end();

    _return = 0;
    _exited = false;

    if ((_config->auto_restart == TaskConfig::RestartMode::ALWAYS)
        || !expected && _config->auto_restart == TaskConfig::RestartMode::ON_ERROR)
    {
		if (_config->start_retries == 0 || _retry_count < _config->start_retries)
		{
			_start();
			_transition(State::STARTING);
			return ;
		}
    }

    _transition(State::STOPPED);
}

/*
    process exits				->	STOPPED
    timeout exceeded			->	send SIGKILL
    SIGKILL success				->	STOPPED
*/
void	Process::_update_stopping()
{
    if (_exited)
    {
        _transition(State::STOPPED);
        return ;
    }
    if (_time.get() - _stop_timestamp > _config->stop_time)
    {
        kill(_pid, SIGKILL);
        return ;
    }
}

/*
	expected exit				->	EXITED
	unexpected exit				->	EXITED
	stop command				->	STOPPING
*/
void	Process::_update_running()
{
    if (_exited)
    {
        _transition(State::EXITED);
        return ;
    }
}

/*
	start command				->	STARTING
	autostart 					->	STARTING
*/
void	Process::_update_stopped()
{
	_return = 0;
    _exited = false;

	if (_restart)
	{
		_restart = false;
		start();
		return ;
	}
	if (_config->auto_restart == TaskConfig::RestartMode::ALWAYS)
	{
		if (_config->start_retries == 0 || _retry_count < _config->start_retries)
		{
			start();
			return ;
		}
	}
}

void	Process::_transition(Process::State next_state)
{
	logger << Logger::DEBUG << "Process " << _config->name << " transition : " << _state << " -> " << next_state << ENDL;
	_state = next_state;
}

int	Process::_start()
{
	_start_timestamp = _time.get();

	_pid = fork();
	if (_pid == -1)
		return (-1);
	if (_pid != 0)
		return (_pid);
    
    int fd_out = open(_config->stdout_.c_str(), O_APPEND | O_CREAT | O_WRONLY);
    int fd_err = open(_config->stderr_.c_str(), O_APPEND | O_CREAT | O_WRONLY);
    
    if (fd_out != -1)
    {
        dup2(fd_out, 1);
        close(fd_out);
    }
    else
        logger << Logger::ERROR << "Unable to open '" << _config->stdout_ << "'." << ENDL;
    if (fd_err != -1)
    {
        dup2(fd_err, 2);
        close(fd_err);
    }
    else
        logger << Logger::ERROR << "Unable to open '" << _config->stderr_ << "'." << ENDL;

    chdir(_config->working_dir.c_str());

	execve(_config->cmds[0].c_str(), (char *const *)c_str_array(_config->cmds), (char *const *)get_env(_config->env));
	exit(EXIT_FAILURE);
}

bool Process::is_alive()
{
    return (_pid != 0);
}

std::ostream	&operator<<(std::ostream &s, const Process::State &state)
{
    switch (state)
    {
        case Process::State::STOPPED:
            s << "STOPPED"; break;
        case Process::State::STARTING:
            s << "STARTING"; break;
        case Process::State::RUNNING:
            s << "RUNNING"; break;
        case Process::State::STOPPING:
            s << "STOPPING"; break;
        case Process::State::EXITED:
            s << "EXITED"; break;
        case Process::State::BACKOFF:
            s << "BACKOFF"; break;
        case Process::State::FATAL:
            s << "FATAL"; break;
        case Process::State::UNKNOWN:
            s << "UNKNOWN"; break;
    }
    return (s);
}
