/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   kill.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pepie <pepie@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/04 01:50:13 by pepie             #+#    #+#             */
/*   Updated: 2024/12/04 01:50:13 by pepie            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "philo.h"

bool	check_all_philo_eaten(t_philo *philo_gen)
{
	int		i;
	bool	game_over;

	i = 0;
	if (philo_gen->num_eat_max == -1)
		return (false);
	game_over = true;
	pthread_mutex_lock(&philo_gen->num_eaten_mutex);
	while (i < philo_gen->number_of_philosophers)
	{
		if (philo_gen->num_eaten[i] < philo_gen->num_eat_max)
		{
			game_over = false;
			break ;
		}
		i++;
	}
	pthread_mutex_unlock(&philo_gen->num_eaten_mutex);
	return (game_over);
}

void	kill_philosopher(t_philosopher *philosopher, t_philo *philo_gen, t_ll time, bool show_dead)
{
	int	i;

	pthread_mutex_lock(&philo_gen->game_over_mutex);
	if (philo_gen->game_over)
	{
		pthread_mutex_unlock(&philo_gen->game_over_mutex);
		return ;
	}
	philo_gen->game_over = true;
	pthread_mutex_unlock(&philo_gen->game_over_mutex);
	pthread_mutex_lock(&philosopher->dead);
	if (philosopher->game_over)
	{
		pthread_mutex_unlock(&philosopher->dead);
		return ;
	}
	philosopher->status = DEAD;
	philosopher->game_over = true;
	pthread_mutex_unlock(&philosopher->dead);
	if (show_dead)
		printf("%lld %d died\n", time - philo_gen->time_start, philosopher->id);
	i = 0;
	pthread_mutex_lock(&philo_gen->num_eaten_mutex);
	while (i < philo_gen->number_of_philosophers)
	{
		pthread_mutex_lock(&philo_gen->philosophers[i].dead);
		philo_gen->philosophers[i].game_over = true;
		pthread_mutex_unlock(&philo_gen->philosophers[i].dead);
		i++;
	}
	pthread_mutex_unlock(&philo_gen->num_eaten_mutex);
}