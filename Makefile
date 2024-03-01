CFLAGS = -Wall -g

PORT_SERVER = 12345
IP_SERVER = 127.0.0.1
ID_CLIENT = 1234


all: server subscriber


# Compileaza server.cpp
server: server.cpp

#Compileaza subscriber.cpp
subescriber: subscriber.cpp

.PHONY: clean run_server run_subscriber

# Ruleaza serverul
run_server:
	./server ${PORT_SERVER}

# Ruleaza subscriberul
run_subscriber:
	./subscriber ${ID_CLIENT} ${IP_SERVER} ${PORT_SERVER}

clean:
	rm -f server subscriber