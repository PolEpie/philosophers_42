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

typedef long long	t_ll;

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
	t_status		status;
	long long		time_act_start;
	long long		time_act_end;
	long long		last_time_eat;
	pthread_t		thread;
	t_fork			*left_fork;
	t_fork			*right_fork;
	pthread_mutex_t	dead;
	pthread_mutex_t	write_mutex;
	pthread_mutex_t	finish;
	void 			*philo;
	bool			game_over;
}	t_philosopher;

typedef struct s_philo
{
	int				number_of_philosophers;
	t_ll			time_to_die;
	t_ll			time_to_eat;
	t_ll			time_to_sleep;
	t_ll			time_start;
	int				num_eat_max;
	t_philosopher	*philosophers;
	t_fork			**forks;
    pthread_mutex_t *philo_mutex;
	pthread_t		*tid;
	int				*num_eaten;
	pthread_mutex_t	num_eaten_mutex;
	bool			game_over;
	pthread_mutex_t	game_over_mutex;
}	t_philo;

int			ft_atoi(char *str);

int			take_fork(t_philosopher *philosopher, t_fork *fork);
int			ft_usleep(int time);

/* Routine */
void		*philo_think(void *philo);
t_ll		timestamp(void);


/* Actions */
bool		try_take_fork(t_philosopher *phil, t_ll time);
void		start_sleep(t_philosopher *philosopher, t_ll time);
void		start_thinking(t_philosopher *philosopher, t_ll time);
void		kill_philosopher(t_philosopher *philosopher, t_philo *philo_gen, t_ll time, bool show_dead);
bool		check_game_over(t_philo *philo_gen);

#endif