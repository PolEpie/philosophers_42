NAME		=	philo

CC			=	cc
INCLUDE 	=	./includes/
INCLUDE_LIBFT = $(addprefix $(LIBFT), includes/)

CFLAGS		=	-Wall -Werror -Wextra -g -I$(INCLUDE)
RM			=	rm -f
OBJ_DIR		=	objs
SRCS		=	./srcs/main.c \
				./srcs/atoi.c \

OBJS		=	$(SRCS:/%.c=%.o)

$(NAME):		$(OBJS)
				@$(CC) $(CFLAGS) $(OBJS) -o $(NAME);
				@echo "Linked into executable \033[0;32m$(NAME)\033[0m."

$(LIBFT_A):
				@$(MAKE) -s -C $(LIBFT)
				@echo "Compiled $(LIBFT_A)."

all:			$(NAME)

bonus:			all

.c.o:
				@$(CC) $(CFLAGS) -c $< -o $(OBJ_DIR)/$(<:.c=.o)
				@echo "Compiling $<."

localclean:
				@$(RM) -rf $(OBJ_DIR)
				@mkdir $(OBJ_DIR)
				@echo "Removed object files."

clean:			localclean

fclean:			localclean
				@$(RM) $(NAME)
				@echo "Removed executable."

test:			$(NAME)
				./$(NAME) 4 10 3 2

re:				fclean all

.PHONY:			all clean fclean re localclean bonus 