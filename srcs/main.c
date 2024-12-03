/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pepie <pepie@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/17 07:51:42 by pepie             #+#    #+#             */
/*   Updated: 2024/05/10 13:14:40 by pepie            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "philo.h"
#include <unistd.h>

bool	check_game_over(t_philo *philo_gen)
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

void	kill_philosopher(t_philosopher *philosopher, t_philo *philo_gen, unsigned long time, bool show_dead)
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

bool    create_mutex(t_philo *philo)
{
	philo->tid = malloc(sizeof(pthread_t) * philo->number_of_philosophers);
	if (!philo->tid)
		return (true);
	philo->philo_mutex = malloc(sizeof(pthread_mutex_t) * philo->number_of_philosophers);
	if (!philo->philo_mutex)
		return (true);
	return (false);
}

void	init_num_eaten(t_philo *philo)
{
	int	i;

	i = 0;
	while (i < philo->number_of_philosophers)
	{
		philo->num_eaten[i] = 0;
		i++;
	}
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
	philo->time_start = 0;
	philo->philosophers = philosophers;
	philo->num_eaten = malloc(sizeof(int) * philo->number_of_philosophers);
	philo->game_over = false;
	if (!philo->num_eaten)
		return (printf("Malloc failed\n"), 1);
	init_num_eaten(philo);
	pthread_mutex_init(&philo->num_eaten_mutex, NULL);
	pthread_mutex_init(&philo->game_over_mutex, NULL);
	while (i < philo->number_of_philosophers)
	{
		time = timestamp();
		philosophers[i].status = THINKING;
		philosophers[i].time_act_start = time;
		philosophers[i].id = i + 1;
		philosophers[i].time_act_end = 0;
		philosophers[i].last_time_eat = time;
		philosophers[i].eaten_count = 0;
		philosophers[i].philo = philo;
		philosophers[i].game_over = false;
		pthread_mutex_init(&philosophers[i].dead, NULL);
		pthread_mutex_init(&philosophers[i].write_mutex, NULL);
		if (i == 0)
			philosophers[i].right_fork = forks[philo->number_of_philosophers-1];
		else
			philosophers[i].right_fork = forks[i-1];
		philosophers[i].left_fork = forks[i];
		if (philo->time_start == 0)
			philo->time_start = time;
		i++;
	}
	i = 0;
	while (i < philo->number_of_philosophers)
	{
		pthread_create(&philo->tid[i], NULL, philo_think, &philosophers[i]);
		i++;
	}
	i = 0;
	while (i < philo->number_of_philosophers)
	{
		if (pthread_join(philo->tid[i], NULL))
			return (printf("Error join\n"), 1);
		i++;
	}
	i = 0;
	while (i < philo->number_of_philosophers)
	{
		pthread_mutex_destroy(&philosophers[i].dead);
		pthread_mutex_destroy(&philosophers[i].write_mutex);
		pthread_mutex_destroy(&forks[i]->mutex);
		free(forks[i]);
		i++;
	}
	free(forks);
	free(philo->num_eaten);
	free(philo->tid);
	free(philo->philo_mutex);
	return (0);
}


int main(int argc, char **argv)
{
	t_philo *philo;

	if (argc < 5)
		return (printf("Wrong arg use ./philo [number_of_philosophers] [time_to_die] [time_to_eat] [time_to_sleep] [optional num_eat_max]\n"), 1);
	if (argc > 6)
		return (printf("Too many args use ./philo [number_of_philosophers] [time_to_die] [time_to_eat] [time_to_sleep] [optional num_eat_max]\n"), 1);
	philo = malloc(sizeof(t_philo));
	if (!philo)
		return (printf("Malloc failed\n"), 1);
	philo->number_of_philosophers = ft_atoi(argv[1]);
	philo->time_to_die = ft_atoi(argv[2]);
	philo->time_to_eat = ft_atoi(argv[3]);
	philo->time_to_sleep = ft_atoi(argv[4]);
	if (argc == 6)
		philo->num_eat_max = ft_atoi(argv[5]);
	else
		philo->num_eat_max = -1;
	if (philo->number_of_philosophers <= 0)
		return (printf("Error number_of_philosophers!\n"), free(philo), 1);
	if (philo->time_to_die <= 0)
		return (printf("Error time_to_die!\n"), free(philo), 1);
	if (philo->time_to_eat <= 0)
		return (printf("Error time_to_eat!\n"), free(philo), 1);
	if (philo->time_to_sleep <= 0)
	{
		return (printf("Error time_to_sleep!\n"), free(philo), 1);
	}
	create_philosophers(philo);
	free(philo->philosophers);
	free(philo);
	return (0);
}