compile:
	gcc server.c -std=gnu99 -Wextra -fno-common -lraylib -lGL -lm -pthread -ldl -lrt -lX11 -Wall functions.c util_functions.c setup.c -lm -o server
	gcc client.c -std=gnu99 -Wextra -fno-common -lraylib -lGL -lm -pthread -ldl -lrt -lX11 -Wall functions.c util_functions.c setup.c -lm -o client
clean:
	rm server client -f