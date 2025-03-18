/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pepie <pepie@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/17 07:51:42 by pepie             #+#    #+#             */
/*   Updated: 2025/03/18 03:05:49 by pepie            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "philo.h"

long long	timestamp(void)
{
	struct timeval	t;

	gettimeofday(&t, NULL);
	return ((t.tv_sec * 1000) + (t.tv_usec / 1000));
}

int	ft_usleep(int time, t_philo *philo)
{
	long long	start;
	long long	current;

	start = timestamp();
	current = timestamp();
	while (current - start < time)
	{
		if ((current - start) % 100 == 0)
			if (check_dead(philo))
				return (1);
		usleep(10);
		current = timestamp();
	}
	return (0);
}

void	*philo_think(void *philo)
{
	t_philo			*philo_gen;
	t_philosopher	*philosopher;

	philosopher = (t_philosopher *)philo;
	philo_gen = (t_philo *)philosopher->philo;
	pthread_mutex_lock(&philo_gen->start_mutex);
	pthread_mutex_unlock(&philo_gen->start_mutex);
	if (philo_gen->number_of_philosophers == 1)
	{
		pthread_mutex_lock(&philosopher->left_fork->mutex);
		print_philo(philosopher, "has taken a fork");
		ft_usleep(philo_gen->time_to_die, philo_gen);
		return (pthread_mutex_unlock(&philosopher->left_fork->mutex), NULL);
	}
	if (philosopher->id % 2 == 0)
		ft_usleep(philo_gen->time_to_eat, philo_gen);
	while (true)
	{
		if (check_dead(philo_gen))
			break ;
		try_take_fork(philosopher);
		start_sleep(philosopher);
		start_thinking(philosopher);
	}
	return (NULL);
}

void	destroy_threads(t_philo *philo, int count, t_fork **forks)
{
	int	i;

	i = -1;
	pthread_mutex_lock(&philo->game_over_mutex);
	philo->game_over = true;
	pthread_mutex_unlock(&philo->game_over_mutex);
	while (++i < count)
		pthread_join(philo->tid[i], NULL);
	i = 0;
	while (i < philo->number_of_philosophers)
	{
		pthread_mutex_destroy(&forks[i]->mutex);
		free(forks[i]);
		i++;
	}
	free(forks);
	free(philo->tid);
}

int	main(int argc, char **argv)
{
	t_philo	*philo;
	bool	have_eat_max;
	int		exit_code;

	if (argc < 5)
		return (printf("Wrong arg use ./philo [number_of_philosophers] [time_to_die] [time_to_eat] [time_to_sleep] [optional num_must_eat]\n"), 1);
	if (argc > 6)
		return (printf("Too many args use ./philo [number_of_philosophers] [time_to_die] [time_to_eat] [time_to_sleep] [optional num_must_eat]\n"), 1);
	philo = malloc(sizeof(t_philo));
	if (!philo)
		return (printf("Malloc failed\n"), 1);
	philo->number_of_philosophers = ft_atoi(argv[1]);
	philo->time_to_die = ft_atoi(argv[2]);
	philo->time_to_eat = ft_atoi(argv[3]);
	philo->time_to_sleep = ft_atoi(argv[4]);
	have_eat_max = false;
	if (argc == 6)
	{
		philo->num_must_eat = ft_atoi(argv[5]);
		have_eat_max = true;
	}
	else
		philo->num_must_eat = -1;
	if (philo->number_of_philosophers <= 0)
		return (printf("Error number_of_philosophers!\n"), free(philo), 1);
	if (philo->time_to_die <= 0)
		return (printf("Error time_to_die!\n"), free(philo), 1);
	if (philo->time_to_eat <= 0)
		return (printf("Error time_to_eat!\n"), free(philo), 1);
	if (philo->time_to_sleep <= 0)
		return (printf("Error time_to_sleep!\n"), free(philo), 1);
	if (philo->num_must_eat < 0 && have_eat_max)
	{
		return (printf("Error num_times_each_philo_must_eat!\n"), free(philo), 1);
	}
	exit_code = create_philosophers(philo);
	free(philo->philosophers);
	pthread_mutex_destroy(&philo->write_mutex);
	pthread_mutex_destroy(&philo->eating_mutex);
	pthread_mutex_destroy(&philo->game_over_mutex);
	pthread_mutex_destroy(&philo->start_mutex);
	free(philo);
	return (exit_code);
}
