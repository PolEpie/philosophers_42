/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   actions.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pepie <pepie@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/03 13:43:25 by pepie             #+#    #+#             */
/*   Updated: 2024/12/03 14:04:20 by pepie            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "philo.h"

int	start_eat(t_philosopher *philosopher, t_ll time)
{
	t_philo			*philo_gen;

	philo_gen = (t_philo *)philosopher->philo;
	printf("%lld %d has taken a fork\n", time - philo_gen->time_start,
		philosopher->id);
	printf("%lld %d has taken a fork\n", time - philo_gen->time_start,
		philosopher->id);
	printf("%lld %d is eating\n", time - philo_gen->time_start, philosopher->id);
	philosopher->status = EATING;
	philosopher->time_act_start = time;
	philosopher->time_act_end = time + philo_gen->time_to_eat;
	return (0);
}

void	start_thinking(t_philosopher *philosopher, t_ll time)
{
	t_philo			*philo_gen;

	philo_gen = (t_philo *)philosopher->philo;
	printf("%lld %d is thinking\n", time - philo_gen->time_start,
		philosopher->id);
	philosopher->status = THINKING;
	philosopher->time_act_start = time;
	philosopher->time_act_end = time;
	return ;
}

void	start_sleep(t_philosopher *philosopher, t_ll time)
{
	t_philo			*philo_gen;

	philo_gen = (t_philo *)philosopher->philo;
	pthread_mutex_lock(&philosopher->left_fork->mutex);
	philosopher->left_fork->user_id = -1;
	pthread_mutex_unlock(&philosopher->left_fork->mutex);
	pthread_mutex_lock(&philosopher->right_fork->mutex);
	philosopher->right_fork->user_id = -1;
	pthread_mutex_unlock(&philosopher->right_fork->mutex);
	printf("%lld %d is sleeping\n", time - philo_gen->time_start,
		philosopher->id);
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
		return ;
	}
	return ;
}


bool	try_take_fork2(t_philosopher *p, t_ll t, t_fork *f1, t_fork *f2)
{
	t_philo			*philo_gen;

	philo_gen = (t_philo *)p->philo;
	pthread_mutex_lock(&f1->mutex);
	if (take_fork(p, f1))
		return (pthread_mutex_unlock(&f1->mutex), 0);
	pthread_mutex_unlock(&f1->mutex);
	if (&p->left_fork->mutex == &p->right_fork->mutex)
	{	
		return (printf("%lld %d has taken a fork\n",
				t - philo_gen->time_start, p->id), 0);
	}
	pthread_mutex_lock(&f2->mutex);
	if (take_fork(p, f2))
	{
		pthread_mutex_lock(&f1->mutex);
		f1->user_id = -1;
		pthread_mutex_unlock(&f1->mutex);
		return (pthread_mutex_unlock(&f2->mutex), 0);
	}
	pthread_mutex_unlock(&f2->mutex);
	start_eat(p, t);
	return (0);
}


bool	try_take_fork(t_philosopher	*phil, t_ll time)
{
	t_fork			*first_fork;
	t_fork			*second_fork;

	if (phil->left_fork->id < phil->right_fork->id)
	{
		first_fork = phil->left_fork;
		second_fork = phil->right_fork;
	}
	else
	{
		first_fork = phil->right_fork;
		second_fork = phil->left_fork;
	}
	return (try_take_fork2(phil, time, first_fork, second_fork));
}
