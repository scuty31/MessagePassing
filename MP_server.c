#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/msg.h>
#include <fcntl.h>
#include "mem.h"

#define BUFFER_SIZE 256
#define SERVER 1
#define CONNECT 1

typedef struct MessageType{
	long mtype;
	char data[15][15];
	long source;
}Message_t;

typedef struct WaitingRoom{
	long clients[2];
}WaitingRoom;

typedef struct Room{
	long clients[2];
}Room;

void receiveConnect(WaitingRoom* room);

key_t key_id;

int main(){
	pthread_t chat_thread;
	Room room;
	pthread_t t_id;
	void* game_thread_return;
	
	key_id = msgget((key_t)60109, IPC_CREAT | 0666);
	if(key_id == -1){
		printf("error\n");
		exit(0);
	}
	receiveConnect(&room);
}

void receiveConnect(WaitingRoom* room){
	int i;
	Message_t message;
	memset(message.data, 0, BUFFER_SIZE);
	
	
}
