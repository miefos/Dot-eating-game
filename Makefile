compile:
	gcc -Wall -pthread functions.c util_functions.c setup.c server.c -o server
	gcc -Wall -pthread functions.c util_functions.c setup.c client.c -o client
