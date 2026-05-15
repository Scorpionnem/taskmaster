#include "Process.hpp"

void	Process::update()
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
    if (_time.get() - _start_timestamp > _def->run_time_validity)
    {
        _transition(State::RUNNING);
        return ;
    }
}

/*
    autorestart					->	STARTING
    unexpected exit				->	STARTING
    else						->	STOPPED
*/
void	Process::_update_exited()
{
    bool	expected = std::find(_def->expected_exit_code.begin(), _def->expected_exit_code.end(), _return) != _def->expected_exit_code.end();

    _return = 0;
    _exited = false;

    if (_def->restart_mode == ProcessDefinition::RestartMode::ALWAYS
        || !expected && _def->restart_mode == ProcessDefinition::RestartMode::ON_ERROR)
    {
        _start();
        _transition(State::STARTING);
        return ;
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
    if (_time.get() - _stop_timestamp > _def->max_stop_time)
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

void	Process::_update_stopped()
{
    
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