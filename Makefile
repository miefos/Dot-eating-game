compile:
	gcc server.c -lraylib -lGL -lm -lpthread -pthread -ldl -lrt -lX11 -Wall functions.c util_functions.c setup.c -o server
	gcc client.c -lraylib -lGL -lm -lpthread -pthread -ldl -lrt -lX11 -Wall functions.c util_functions.c setup.c -o client
