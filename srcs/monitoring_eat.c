/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitoring_eat.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pepie <pepie@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/17 07:51:42 by pepie             #+#    #+#             */
/*   Updated: 2025/03/18 02:55:35 by pepie            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "philo.h"

static int	is_all_ate(t_philo *philo)
{
	int	i;

	i = 0;
	while (i < philo->number_of_philosophers)
	{
		pthread_mutex_lock(&philo->eating_mutex);
		if (philo->philosophers[i].eaten_count < philo->num_must_eat)
		{
			pthread_mutex_unlock(&philo->eating_mutex);
			return (0);
		}
		pthread_mutex_unlock(&philo->eating_mutex);
		i++;
		usleep(500);
	}
	pthread_mutex_lock(&philo->game_over_mutex);
	philo->game_over = true;
	pthread_mutex_unlock(&philo->game_over_mutex);
	return (1);
}

void	*monitor_eat(void *arg)
{
	t_philo		*philo;
	long long	start;
	long long	now;

	philo = (t_philo *)arg;
	pthread_mutex_lock(&philo->start_mutex);
	pthread_mutex_unlock(&philo->start_mutex);
	start = timestamp();
	now = timestamp();
	while (!is_all_ate(philo))
	{
		if ((now - start) % 50 == 0)
			if (check_dead(philo))
				return (NULL);
		usleep(500);
		now = timestamp();
	}
	return (NULL);
}
