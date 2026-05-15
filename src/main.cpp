/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbatty <mbatty@student.42angouleme.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 12:30:09 by mbatty            #+#    #+#             */
/*   Updated: 2026/04/25 17:26:17 by mbatty           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Process.hpp"

int	main(int ac, char **av, char **envp)
{
	ProcessDefinition	def;

	def.name = "listener";
	def.cmd = "./test.sh";
	def.av = {"./test.sh"};
	def.env = {};

	def.processes_count = 1;

	def.start_at_launch = true;
	def.restart_mode = ProcessDefinition::RestartMode::ALWAYS;
	def.expected_exit_code = {0};
	def.run_time_validity = 0;
	def.restart_tries = 0;
	def.stop_signal = SIGINT;
	def.max_stop_time = 1;

	Process	proc = Process(&def);

	proc.start();

	while (1)
	{
		proc.update();
	}

	return (0);
}
