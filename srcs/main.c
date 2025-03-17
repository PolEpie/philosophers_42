/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pepie <pepie@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/17 07:51:42 by pepie             #+#    #+#             */
/*   Updated: 2025/03/17 16:52:28 by pepie            ###   ########.fr       */
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

bool	check_dead(t_philosopher *philosopher)
{
	pthread_mutex_lock(&philosopher->dead);
	if (philosopher->game_over) {
		pthread_mutex_unlock(&philosopher->dead);
		return (true);
	}
	pthread_mutex_unlock(&philosopher->dead);
	
	t_philo *philo_gen = philosopher->philo;
	pthread_mutex_lock(&philo_gen->game_over_mutex);
	bool is_game_over = philo_gen->game_over;
	pthread_mutex_unlock(&philo_gen->game_over_mutex);
	
	return (is_game_over);
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
	philosopher->game_over = true;
	pthread_mutex_unlock(&philosopher->dead);
	
	pthread_mutex_lock(&philosopher->status_mutex);
	philosopher->status = DEAD;
	pthread_mutex_unlock(&philosopher->status_mutex);
	
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
	
	if (show_dead) {
		pthread_mutex_lock(&philo_gen->write_mutex);
		printf("%lld %d died\n", time - philo_gen->time_start, philosopher->id);
		pthread_mutex_unlock(&philo_gen->write_mutex);
	}
}

long long	timestamp(void)
{
	struct timeval	t;

	gettimeofday(&t, NULL);
	return ((t.tv_sec * 1000) + (t.tv_usec / 1000));
}

int	ft_usleep(int time)
{
	long long	start;
	long long	current;

	start = timestamp();
	while (1)
	{
		current = timestamp();
		if ((current - start) >= time)
			break;
		usleep(10);  // Reduced sleep time for more precision
	}
	return (0);
}

void	print_philo(t_philosopher *philosopher, char *str)
{
	t_philo			*philo_gen;

	philo_gen = (t_philo *)philosopher->philo;
	pthread_mutex_lock(&philo_gen->write_mutex);
	if (!check_dead(philosopher))
		printf("%llu %d %s\n", timestamp() - philo_gen->time_start, philosopher->id, str);
	pthread_mutex_unlock(&philo_gen->write_mutex);
}

int start_eat(t_philosopher *philosopher, unsigned long time)
{
	t_philo *philo_gen;

	philo_gen = (t_philo *)philosopher->philo;
	
	// Set status and timing atomically
	pthread_mutex_lock(&philosopher->status_mutex);
	pthread_mutex_lock(&philosopher->timing_mutex);
	philosopher->status = EATING;
	philosopher->time_act_start = time;
	philosopher->time_act_end = time + philo_gen->time_to_eat;
	philosopher->last_time_eat = time;  // Set to start of eating time
	pthread_mutex_unlock(&philosopher->timing_mutex);
	pthread_mutex_unlock(&philosopher->status_mutex);
	
	print_philo(philosopher, "is eating");
	
	// Sleep for eating duration
	ft_usleep(philo_gen->time_to_eat);

	// Update eaten count
	pthread_mutex_lock(&philo_gen->num_eaten_mutex);
	philo_gen->num_eaten[philosopher->id - 1] += 1;
	bool should_stop = (philo_gen->number_of_times_each_philosopher_must_eat != -1 &&
					   philo_gen->num_eaten[philosopher->id - 1] >= philo_gen->number_of_times_each_philosopher_must_eat);
	pthread_mutex_unlock(&philo_gen->num_eaten_mutex);

	// Check if all philosophers have eaten enough
	if (should_stop && check_game_over(philo_gen))
	{
		kill_philosopher(philosopher, philo_gen, timestamp(), false);
		return (1);
	}
	return (0);
}

int	start_thinking(t_philosopher *philosopher, unsigned long time)
{
	print_philo(philosopher, "is thinking");
	
	pthread_mutex_lock(&philosopher->status_mutex);
	philosopher->status = THINKING;
	pthread_mutex_unlock(&philosopher->status_mutex);
	
	pthread_mutex_lock(&philosopher->timing_mutex);
	philosopher->time_act_start = time;
	philosopher->time_act_end = time + 1;
	pthread_mutex_unlock(&philosopher->timing_mutex);
	
	// Add a small random delay (1-5ms) to break perfect synchronization
	ft_usleep((philosopher->id * 3) % 5 + 1);
	return (0);
}

int start_sleep(t_philosopher *philosopher, unsigned long time)
{
	t_philo			*philo_gen;
	
	philo_gen = (t_philo *)philosopher->philo;
	
	// Update timing information first
	pthread_mutex_lock(&philosopher->timing_mutex);
	philosopher->time_act_start = time;
	philosopher->time_act_end = time + philo_gen->time_to_sleep;
	pthread_mutex_unlock(&philosopher->timing_mutex);
	
	// Release forks
	pthread_mutex_lock(&philosopher->left_fork->mutex);
	philosopher->left_fork->user_id = -1;
	pthread_mutex_unlock(&philosopher->left_fork->mutex);
	pthread_mutex_lock(&philosopher->right_fork->mutex);
	philosopher->right_fork->user_id = -1;
	pthread_mutex_unlock(&philosopher->right_fork->mutex);
	
	// Update status
	pthread_mutex_lock(&philosopher->status_mutex);
	philosopher->status = SLEEPING;
	pthread_mutex_unlock(&philosopher->status_mutex);
	
	print_philo(philosopher, "is sleeping");
	
	// Check if this philosopher has eaten enough
	bool has_eaten_enough = false;
	pthread_mutex_lock(&philo_gen->num_eaten_mutex);
	if (philo_gen->number_of_times_each_philosopher_must_eat != -1 &&
		philo_gen->num_eaten[philosopher->id - 1] >= philo_gen->number_of_times_each_philosopher_must_eat)
	{
		has_eaten_enough = true;
	}
	pthread_mutex_unlock(&philo_gen->num_eaten_mutex);
	
	ft_usleep(philo_gen->time_to_sleep);
	return (has_eaten_enough ? 1 : 0);
}

int	take_fork(t_philosopher *philosopher, t_fork *fork)
{
	if (fork->user_id != -1)
		return (1);
	fork->user_id = philosopher->id;
	return (0);
}

bool check_philosopher_dead(t_philosopher *philosopher, t_philo *philo_gen, long long current_time)
{
	long long last_eat;
	t_status current_status;
	
	// Get both status and timing atomically
	pthread_mutex_lock(&philosopher->status_mutex);
	pthread_mutex_lock(&philosopher->timing_mutex);
	current_status = philosopher->status;
	last_eat = philosopher->last_time_eat;
	pthread_mutex_unlock(&philosopher->timing_mutex);
	pthread_mutex_unlock(&philosopher->status_mutex);

	// If they're eating, they can't die
	if (current_status == EATING)
		return false;

	// They die if time since last meal start exceeds time_to_die
	if ((current_time - last_eat) > philo_gen->time_to_die)
	{
		pthread_mutex_lock(&philo_gen->game_over_mutex);
		if (!philo_gen->game_over)
		{
			pthread_mutex_unlock(&philo_gen->game_over_mutex);
			kill_philosopher(philosopher, philo_gen, current_time, true);
			return true;
		}
		pthread_mutex_unlock(&philo_gen->game_over_mutex);
	}
	
	return false;
}

bool try_take_fork(t_philosopher *philosopher, long long time)
{
	t_fork *first_fork;
	t_fork *second_fork;
	(void)time;  // Add this to silence unused parameter warning

	// Always take lower ID fork first to prevent deadlock
	if (philosopher->left_fork->id < philosopher->right_fork->id)
	{
		first_fork = philosopher->left_fork;
		second_fork = philosopher->right_fork;
	} else {
		first_fork = philosopher->right_fork;
		second_fork = philosopher->left_fork;
	}

	// Try to take first fork
	pthread_mutex_lock(&first_fork->mutex);
	if (first_fork->user_id != -1 || check_dead(philosopher))
	{
		pthread_mutex_unlock(&first_fork->mutex);
		return false;
	}
	first_fork->user_id = philosopher->id;
	print_philo(philosopher, "has taken a fork");

	// Handle single fork case
	if (&philosopher->left_fork->mutex == &philosopher->right_fork->mutex)
	{
		pthread_mutex_unlock(&first_fork->mutex);
		return false;
	}

	// Try to take second fork
	pthread_mutex_lock(&second_fork->mutex);
	if (second_fork->user_id != -1 || check_dead(philosopher))
	{
		first_fork->user_id = -1;
		pthread_mutex_unlock(&first_fork->mutex);
		pthread_mutex_unlock(&second_fork->mutex);
		return false;
	}
	second_fork->user_id = philosopher->id;
	print_philo(philosopher, "has taken a fork");

	// Start eating only if we're not already dead
	if (!check_dead(philosopher))
	{
		start_eat(philosopher, timestamp());  // Use current time here
		pthread_mutex_unlock(&first_fork->mutex);
		pthread_mutex_unlock(&second_fork->mutex);
		return true;
	}

	// Release forks if we're dead
	first_fork->user_id = -1;
	second_fork->user_id = -1;
	pthread_mutex_unlock(&first_fork->mutex);
	pthread_mutex_unlock(&second_fork->mutex);
	return false;
}

void *death_monitor(void *arg)
{
	t_philo *philo_gen = (t_philo *)arg;
	int i;
	
	usleep(50);  // Shorter initial delay
	
	while (true)
	{
		long long current_time = timestamp();
		
		pthread_mutex_lock(&philo_gen->game_over_mutex);
		bool is_game_over = philo_gen->game_over;
		pthread_mutex_unlock(&philo_gen->game_over_mutex);
		
		if (is_game_over || check_game_over(philo_gen))
			return NULL;

		i = 0;
		while (i < philo_gen->number_of_philosophers)
		{
			if (check_philosopher_dead(&philo_gen->philosophers[i], philo_gen, current_time))
				return NULL;
			i++;
		}
		
		usleep(50);  // More frequent checks for precise timing
	}
	return NULL;
}

void *philo_think(void *philo)
{
	long long	 	time;
	t_philo			*philo_gen;
	t_philosopher	*philosopher;
	t_status		current_status;

	philosopher = (t_philosopher *)philo;
	philo_gen = (t_philo *)philosopher->philo;
	
	if (philosopher->id % 2 == 0)
		usleep(500);  // Even smaller delay for even philosophers
	
	while (!check_dead(philosopher))
	{
		time = timestamp();
		
		pthread_mutex_lock(&philosopher->status_mutex);
		current_status = philosopher->status;
		pthread_mutex_unlock(&philosopher->status_mutex);

		if (current_status == THINKING)
		{
			if (try_take_fork(philosopher, time))
				continue;
			usleep(100);  // Smaller delay between fork attempts
		}
		else if (current_status == EATING)
		{
			if (start_sleep(philosopher, time) != 0)
				break;
		}
		else if (current_status == SLEEPING)
			start_thinking(philosopher, time);
			
		if (check_game_over(philo_gen))
			break;
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
	pthread_t		death_monitor_thread;

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
	philo->num_eaten = malloc(sizeof(int) * philo->number_of_philosophers);
	philo->game_over = false;
	if (!philo->num_eaten)
		return (printf("Malloc failed\n"), 1);
	init_num_eaten(philo);
	pthread_mutex_init(&philo->num_eaten_mutex, NULL);
	pthread_mutex_init(&philo->game_over_mutex, NULL);
	pthread_mutex_init(&philo->write_mutex, NULL);
	while (i < philo->number_of_philosophers)
	{
		philosophers[i].status = THINKING;
		philosophers[i].time_act_start = time;
		philosophers[i].id = i + 1;
		philosophers[i].time_act_end = 0;
		philosophers[i].eaten_count = 0;
		philosophers[i].philo = philo;
		philosophers[i].game_over = false;
		pthread_mutex_init(&philosophers[i].dead, NULL);
		pthread_mutex_init(&philosophers[i].status_mutex, NULL);
		pthread_mutex_init(&philosophers[i].timing_mutex, NULL);
		if (i == 0)
			philosophers[i].right_fork = forks[philo->number_of_philosophers-1];
		else
			philosophers[i].right_fork = forks[i-1];
		philosophers[i].left_fork = forks[i];
		philosophers[i].last_time_eat = time;  // Initialize last_time_eat to start time
		i++;
	}

	// Create philosopher threads first
	i = 0;
	while (i < philo->number_of_philosophers)
	{
		if (pthread_create(&philo->tid[i], NULL, philo_think, &philosophers[i]))
		{
			philo->game_over = true;
			return (printf("Error creating philosopher thread\n"), 1);
		}
		i++;
	}

	// Create death monitor thread immediately
	if (pthread_create(&death_monitor_thread, NULL, death_monitor, philo))
		return (printf("Error creating death monitor thread\n"), 1);

	// Wait for philosopher threads
	i = 0;
	while (i < philo->number_of_philosophers)
	{
		pthread_join(philo->tid[i], NULL);
		i++;
	}

	// Wait for death monitor thread
	pthread_join(death_monitor_thread, NULL);

	// Cleanup mutexes and memory
	i = 0;
	while (i < philo->number_of_philosophers)
	{
		pthread_mutex_destroy(&philosophers[i].dead);
		pthread_mutex_destroy(&philosophers[i].status_mutex);
		pthread_mutex_destroy(&philosophers[i].timing_mutex);
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
	pthread_mutex_destroy(&philo->game_over_mutex);
	pthread_mutex_destroy(&philo->num_eaten_mutex);
	free(philo);

	return (0);
}