NAME := webserv

CC := c++

CPPFLAGS := -Wall -Wextra -Werror -std=c++17

VPATH := srcs/

SRCS := main.cpp \
		server.cpp \
		client.cpp

OBJ := $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CPPFLAGS) $(OBJ) -o $(NAME) -fsanitize=address

%.o: %.cpp
	$(CC) $(CPPFLAGS) -c $< -o $@

clean:
	@rm -f $(OBJ)

fclean: clean
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re