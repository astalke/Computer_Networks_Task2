CC=gcc
FLAGS= -Wall -Wextra -O2 -std=c11 -g -I ./src/ -lm
COMMON_INCLUDE = $(wildcard ./src/common/*.h)
SERVER_INCLUDE = $(COMMON_INCLUDE) $(wildcard ./src/server/*.h)
CLIENT_INCLUDE = $(COMMON_INCLUDE) $(wildcard ./src/client/*.h)
COMMON_OBJ = $(patsubst ./src/common/%.c, ./obj/common/%.o, $(wildcard ./src/common/*.c))
SERVER_OBJ = $(patsubst ./src/server/%.c, ./obj/server/%.o, $(wildcard ./src/server/*.c))
CLIENT_OBJ = $(COMMON_OBJ) $(patsubst ./src/client/%.c, ./obj/client/%.o, $(wildcard ./src/client/*.c))

all: deps screen-worms-server screen-worms-client

deps:
	mkdir -p ./obj/server ./obj/client ./obj/common

screen-worms-server: $(COMMON_OBJ) $(SERVER_OBJ)
	$(CC) $^ $(FLAGS) -o $@

screen-worms-client: $(COMMON_OBJ) $(CLIENT_OBJ)
	$(CC) $^ $(FLAGS) -o $@

./obj/server/%.o: ./src/server/%.c $(SERVER_INCLUDE)
	$(CC) $< -c $(FLAGS) -o $@
./obj/client/%.o: ./src/client/%.c $(CLIENT_INCLUDE)
	$(CC) $< -c $(FLAGS) -o $@
./obj/common/%.o: ./src/common/%.c $(COMMON_INCLUDE)
	$(CC) $< -c $(FLAGS) -o $@


.PHONY: clean
clean:
	-rm ./obj/server/*.o
	-rm ./obj/client/*.o
	-rm ./obj/common/*.o
	-rm screen-worms-client
	-rm screen-worms-server
