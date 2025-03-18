/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pepie <pepie@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/17 07:51:42 by pepie             #+#    #+#             */
/*   Updated: 2025/03/18 00:35:34 by pepie            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "philo.h"


bool	check_dead(t_philo *philosopher)
{
	pthread_mutex_lock(&philosopher->game_over_mutex);
	if (philosopher->game_over) {
		pthread_mutex_unlock(&philosopher->game_over_mutex);
		return (true);
	}
	pthread_mutex_unlock(&philosopher->game_over_mutex);
	return (false);
}

static int	is_philosopher_dead(t_philo *philo_gen, t_philosopher *philosopher)
{
	pthread_mutex_lock(&philo_gen->eating_mutex);
	if ((timestamp() - philosopher->last_time_eat >= (unsigned int)philo_gen->time_to_die
			&& !philosopher->is_eating))
		return (pthread_mutex_unlock(&philo_gen->eating_mutex), 1);
	return (pthread_mutex_unlock(&philo_gen->eating_mutex), 0);
}


static int	is_a_philosopher_dead(t_philo *philo)
{
	int	i;

	i = 0;
	while (i < philo->number_of_philosophers)
	{
		if (is_philosopher_dead(philo, &philo->philosophers[i]))
		{
			print_philo(&philo->philosophers[i], "died");
			pthread_mutex_lock(&philo->game_over_mutex);
			philo->game_over = true;
			pthread_mutex_unlock(&philo->game_over_mutex);
			return (1);
		}
		i++;
		usleep(100);
	}
	return (0);
}

void *death_monitor(void *arg)
{
    t_philo *philo_gen = (t_philo *)arg;
    
    while (true)
    {
		if (is_a_philosopher_dead(philo_gen))
			return (NULL);
		usleep(100);
    }
    return (NULL);
}
