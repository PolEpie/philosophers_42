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

long long	timestamp(void)
{
	struct timeval	t;

	gettimeofday(&t, NULL);
	return ((t.tv_sec * 1000) + (t.tv_usec / 1000));
}

int ft_usleep(int time, t_philo *philo)
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
		usleep(100);
		current = timestamp();
	}
	return (0);
}

void	print_philo(t_philosopher *philosopher, char *str)
{
	t_philo			*philo_gen;

	philo_gen = (t_philo *)philosopher->philo;
	pthread_mutex_lock(&philo_gen->write_mutex);
	if (!check_dead(philo_gen))
		printf("%llu %d %s\n", timestamp() - philo_gen->time_start, philosopher->id, str);
	pthread_mutex_unlock(&philo_gen->write_mutex);
}

int start_eat(t_philosopher *philosopher, unsigned long time)
{
	t_philo *philo_gen;

	philo_gen = (t_philo *)philosopher->philo;
	
	// Set status and timing atomically
	pthread_mutex_lock(&philo_gen->eating_mutex);
	philosopher->is_eating = true;
	philosopher->last_time_eat = time;  // Set to start of eating time
	philosopher->eaten_count += 1;
	pthread_mutex_unlock(&philo_gen->eating_mutex);
	print_philo(philosopher, "is eating");
	// Sleep for eating duration
	if (ft_usleep(philo_gen->time_to_eat, philo_gen))
		return (1);
	pthread_mutex_lock(&philo_gen->eating_mutex);
	philosopher->is_eating = false;
	pthread_mutex_unlock(&philo_gen->eating_mutex);
	return (0);
}

int	start_thinking(t_philosopher *philosopher)
{
	print_philo(philosopher, "is thinking");
	if (philosopher->time_think > 0)
		ft_usleep(philosopher->time_think, philosopher->philo);
	return (0);
}

int start_sleep(t_philosopher *philosopher)
{
	t_philo          *philo_gen;
	
	philo_gen = (t_philo *)philosopher->philo;
	print_philo(philosopher, "is sleeping");
	ft_usleep(philo_gen->time_to_sleep, philo_gen);
	return (0);
}

int	take_fork(t_philosopher *philosopher, t_fork *fork)
{
	if (fork->user_id != -1)
		return (1);
	fork->user_id = philosopher->id;
	return (0);
}

// Completely rewrite the try_take_fork function to match your friend's simpler approach
void try_take_fork(t_philosopher *philosopher)
{
    pthread_mutex_t *first_fork;
    pthread_mutex_t *second_fork;

    // Even philosophers take right fork first, odd take left fork first
    if (philosopher->id % 2 != 0)
    {
        first_fork = &philosopher->right_fork->mutex;
        second_fork = &philosopher->left_fork->mutex;
    }
    else
    {
        first_fork = &philosopher->left_fork->mutex;
        second_fork = &philosopher->right_fork->mutex;
    }

    // Take the first fork and don't proceed if simulation has ended
    pthread_mutex_lock(first_fork);
    print_philo(philosopher, "has taken a fork");
    pthread_mutex_lock(second_fork);
    print_philo(philosopher, "has taken a fork");

    // Eat
    start_eat(philosopher, timestamp());

    // Release forks after eating
    pthread_mutex_unlock(first_fork);
    pthread_mutex_unlock(second_fork);
}


void *philo_think(void *philo)
{
    t_philo          *philo_gen;
    t_philosopher    *philosopher;

    philosopher = (t_philosopher *)philo;
    philo_gen = (t_philo *)philosopher->philo;
    
    // Special case for single philosopher
    if (philo_gen->number_of_philosophers == 1) {
        pthread_mutex_lock(&philosopher->left_fork->mutex);
        print_philo(philosopher, "has taken a fork");
        ft_usleep(philo_gen->time_to_die + 10, philo_gen);
        pthread_mutex_unlock(&philosopher->left_fork->mutex);
        return NULL;
    }

    // Stagger start times to prevent deadlock
    if (philosopher->id % 2 == 0)
        ft_usleep(philo_gen->time_to_eat, philo_gen);  // Shorter delay
    
    while (true)
    {
        if (check_dead(philo_gen))
            break;
		try_take_fork(philosopher);
		start_sleep(philosopher);
		start_thinking(philosopher);
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
	return (false);
}

int	create_philosophers(t_philo *philo)
{
	t_philosopher   *philosophers;
	t_fork          **forks;
	long long       time;
	int             i;
	pthread_t       death_monitor_thread;
	pthread_t       eat_monitor_thread;

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
	pthread_mutex_init(&philo->write_mutex, NULL);
	pthread_mutex_init(&philo->eating_mutex, NULL);
	while (i < philo->number_of_philosophers)last_time_eat
	{
		philosophers[i].time_think = (philo->time_to_eat * ((i % 2) + 1)) - philo->time_to_sleep;
		philosophers[i].id = i + 1;
		philosophers[i].eaten_count = 0;
		philosophers[i].philo = philo;
		philosophers[i].left_fork = forks[i];
		philosophers[i].right_fork = forks[i == 0 ? philo->number_of_philosophers - 1 : i - 1];
		philosophers[i].last_time_eat = time;  // Initialize last_time_eat to start time
		i++;
	}

	// Create philosopher threads first
	i = 0;
	while (i < philo->number_of_philosophers)
	{
		if (pthread_create(&philo->tid[i], NULL, philo_think, &philosophers[i]))
		{
			return (printf("Error creating philosopher thread\n"), 1);
		}
		i++;
	}

	// Create monitor threads
	if (pthread_create(&death_monitor_thread, NULL, death_monitor, philo))
		return (printf("Error creating death monitor thread\n"), 1);
		
	// Only create eat monitor if there's a eat limit
	if (philo->number_of_times_each_philosopher_must_eat != -1)
	{
		if (pthread_create(&eat_monitor_thread, NULL, monitor_eat, philo))
			return (printf("Error creating eat monitor thread\n"), 1);
	}

	// Wait for philosopher threads
	i = 0;
	while (i < philo->number_of_philosophers)
	{
		pthread_join(philo->tid[i], NULL);
		i++;
	}

	// Wait for monitor threads
	pthread_join(death_monitor_thread, NULL);
	if (philo->number_of_times_each_philosopher_must_eat != -1)
		pthread_join(eat_monitor_thread, NULL);

	// Cleanup mutexes and memory
	i = 0;
	while (i < philo->number_of_philosophers)
	{
		pthread_mutex_destroy(&forks[i]->mutex);
		free(forks[i]);
		i++;
	}
	free(forks);
	free(philo->tid);
	return (0);
}


int main(int argc, char **argv)
{
	t_philo *philo;
	bool	have_eat_max;

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
	have_eat_max = false;
	if (argc == 6)
	{
		philo->number_of_times_each_philosopher_must_eat = ft_atoi(argv[5]);
		have_eat_max = true;
	}
	else
		philo->number_of_times_each_philosopher_must_eat = -1;
	if (philo->number_of_philosophers <= 0)
		return (printf("Error number_of_philosophers!\n"), free(philo), 1);
	if (philo->time_to_die <= 0)
		return (printf("Error time_to_die!\n"), free(philo), 1);
	if (philo->time_to_eat <= 0)
		return (printf("Error time_to_eat!\n"), free(philo), 1);
	if (philo->time_to_sleep <= 0)
		return (printf("Error time_to_sleep!\n"), free(philo), 1);
	if (philo->number_of_times_each_philosopher_must_eat < 0 && have_eat_max)
	{
		return (printf("Error num_times_each_philo_must_eat!\n"), free(philo), 1);
	}
	create_philosophers(philo);
	free(philo->philosophers);
	pthread_mutex_destroy(&philo->write_mutex);
	pthread_mutex_destroy(&philo->eating_mutex);
	free(philo);

	return (0);
}