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
#include <unistd.h>

// Function prototype declarations
long long timestamp(void);
bool check_dead(t_philosopher *philosopher);
void kill_philosopher(t_philosopher *philosopher, t_philo *philo_gen, unsigned long time, bool show_dead);
int ft_usleep(int time, t_philosopher *philosopher);
void print_philo(t_philosopher *philosopher, char *str);
bool is_philosopher_dead(t_philosopher *philosopher);

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

	// Set global game_over flag
	pthread_mutex_lock(&philo_gen->game_over_mutex);
	if (philo_gen->game_over) {
		pthread_mutex_unlock(&philo_gen->game_over_mutex);
		return;
	}
	philo_gen->game_over = true;
	pthread_mutex_unlock(&philo_gen->game_over_mutex);
	
	// Set this philosopher's game_over flag
	pthread_mutex_lock(&philosopher->dead);
	if (philosopher->game_over) {
		pthread_mutex_unlock(&philosopher->dead);
		return;
	}
	philosopher->game_over = true;
	pthread_mutex_unlock(&philosopher->dead);
	
	// Update status to DEAD
	pthread_mutex_lock(&philosopher->status_mutex);
	philosopher->status = DEAD;
	pthread_mutex_unlock(&philosopher->status_mutex);
	
	// Mark all philosophers as game over to ensure they exit their loops
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
	
	// Display death message if needed
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

bool is_philosopher_dead(t_philosopher *philosopher)
{
    t_philo *philo_gen = philosopher->philo;
    long long current_time = timestamp();
    long long last_eat;
    
    pthread_mutex_lock(&philosopher->timing_mutex);
    last_eat = philosopher->last_time_eat;
    pthread_mutex_unlock(&philosopher->timing_mutex);
    
    // Use >= for absolutely consistent death detection
    return ((current_time - last_eat) >= philo_gen->time_to_die);
}

int ft_usleep(int time, t_philosopher *philosopher)
{
	long long	start;
	long long	current;

	start = timestamp();
	while (1)
	{
		// Check if the philosopher is dead or if the simulation is over
		if (check_dead(philosopher))
			return (1);
			
		// Check if time_to_die has elapsed
		if (is_philosopher_dead(philosopher)) {
			kill_philosopher(philosopher, philosopher->philo, timestamp(), true);
			return (1);
		}
			
		current = timestamp();
		if ((current - start) >= time)
			break;
			
		// Sleep for very short intervals and check death status frequently
		usleep(1);
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
	if (ft_usleep(philo_gen->time_to_eat, philosopher))
		return (1);

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
	
	if (philosopher->time_think > 0)
		ft_usleep(philosopher->time_think, philosopher);
	return (0);
}

int start_sleep(t_philosopher *philosopher, unsigned long time)
{
	t_philo          *philo_gen;
	
	philo_gen = (t_philo *)philosopher->philo;
	
	// Update timing information
	pthread_mutex_lock(&philosopher->timing_mutex);
	philosopher->time_act_start = time;
	philosopher->time_act_end = time + philo_gen->time_to_sleep;
	pthread_mutex_unlock(&philosopher->timing_mutex);
	
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
	
	ft_usleep(philo_gen->time_to_sleep, philosopher);
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
	
	// Get the time of last meal atomically
	pthread_mutex_lock(&philosopher->timing_mutex);
	last_eat = philosopher->last_time_eat;
	pthread_mutex_unlock(&philosopher->timing_mutex);

	// IMPORTANT CHANGE: Using >= for absolutely consistent death detection
	if ((current_time - last_eat) >= philo_gen->time_to_die)
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

// Update the death_monitor function to be more aggressive
void *death_monitor(void *arg)
{
    t_philo *philo_gen = (t_philo *)arg;
    int i;
    long long current_time;
    
    while (true)
    {
        // First check if the simulation is already over
        pthread_mutex_lock(&philo_gen->game_over_mutex);
        bool is_game_over = philo_gen->game_over;
        pthread_mutex_unlock(&philo_gen->game_over_mutex);
        
        if (is_game_over)
            return NULL;
            
        // Then check if everyone has eaten enough
        if (check_game_over(philo_gen)) {
            pthread_mutex_lock(&philo_gen->game_over_mutex);
            philo_gen->game_over = true;
            pthread_mutex_unlock(&philo_gen->game_over_mutex);
            return NULL;
        }

        // Check each philosopher for death with precise timing
        current_time = timestamp();
        i = 0;
        while (i < philo_gen->number_of_philosophers)
        {
            pthread_mutex_lock(&philo_gen->philosophers[i].timing_mutex);
            long long last_eat = philo_gen->philosophers[i].last_time_eat;
            pthread_mutex_unlock(&philo_gen->philosophers[i].timing_mutex);
            
            // Use >= for absolutely consistent death detection
            if ((current_time - last_eat) >= philo_gen->time_to_die) {
                if (!check_dead(&philo_gen->philosophers[i])) {
                    kill_philosopher(&philo_gen->philosophers[i], philo_gen, current_time, true);
                    return NULL;
                }
            }
            i++;
        }
        
        // Use a much shorter sleep interval for more precise death detection
        usleep(50);  // Check every 0.05ms instead of 0.1ms
    }
    return NULL;
}

// Add this function to monitor eating
void *monitor_eat(void *arg)
{
    t_philo *philo_gen = (t_philo *)arg;
    int i;
    
    // Small initial delay to let philosophers start
    usleep(100);
    
    while (true)
    {
        // First check if simulation is already over
        pthread_mutex_lock(&philo_gen->game_over_mutex);
        bool is_game_over = philo_gen->game_over;
        pthread_mutex_unlock(&philo_gen->game_over_mutex);
        
        if (is_game_over)
            return NULL;

        // Check if all philosophers have eaten enough
        i = 0;
        bool all_ate_enough = true;
        
        pthread_mutex_lock(&philo_gen->num_eaten_mutex);
        while (i < philo_gen->number_of_philosophers)
        {
            if (philo_gen->num_eaten[i] < philo_gen->number_of_times_each_philosopher_must_eat)
            {
                all_ate_enough = false;
                break;
            }
            i++;
        }
        pthread_mutex_unlock(&philo_gen->num_eaten_mutex);
        
        if (all_ate_enough)
        {
            // End simulation if all philosophers have eaten enough
            pthread_mutex_lock(&philo_gen->game_over_mutex);
            philo_gen->game_over = true;
            pthread_mutex_unlock(&philo_gen->game_over_mutex);
            return NULL;
        }
        
        usleep(500); // Short sleep to prevent busy waiting
    }
    return NULL;
}

void *philo_think(void *philo)
{
    long long        time;
    t_philo          *philo_gen;
    t_philosopher    *philosopher;
    t_status         current_status;

    philosopher = (t_philosopher *)philo;
    philo_gen = (t_philo *)philosopher->philo;
    
    // Special case for single philosopher
    if (philo_gen->number_of_philosophers == 1) {
        pthread_mutex_lock(&philosopher->left_fork->mutex);
        print_philo(philosopher, "has taken a fork");
        ft_usleep(philo_gen->time_to_die + 10, philosopher);
        pthread_mutex_unlock(&philosopher->left_fork->mutex);
        return NULL;
    }

    // Stagger start times to prevent deadlock
    if (philosopher->id % 2 == 0)
        ft_usleep(philo_gen->time_to_eat, philosopher);  // Shorter delay
    
    while (true)
    {
        if (check_dead(philosopher) || check_game_over(philo_gen))
            break;
            
        time = timestamp();
        
        pthread_mutex_lock(&philosopher->status_mutex);
        current_status = philosopher->status;
        pthread_mutex_unlock(&philosopher->status_mutex);

        if (current_status == THINKING)
        {
            try_take_fork(philosopher);
        }
        else if (current_status == EATING)
        {
            if (start_sleep(philosopher, time) != 0)
                break;
        }
        else if (current_status == SLEEPING)
        {
            start_thinking(philosopher, time);
            // Add a tiny delay to prevent busy-waiting
        }
            
        // Check again for exit conditions
        if (check_dead(philosopher) || check_game_over(philo_gen))
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
		philosophers[i].time_think = (philo->time_to_eat * ((i % 2) + 1)) - philo->time_to_sleep;
		philosophers[i].id = i + 1;
		philosophers[i].time_act_end = 0;
		philosophers[i].eaten_count = 0;
		philosophers[i].philo = philo;
		philosophers[i].game_over = false;
		pthread_mutex_init(&philosophers[i].dead, NULL);
		pthread_mutex_init(&philosophers[i].status_mutex, NULL);
		pthread_mutex_init(&philosophers[i].timing_mutex, NULL);
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
			philo->game_over = true;
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