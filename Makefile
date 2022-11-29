CC = gcc
TARGET = 

normar: $(TARGET)

shm_server: shm_server.c
	$(CC) -o shm_server shm_server.c -lpthread

shm_client1: shm_client1.c
	$(CC) -o shm_client1 shm_client1.c -lpthread -lncurses

shm_client2: shm_client2.c
	$(CC) -o shm_client2 shm_client2.c -lpthread -lncurses

clean:
	$(RM) $(TARGET)
