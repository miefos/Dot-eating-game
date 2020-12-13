compile:
	gcc server.c -std=c90 -Wextra -fno-common -lraylib -lGL -lm -lpthread -pthread -ldl -lrt -lX11 -Wall functions.c util_functions.c setup.c -o server
	gcc client.c -std=c90 -Wextra -fno-common -lraylib -lGL -lm -lpthread -pthread -ldl -lrt -lX11 -Wall functions.c util_functions.c setup.c -o client
clean:
	rm server client -f