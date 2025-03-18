/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   action.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pepie <pepie@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/17 07:51:42 by pepie             #+#    #+#             */
/*   Updated: 2025/03/18 03:02:36 by pepie            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "philo.h"

void	print_philo(t_philosopher *philosopher, char *str)
{
	t_philo			*philo_gen;

	philo_gen = (t_philo *)philosopher->philo;
	pthread_mutex_lock(&philo_gen->write_mutex);
	if (!check_dead(philo_gen))
		printf("%llu %d %s\n", timestamp() - philo_gen->time_start,
			philosopher->id, str);
	pthread_mutex_unlock(&philo_gen->write_mutex);
}

int	start_eat(t_philosopher *philosopher)
{
	t_philo	*philo_gen;

	philo_gen = (t_philo *)philosopher->philo;
	pthread_mutex_lock(&philo_gen->eating_mutex);
	philosopher->is_eating = true;
	philosopher->eaten_count += 1;
	philosopher->last_time_eat = timestamp();
	pthread_mutex_unlock(&philo_gen->eating_mutex);
	print_philo(philosopher, "is eating");
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

int	start_sleep(t_philosopher *philosopher)
{
	t_philo	*philo_gen;

	philo_gen = (t_philo *)philosopher->philo;
	print_philo(philosopher, "is sleeping");
	ft_usleep(philo_gen->time_to_sleep, philo_gen);
	return (0);
}

void	try_take_fork(t_philosopher *philosopher)
{
	pthread_mutex_t	*first_fork;
	pthread_mutex_t	*second_fork;

	if (philosopher->id % 2 == 0)
	{
		first_fork = &philosopher->right_fork->mutex;
		second_fork = &philosopher->left_fork->mutex;
	}
	else
	{
		first_fork = &philosopher->left_fork->mutex;
		second_fork = &philosopher->right_fork->mutex;
	}
	pthread_mutex_lock(first_fork);
	print_philo(philosopher, "has taken a fork");
	pthread_mutex_lock(second_fork);
	print_philo(philosopher, "has taken a fork");
	start_eat(philosopher);
	pthread_mutex_unlock(first_fork);
	pthread_mutex_unlock(second_fork);
}
