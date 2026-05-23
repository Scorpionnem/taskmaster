/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ehode <ehode@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04 22:59:59 by ehode             #+#    #+#             */
/*   Updated: 2026/05/23 17:15:20 by ehode            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "Logger.hpp"

# include <ctime>
# include <iostream>

Logger logger(Logger::DEBUG);

static std::string getFormatedTime(std::string const fmt) {
	time_t timestamp = time(NULL);
	struct tm datetime = *localtime(&timestamp);

	char tmp[1024];
	std::strftime(tmp, sizeof(tmp), fmt.c_str(), &datetime);
	return (std::string(tmp));
}

Logger::Logger(Level level) {
	_level = level;

	std::string fileName = "log/";
	fileName.append(getFormatedTime("%Y-%m-%d_%H-%M-%S"));
	fileName.append(".log");
	_file.open(fileName.c_str());
	if (!_file.is_open())
		this->error("Unable to open log file.");
}

Logger::~Logger(void) {
	if (_file.is_open())
		_file.close();
}

void Logger::setLevel(Level level) {
	_level = level;
}

std::string Logger::getLevelName(Level level) {
	switch (level)
	{
		case DEBUG:
			return("DEBUG");
		case INFO:
			return("INFO");
		case WARNING:
			return("WARNING");
		case ERROR:
			return("ERROR");
		case CRITICAL:
			return ("CRITICAL");
		default:
			return ("UNKNOWN");
	}
}

void Logger::log(Level level, std::string const &message) {
	if (_level > level)
		return;

	std::string currentTime = getFormatedTime("%Y-%m-%d %H:%M:%S");
	std::string levelName = getLevelName(level);

	if (_file.is_open())
		_file << "[" << currentTime << "] " << "[" << levelName << "] " << message << std::endl;
}

Logger &Logger::operator<<(Level level) {
	_level = level;
	return (*this);
}

Logger &Logger::operator<<(const char *s) {
	if (std::string(s) == "\n")
	{
		this->log(_level, _current.str());
		_current.str("");
	}
	else
		_current << s;
	return (*this);
}