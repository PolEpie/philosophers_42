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


int	start_eat(t_philosopher *philosopher, unsigned long time)
{
	t_philo			*philo_gen;
	
	philo_gen = (t_philo *)philosopher->philo;
	printf("%lld [%d] is eating\n", time - philo_gen->time_start, philosopher->id);
	philosopher->status = EATING;
	philosopher->time_act_start = time;
	philosopher->time_act_end = time + philo_gen->time_to_sleep;
	return (0);
}

int start_sleep(t_philosopher *philosopher, unsigned long time)
{
	t_philo			*philo_gen;
	
	philo_gen = (t_philo *)philosopher->philo;
	pthread_mutex_unlock(&philosopher->left_fork->mutex);
	pthread_mutex_unlock(&philosopher->right_fork->mutex);
	philosopher->left_fork->user_id = -1;
	philosopher->right_fork->user_id = -1;
	printf("%lld [%d] is sleeping\n", time - philo_gen->time_start, philosopher->id);
	philosopher->status = SLEEPING;
	philosopher->time_act_start = time;
	philosopher->time_act_end = time + philo_gen->time_to_sleep;
	return (0);
}

void	kill_philosopher(t_philosopher *philosopher, unsigned long time)
{
	t_philo	*philo_gen;

	philo_gen = (t_philo *)philosopher->philo;
	philosopher->status = DEAD;
	printf("%lld [%d] died\n", time - philo_gen->time_start, philosopher->id);
}


long long	timestamp(void)
{
	struct timeval	t;

	gettimeofday(&t, NULL);
	return ((t.tv_sec * 1000) + (t.tv_usec / 1000));
}

void *philo_think(void *philo)
{
	//pthread_t		tid;
	long long	 	time;
	t_philo			*philo_gen;
	t_philosopher	*philosopher;

	//tid = pthread_self();
	//printf("time_at: %ld\n", time.tv_usec);
	philosopher = (t_philosopher *)philo;
	philo_gen = (t_philo *)philosopher->philo;
	//printf("%ld [%d] is thinking\n", time.tv_usec - philo_gen->time_start, philosopher->id);
	while (1)
	{
		time = timestamp();


		if (philosopher->time_act_end != 0 && time >= philosopher->time_act_end)
		{
			if (philosopher->status == EATING)
			{
				start_sleep(philosopher, time);
				philosopher->last_time_eat = time;
			}
			else if (philosopher->status == SLEEPING)
			{
				start_eat(philosopher, time);
			}
		}
		else if (time - philosopher->last_time_eat >= philo_gen->time_to_die)
		{
			kill_philosopher(philosopher, time);
			break;
		}
	}
	return (NULL);
}

int	take_fork(t_philosopher *philosopher, int id_fork, unsigned long time, bool is_left)
{
	t_philo			*philo_gen;
	t_fork			*fork;

	philo_gen = (t_philo *)philosopher->philo;
	fork = philo_gen->forks[id_fork];
	if (fork->user_id != -1)
		return (1);
	pthread_mutex_lock(&fork->mutex);
	fork->user_id = philosopher->id;
	if (is_left)
		philosopher->left_fork = fork;
	else
		philosopher->right_fork = fork;
	printf("%lld [%d] has taken a fork\n", time - philo_gen->time_start, philosopher->id);
	if (is_left && philosopher->right_fork->user_id == -1)
	{
		pthread_mutex_lock(&philosopher->right_fork->mutex);
		philosopher->right_fork->user_id = philosopher->id;
		printf("%lld [%d] has taken a fork\n", time - philo_gen->time_start, philosopher->id);
	}
	return (0);
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
		forks[i]->id = i;
		pthread_mutex_init(&forks[i]->mutex, NULL);
		i++;
	}
	philo->forks = forks;
	return (forks);
}



int	create_philosophers(t_philo *philo)
{
	t_philosopher	*philosophers;
	t_fork			**forks;
	long long		time;
	int				i;

	philosophers = malloc(sizeof(t_philosopher) * philo->number_of_philosophers);
	if (!philosophers)
		return (printf("Malloc failed\n"), 1);
	forks = create_forks(philo);
	if (forks == NULL)
		return (printf("Malloc failed\n"), 1);
	i = 0;
	philo->forks = forks;
	philo->time_start = 0;
	while (i < philo->number_of_philosophers)
	{
		time = timestamp();
		philosophers[i].status = THINKING;
		philosophers[i].time_act_start = time;
		philosophers[i].id = i + 1;
		philosophers[i].time_act_end = 0;
		philosophers[i].last_time_eat = time;
		philosophers[i].philo = philo;
		pthread_mutex_init(&philosophers[i].mutex, NULL);
		if (i == 0)
			philosophers[i].left_fork = NULL;
		else
			philosophers[i].left_fork = forks[i-1];
		philosophers[i].right_fork = forks[i];
		if (philo->time_start == 0)
			philo->time_start = time;
		if (i == 0)
		{
			take_fork(&philosophers[i], 0, time, true);
			take_fork(&philosophers[i], 1, time, false);
			start_eat(&philosophers[i], time);
		}
		pthread_create(&philosophers[i].thread, NULL, philo_think, &philosophers[i]);
		i++;
	}
	philosophers[0].right_fork = forks[philo->number_of_philosophers - 1];

	i = 0;
	while (i < philo->number_of_philosophers)
	{
		pthread_join(philosophers[i].thread, NULL);
		i++;
	}
	philo->philosophers = philosophers;
	return (0);
}


int main(int argc, char **argv)
{
	t_philo *philo;

	if (argc < 5)
    	return (printf("Wrong arg use ./philo [number_of_philosophers] [time_to_die] [time_to_eat] [time_to_sleep] [optional number_of_times_each_philosopher_must_eat]\n"), 1);
	if (argc > 6)
		return (printf("Too many args use ./philo [number_of_philosophers] [time_to_die] [time_to_eat] [time_to_sleep] [optional number_of_times_each_philosopher_must_eat]\n"), 1);
	philo = malloc(sizeof(t_philo));
	if (!philo)
		return (printf("Malloc failed\n"), 1);
	philo->number_of_philosophers = ft_atoi(argv[1]);
	philo->time_to_die = ft_atoi(argv[2]);
	philo->time_to_eat = ft_atoi(argv[3]);
	philo->time_to_sleep = ft_atoi(argv[4]);
	if (argc == 6)
		philo->number_of_times_each_philosopher_must_eat = ft_atoi(argv[5]);
	printf("number_of_philosophers: %d\n", philo->number_of_philosophers);
	printf("time_to_die: %llu\n", philo->time_to_die);
	printf("time_to_eat: %llu\n", philo->time_to_eat);
	printf("time_to_sleep: %d\n", philo->time_to_sleep);
	if (philo->number_of_times_each_philosopher_must_eat)
		printf("number_of_times_each_philosopher_must_eat: %d\n", philo->number_of_times_each_philosopher_must_eat);
	create_philosophers(philo);
	free(philo->philosophers);
	free(philo);
	return (0);
}