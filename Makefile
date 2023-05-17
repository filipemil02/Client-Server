CFLAGS = -Wall -g

all: server subscriber

# Compile server.c
server: server.cpp

# Compile subscriber.c
subscriber: subscriber.cpp

.PHONY: clean run_server run_subscriber

# Run server
run_server: server
	./server ${PORT}

# Run subscriber
run_subscriber: subscriber
	./subscriber ${ID_CLIENT} ${IP_SERVER} ${PORT}

clean:
	rm -f server subscriber
