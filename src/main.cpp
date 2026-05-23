/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ehode <ehode@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 12:30:09 by mbatty            #+#    #+#             */
/*   Updated: 2026/05/23 21:17:36 by ehode            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "TaskMaster.hpp"

int	main(int ac, char **av, char **envp)
{
	if (ac != 2)
	{
		std::cerr << "No config file provided!" << std::endl;
		return (1);
	} 
	
	TaskMaster	systemd_wanna_be;
	try
	{
		systemd_wanna_be.start(av[1]);
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return (1);
	}
	return (0);
}
