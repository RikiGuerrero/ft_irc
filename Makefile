NAME = ircserv

CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98

SRCS = src/main.cpp src/Server.cpp src/Client.cpp src/Channel.cpp src/cmds/ServerCommands.cpp src/ServerUtils.cpp src/cmds/OpCommands.cpp src/cmds/mode.cpp
OBJS = $(SRCS:.cpp=.o)
INCS = -Iinclude

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all