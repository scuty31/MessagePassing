CC = gcc
TARGET = MP_client MP_server

normar: $(TARGET)

MP_server: MP_server.c
	$(CC) -o MP_server MP_server.c -lpthread

MP_client: MP_client.c
	$(CC) -o MP_client MP_client.c -lpthread -lncurses

clean:
	$(RM) $(TARGET)
