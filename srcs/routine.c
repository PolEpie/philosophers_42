/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   routine.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pepie <pepie@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/03 13:55:46 by pepie             #+#    #+#             */
/*   Updated: 2024/12/03 14:03:49 by pepie            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "philo.h"

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

int	ft_usleep(int time)
{
	long long	start;

	start = timestamp();
	while ((timestamp() - start) < time)
		usleep(time / 10);
	return (0);
}

/* int		i;
bool	game_over;

i = 0;
if (philo_gen->num_eat_max == -1)
	return (false);
pthread_mutex_lock(&philo_gen->num_eaten_mutex);
game_over = true;
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
return (game_over); */
bool	check_game_over(t_philo *philo_gen)
{
	pthread_mutex_lock(&philo_gen->game_over_mutex);
	if (philo_gen->game_over)
	{
		pthread_mutex_unlock(&philo_gen->game_over_mutex);
		return (false);
	}
	pthread_mutex_unlock(&philo_gen->game_over_mutex);
	return (true);
}

void	*philo_think(void *philo)
{
	long long		time;
	t_philo			*philo_gen;
	t_philosopher	*philosopher;

	philosopher = (t_philosopher *)philo;
	philo_gen = (t_philo *)philosopher->philo;
	time = timestamp();
	while (check_game_over(philo_gen))
	{
		if (philosopher->status == THINKING)
			try_take_fork(philosopher, time);
		else if (philosopher->status == EATING)
			start_sleep(philosopher, time);
		else if (philosopher->status == SLEEPING)
			start_thinking(philosopher, time);
	}
	return (NULL);
}

/* time = timestamp();
if (time - philosopher->last_time_eat >= philo_gen->time_to_die)
{
	kill_philosopher(philosopher, philo_gen, time, true);
	break ;
}
else if (philosopher->time_act_end == 0 || (philosopher->time_act_end != 0 && time >= philosopher->time_act_end))
{ }*/