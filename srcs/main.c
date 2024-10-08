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
	
	/* printf("left_fork->user_id:\t%d\t%d\n", philosopher->left_fork->user_id, philosopher->left_fork->id);
	printf("right_fork->user_id:\t%d\t%d\n", philosopher->right_fork->user_id, philosopher->right_fork->id); */
	if (philosopher->left_fork->user_id != philosopher->id || philosopher->right_fork->user_id != philosopher->id)
		return (1);
	philo_gen = (t_philo *)philosopher->philo;
	printf("%lld %d has taken a fork\n", time - philo_gen->time_start, philosopher->id);
	printf("%lld %d has taken a fork\n", time - philo_gen->time_start, philosopher->id);
	printf("%lld %d is eating\n", time - philo_gen->time_start, philosopher->id);
	philosopher->status = EATING;
	philosopher->time_act_start = time;
	philosopher->time_act_end = time + philo_gen->time_to_eat;
	return (0);
}

int	start_thinking(t_philosopher *philosopher, unsigned long time)
{
	t_philo			*philo_gen;
	
	philo_gen = (t_philo *)philosopher->philo;
	printf("%lld %d is thinking\n", time - philo_gen->time_start, philosopher->id);
	philosopher->status = THINKING;
	philosopher->time_act_start = time;
	philosopher->time_act_end = time + philo_gen->time_to_sleep;
	return (0);
}

int start_sleep(t_philosopher *philosopher, unsigned long time)
{
	t_philo			*philo_gen;
	
	philo_gen = (t_philo *)philosopher->philo;
	/* printf("left_fork->user_id:\t%d\t%d\n", philosopher->left_fork->user_id, philosopher->left_fork->id);
	printf("right_fork->user_id:\t%d\t%d\n", philosopher->right_fork->user_id, philosopher->right_fork->id); */
	philosopher->left_fork->user_id = -1;
	philosopher->right_fork->user_id = -1;
	pthread_mutex_unlock(&philosopher->left_fork->mutex);
	pthread_mutex_unlock(&philosopher->right_fork->mutex);
	printf("%lld %d is sleeping\n", time - philo_gen->time_start, philosopher->id);
	philosopher->status = SLEEPING;
	philosopher->time_act_start = time;
	philosopher->time_act_end = time + philo_gen->time_to_sleep;
	philosopher->last_time_eat = time;
	philosopher->eaten_count += 1;
	return (0);
}

void	kill_philosopher(t_philosopher *philosopher, t_philo *philo_gen, unsigned long time)
{
	//int		i;

	philosopher->status = DEAD;
	philo_gen->game_over = true;
	printf("%lld %d died\n", time - philo_gen->time_start, philosopher->id);
/* 	i = 0;
	while (i < philo_gen->number_of_philosophers)
	{
		pthread_mutex_destroy(&philo_gen->forks[i]->mutex);
		free(philo_gen->forks[i]);
		//pthread_detach(philo_gen->philosophers[i].thread);
		i++;
	}
	free(philo_gen->forks);
	free(philo_gen->philosophers);
	free(philo_gen); */
	//exit(0);
}

long long	timestamp(void)
{
	struct timeval	t;

	gettimeofday(&t, NULL);
	return ((t.tv_sec * 1000) + (t.tv_usec / 1000));
}

int	take_fork(t_philosopher *philosopher, int id_fork, bool is_left)
{
	t_philo			*philo_gen;
	t_fork			*fork;

	philo_gen = (t_philo *)philosopher->philo;
	fork = philo_gen->forks[id_fork - 1];
	if (fork->user_id != -1)
		return (1);
	pthread_mutex_lock(&fork->mutex);
	fork->user_id = philosopher->id;
	if (is_left)
		philosopher->left_fork = fork;
	else
		philosopher->right_fork = fork;
	return (0);
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
	//printf("%ld %d is thinking\n", time.tv_usec - philo_gen->time_start, philosopher->id);
	while (1)
	{
		if (philo_gen->game_over)
			break;
		time = timestamp();
		if (time - philosopher->last_time_eat >= philo_gen->time_to_die || (philo_gen->number_of_times_each_philosopher_must_eat != -1 && philosopher->eaten_count >= philo_gen->number_of_times_each_philosopher_must_eat))
		{
			//printf("P* %d\n", philosopher->id);
			kill_philosopher(philosopher, philo_gen, time);
			break;
		}
		else if (philosopher->time_act_end == 0 || (philosopher->time_act_end != 0 && time >= philosopher->time_act_end))
		{
			if (philosopher->status == THINKING)
			{
				/* if (philosopher->id == 2)
				{
					printf("left_fork->user_id:\t%d\t%d\n", philosopher->left_fork->user_id, philosopher->left_fork->id);
					printf("right_fork->user_id:\t%d\t%d\n", philosopher->right_fork->user_id, philosopher->right_fork->id);
				} */
				//printf("$* %d\n", philosopher->id);
				if (philosopher->left_fork->user_id == -1 && philosopher->right_fork->user_id == -1)
				{
					if (take_fork(philosopher, philosopher->left_fork->id, true))
						continue;
					if (take_fork(philosopher, philosopher->right_fork->id, false))
					{
						philosopher->left_fork->user_id = -1;
						pthread_mutex_unlock(&philosopher->left_fork->mutex);
						continue;
					}
					start_eat(philosopher, time);
				}
			}
			//printf("& %d\n", philosopher->id);
			else if (philosopher->status == EATING)
			{
				start_sleep(philosopher, time);
			}
			else if (philosopher->status == SLEEPING)
			{
				start_thinking(philosopher, time);
			}
		}
	}
	return (NULL);
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
		//printf("i: %d\n", i);
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
	philo->time_start = 0;
	philo->game_over = false;
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
		pthread_mutex_init(&philosophers[i].mutex, NULL);
		/* printf("i: %d\n", i);
		printf("left\t%d\n", i);
		printf("right\t%d %d\n", i-1, philo->number_of_philosophers-1); */
		if (i == 0) {
			philosophers[i].right_fork = forks[philo->number_of_philosophers-1];
			//printf("right_fork->user_id:\t%d\t%d\n", i, philo->number_of_philosophers-1);
		}
		else
		{
			philosophers[i].right_fork = forks[i-1];
			//printf("right_fork->user_id:\t%d\t%d\n", i, i-1);
		}
		philosophers[i].left_fork = forks[i];
		//printf("left_fork->user_id:\t%d\t%d\n", i, i);
		if (philo->time_start == 0)
			philo->time_start = time;
		/* if (i == 0)
		{
			take_fork(&philosophers[i], 0, time, true);
			take_fork(&philosophers[i], 3, time, false);
			start_eat(&philosophers[i], time);
		} */
		pthread_create(&philosophers[i].thread, NULL, philo_think, &philosophers[i]);
		i++;
	}
	philo->philosophers = philosophers;
	//philosophers[0].right_fork = forks[philo->number_of_philosophers - 1];
	i = 0;
	while (i < philo->number_of_philosophers)
	{
		pthread_join(philosophers[i].thread, NULL);
		i++;
	}
	i = 0;
	while (i < philo->number_of_philosophers)
	{
		pthread_mutex_destroy(&forks[i]->mutex);
		free(forks[i]);
		i++;
		//pthread_mutex_destroy(&philosophers[i].mutex);
	}
	free(forks);
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
	else
		philo->number_of_times_each_philosopher_must_eat = -1;
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