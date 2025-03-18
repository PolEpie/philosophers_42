/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pepie <pepie@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/17 07:51:42 by pepie             #+#    #+#             */
/*   Updated: 2025/03/18 03:03:41 by pepie            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "philo.h"

t_fork	**create_forks(t_philo *philo)
{
	t_fork	**forks;
	int		i;

	forks = malloc(sizeof(t_fork) * philo->number_of_philosophers);
	if (!forks)
		return (printf("Malloc failed\n"), NULL);
	i = 0;
	while (i < philo->number_of_philosophers)
	{
		forks[i] = malloc(sizeof(t_fork));
		if (!forks[i])
			return (printf("Malloc failed\n"), NULL);
		forks[i]->user_id = -1;
		forks[i]->id = i + 1;
		pthread_mutex_init(&forks[i]->mutex, NULL);
		i++;
	}
	philo->forks = forks;
	return (forks);
}

bool	create_mutex(t_philo *philo)
{
	philo->tid = malloc(sizeof(pthread_t) * philo->number_of_philosophers);
	if (!philo->tid)
		return (true);
	return (false);
}

static int	init_times(t_philo *philo)
{
	int		i;
	size_t	time;

	i = 0;
	time = timestamp();
	philo->time_start = time;
	while (i < philo->number_of_philosophers)
	{
		philo->philosophers[i].last_time_eat = time;
		i++;
	}
	return (1);
}

int	create_philosophers(t_philo *philo)
{
	t_philosopher	*philosophers;
	t_fork			**forks;
	long long		time;
	int				i;

	philosophers = malloc(sizeof(t_philosopher) * philo->number_of_philosophers);
	if (!philosophers || create_mutex(philo))
		return (printf("Malloc failed\n"), 1);
	forks = create_forks(philo);
	if (forks == NULL)
		return (printf("Malloc failed\n"), 1);
	i = 0;
	time = timestamp();
	philo->time_start = time;
	philo->philosophers = philosophers;
	philo->game_over = false;
	pthread_mutex_init(&philo->write_mutex, NULL);
	pthread_mutex_init(&philo->eating_mutex, NULL);
	pthread_mutex_init(&philo->game_over_mutex, NULL);
	pthread_mutex_init(&philo->start_mutex, NULL);
	pthread_mutex_lock(&philo->start_mutex);
	while (i < philo->number_of_philosophers)
	{
		philosophers[i].time_think = (philo->time_to_eat * ((philo->number_of_philosophers % 2) + 1)) - philo->time_to_sleep;
		philosophers[i].id = i + 1;
		philosophers[i].eaten_count = 0;
		philosophers[i].philo = philo;
		philosophers[i].is_eating = false;
		philosophers[i].left_fork = forks[i];
		if (i == 0)
			philosophers[i].right_fork = forks[philo->number_of_philosophers - 1];
		else
			philosophers[i].right_fork = forks[i - 1];
		philosophers[i].last_time_eat = time;
		i++;
	}
	i = 0;
	while (i < philo->number_of_philosophers)
	{
		if (pthread_create(&philo->tid[i], NULL, philo_think, &philosophers[i]))
		{
			return (printf("Error creating philosopher thread\n"), 1);
		}
		i++;
	}
	if (pthread_create(&philo->death_monitor_thread, NULL, death_monitor, philo))
		return (printf("Error creating death monitor thread\n"), 1);
	if (philo->num_must_eat != -1)
	{
		if (pthread_create(&philo->eat_monitor_thread, NULL, monitor_eat, philo))
			return (printf("Error creating eat monitor thread\n"), 1);
	}
	usleep(100 * philo->number_of_philosophers);
	init_times(philo);
	pthread_mutex_unlock(&philo->start_mutex);
	while (!check_dead(philo))
		usleep(500);
	pthread_join(philo->death_monitor_thread, NULL);
	if (philo->num_must_eat != -1)
		pthread_join(philo->eat_monitor_thread, NULL);
	destroy_threads(philo, philo->number_of_philosophers, forks);
	return (0);
}
