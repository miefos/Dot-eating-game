compile:
	gcc -Wall -pthread functions.c server.c -o server
	gcc -Wall -pthread functions.c client.c -o client
