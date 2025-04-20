##
## EPITECH PROJECT, 2025
## Makefile
## File description:
## Makefile
##

SRC_SERVER = src/Server/main.cpp \
			src/Server/Server.cpp \
			src/Server/Broadcaster.cpp \
			src/Server/Physics.cpp

SRC_CLIENT = src/Client/main.cpp \
			src/Client/NetworkClient.cpp \
			src/Client/GameDisplay.cpp \
			src/Client/SoundManager.cpp

OBJ_SRC_SERVER = $(SRC_SERVER:.cpp=.o)
OBJ_SRC_CLIENT = $(SRC_CLIENT:.cpp=.o)

CXXFLAGS = -Wall -Wextra -Werror -std=c++20

INCFLAGS_SERVER = -I./src/Server -I./src/Shared
INCFLAGS_CLIENT = -I./src/Client -I./src/Shared

LDFLAGS_CLIENT = -lsfml-graphics -lsfml-window -lsfml-system -lsfml-network -lsfml-audio
LDFLAGS =

CXX ?= g++
RM ?= rm -f

NAME_SERVER = jetpack_server
NAME_CLIENT = jetpack_client

.PHONY: all server client clean fclean re

all: server client

server: $(OBJ_SRC_SERVER)
	$(CXX) $(OBJ_SRC_SERVER) $(LDFLAGS) -o $(NAME_SERVER)

client: $(OBJ_SRC_CLIENT)
	$(CXX) $(OBJ_SRC_CLIENT) $(LDFLAGS) $(LDFLAGS_CLIENT) -o $(NAME_CLIENT)

$(OBJ_SRC_SERVER): %.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCFLAGS_SERVER) -c $< -o $@

$(OBJ_SRC_CLIENT): %.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCFLAGS_CLIENT) -c $< -o $@

clean:
	$(RM) $(OBJ_SRC_SERVER) $(OBJ_SRC_CLIENT)

fclean: clean
	$(RM) $(NAME_SERVER) $(NAME_CLIENT)

re: fclean all
