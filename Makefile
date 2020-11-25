compile:
	gcc -Wall -g3 -fsanitize=address -pthread functions.c server.c -o server
	gcc -Wall -g3 -fsanitize=address -pthread functions.c client.c -o client
