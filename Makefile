NAME := webserv

CC := c++

CPPFLAGS := -Wall -Wextra -Werror -std=c++17

VPATH := srcs/

OBJDIR = objs/

SRCS := main.cpp \
		server.cpp \
		config.cpp \
		request/client.cpp \
		utils.cpp \
		response/response.cpp \
		request/request.cpp \
		cgi.cpp \
		methods.cpp

OBJ = $(SRCS:.cpp=.o)
OBJS_PATH = $(addprefix $(OBJDIR), $(OBJ))

all: $(OBJDIR) $(NAME)

$(NAME): $(OBJS_PATH)
		$(CC) $(CPPFLAGS) $(OBJS_PATH) -o $(NAME)

$(OBJDIR)%.o: %.cpp | $(OBJDIR)
		@mkdir -p $(dir $@)
		$(CC) $(CPPFLAGS) -c $< -o $@

$(OBJDIR):
		@mkdir -p $(OBJDIR)

clean:
		@rm -rf $(OBJDIR)

fclean: clean
		@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re