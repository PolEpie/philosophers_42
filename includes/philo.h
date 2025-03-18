/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   minishell.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pepie <pepie@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/12/17 23:57:11 by pepie             #+#    #+#             */
/*   Updated: 2024/05/10 12:41:01 by pepie            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PHILO_H
# define PHILO_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdbool.h>
#include <unistd.h>

typedef enum e_status
{
	THINKING,
	EATING,
	SLEEPING,
	DEAD
}	t_status;

typedef struct s_fork
{
	int				id;
	int				user_id;
	pthread_mutex_t	mutex;
}	t_fork;

typedef struct s_philosopher
{
	int				id;
	int				eaten_count;
	int				time_think;
	long long		last_time_eat;
	pthread_t		thread;
	t_fork			*left_fork;
	t_fork			*right_fork;
	void 			*philo;
	bool			is_eating;
}	t_philosopher;

typedef struct s_philo
{
	int				number_of_philosophers;
	long long		time_to_die;
	long long		time_to_eat;
	int				time_to_sleep;
	long long		time_start;
	int				number_of_times_each_philosopher_must_eat;
	t_philosopher	*philosophers;
	t_fork			**forks;
	pthread_t		*tid;
	pthread_mutex_t	write_mutex;
	pthread_mutex_t	start_mutex;
	pthread_mutex_t	eating_mutex;
	pthread_mutex_t	game_over_mutex;
	bool			game_over;
}	t_philo;

int			ft_atoi(char *str);
void		*death_monitor(void *arg);
void		*monitor_eat(void *arg);
long long	timestamp(void);
bool		check_dead(t_philo *philo);
int			ft_usleep(int time, t_philo *philo);
void		print_philo(t_philosopher *philosopher, char *str);

#endif