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
	if (philo_gen->number_of_times_each_philosopher_must_eat == -1)
		return (false);
	game_over = true;
	pthread_mutex_lock(&philo_gen->num_eaten_mutex); 
	while (i < philo_gen->number_of_philosophers)
	{
		if (philo_gen->num_eaten[i] < philo_gen->number_of_times_each_philosopher_must_eat)
		{
			game_over = false;
			break;
		}
		i++;
	}
	pthread_mutex_unlock(&philo_gen->num_eaten_mutex); 
	return (game_over);
}

void	kill_philosopher(t_philosopher *philosopher, t_philo *philo_gen, unsigned long time, bool show_dead)
{
    int i;

	pthread_mutex_lock(&philo_gen->game_over_mutex);
    if (philo_gen->game_over) {
        pthread_mutex_unlock(&philo_gen->game_over_mutex);
        return;
    }
	philo_gen->game_over = true;
    pthread_mutex_unlock(&philo_gen->game_over_mutex);
	pthread_mutex_lock(&philosopher->dead);
    if (philosopher->game_over) {
        pthread_mutex_unlock(&philosopher->dead);
        return;
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

int	start_eat(t_philosopher *philosopher, unsigned long time)
{
	t_philo			*philo_gen;

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
	pthread_mutex_lock(&philosopher->left_fork->mutex);
    philosopher->left_fork->user_id = -1;
	pthread_mutex_unlock(&philosopher->left_fork->mutex);
    pthread_mutex_lock(&philosopher->right_fork->mutex);
	philosopher->right_fork->user_id = -1;
	pthread_mutex_unlock(&philosopher->right_fork->mutex);
	printf("%lld %d is sleeping\n", time - philo_gen->time_start, philosopher->id);
	philosopher->status = SLEEPING;
	philosopher->time_act_start = time;
	philosopher->time_act_end = time + philo_gen->time_to_sleep;
	philosopher->last_time_eat = time;
	philosopher->eaten_count += 1;
	pthread_mutex_lock(&philo_gen->num_eaten_mutex);
	philo_gen->num_eaten[philosopher->id - 1] += 1;
	pthread_mutex_unlock(&philo_gen->num_eaten_mutex);
	if (check_game_over(philo_gen))
	{
		kill_philosopher(philosopher, philo_gen, time, false);
		return (0);
	}
	return (0);
}

long long	timestamp(void)
{
	struct timeval	t;

	gettimeofday(&t, NULL);
	return ((t.tv_sec * 1000) + (t.tv_usec / 1000));
}

int	take_fork(t_philosopher *philosopher, t_fork *fork)
{
	if (fork->user_id != -1)
		return (1);
	fork->user_id = philosopher->id;
	return (0);
}

bool	try_take_fork(t_philosopher	*philosopher, long long time)
{
	t_philo			*philo_gen;
    t_fork			*first_fork;
    t_fork			*second_fork;

	philo_gen = (t_philo *)philosopher->philo;
	if (philosopher->left_fork->id < philosopher->right_fork->id)
	{
        first_fork = philosopher->left_fork;
        second_fork = philosopher->right_fork;
    } else {
        first_fork = philosopher->right_fork;
        second_fork = philosopher->left_fork;
    }
    pthread_mutex_lock(&first_fork->mutex);
    if (take_fork(philosopher, first_fork))
        return (pthread_mutex_unlock(&first_fork->mutex), 0);
    pthread_mutex_unlock(&first_fork->mutex);
	if (&philosopher->left_fork->mutex == &philosopher->right_fork->mutex)
	{
		return (printf("%lld %d has taken a fork\n", time - philo_gen->time_start, philosopher->id), 0);
	}
    pthread_mutex_lock(&second_fork->mutex);
    if (take_fork(philosopher, second_fork))
    {
        pthread_mutex_lock(&first_fork->mutex);
        first_fork->user_id = -1;
        pthread_mutex_unlock(&first_fork->mutex);
        return (pthread_mutex_unlock(&second_fork->mutex), 0);
    }
    pthread_mutex_unlock(&second_fork->mutex);
    start_eat(philosopher, time);
	return (0);
}

int	ft_usleep(int time)
{
	long long	start;

	start = timestamp();
	while ((timestamp() - start) < time)
		usleep(time / 10);
	return (0);
}

void *philo_think(void *philo)
{
	long long	 	time;
	t_philo			*philo_gen;
	t_philosopher	*philosopher;

	philosopher = (t_philosopher *)philo;
	philo_gen = (t_philo *)philosopher->philo;
	while (true)
	{
        pthread_mutex_lock(&philosopher->dead);
        if (philosopher->game_over) {
            pthread_mutex_unlock(&philosopher->dead);
            break;
        }
        pthread_mutex_unlock(&philosopher->dead);
		time = timestamp();
		if (time - philosopher->last_time_eat >= philo_gen->time_to_die) 
		{
			kill_philosopher(philosopher, philo_gen, time, true);
			break;
		}
		else if (philosopher->time_act_end == 0 || (philosopher->time_act_end != 0 && time >= philosopher->time_act_end))
		{
			if (philosopher->status == THINKING)
                try_take_fork(philosopher, time);
			else if (philosopher->status == EATING)
				start_sleep(philosopher, time);
			else if (philosopher->status == SLEEPING)
				start_thinking(philosopher, time);
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