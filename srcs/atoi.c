/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_atoi.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pepie <pepie@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/09/10 18:49:16 by pepie             #+#    #+#             */
/*   Updated: 2023/09/21 20:36:55 by pepie            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

int	ft_str_is_num(char str)
{
	if (str >= '0' && str <= '9')
		return (1);
	else
		return (0);
}

int	ft_str_is_whitespace(char str)
{
	if (str == ' ' || str == '\t' || str == '\n' || str == '\f')
		return (1);
	else if (str == '\v' || str == '\r')
		return (1);
	else
		return (0);
}

int	ft_atoi(char*str)
{
	int	num;

	num = -1;
	while (ft_str_is_whitespace(*str))
		str++;
	if (*str == '-' || *str == '+')
	{
		str++;
	}
	while (ft_str_is_num(*str))
	{
		if (num == -1)
			num = 0;
		num = num * 10;
		num = num + *str - '0';
		str++;
	}
	return (num);
}