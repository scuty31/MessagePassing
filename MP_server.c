#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include <signal.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <errno.h>
#include "mem.h"

#define CONNECT 32768+1

typedef struct DataMessage{
	long mtype;
	struct data_buf data;
	long source;
}Message_d;

typedef struct Room{
	long clients[2];
}Room;


data_buf mem1;
data_buf mem2;
data_buf mem;
Room room;
int mqid;

int user;

void initque();
void initMemoryData();
void recieveConnection();
void sendMessageUser1();
void sendMessageUser2();
void recieveData();
void* waitingRoomDataCommunication();
void* gameRoomDataCommunication();
void* judgeOmok(void* turn);

int main(){
	pthread_t waiting_thread;
	pthread_t game_thread;
	
	initque();
	initMemoryData();
	
	printf("a\n");	
	pthread_create(&waiting_thread, NULL, waitingRoomDataCommunication, NULL);
	pthread_join(waiting_thread, NULL);
	
	pthread_create(&game_thread, NULL, gameRoomDataCommunication, NULL);
        pthread_join(game_thread, NULL);

	sleep(5);

}

void initque(){
	mqid = msgget((key_t)60109, IPC_CREAT | 0666);
	if(mqid == -1){
		printf("error\n");
		exit(0);
	}
}

void initMemoryData(){
	mem1.wait_msg.connect = 0;
	mem1.wait_msg.ready = 0;
	mem1.wait_msg.status_change = 0;
	mem1.wait_msg.opponent_connect = 0;
	mem1.wait_msg.opponent_ready = 0;
	mem1.wait_msg.opponent_change = 0;

	mem1.game_msg.my_turn = 0;
	mem1.game_msg.result = 0;
	mem1.game_msg.row = 0;
	mem1.game_msg.col = 0;
	mem1.game_msg.turn_end = 0;

	mem2.wait_msg.connect = 0;
        mem2.wait_msg.ready = 0;
        mem2.wait_msg.status_change = 0;
        mem2.wait_msg.opponent_connect = 0;
        mem2.wait_msg.opponent_ready = 0;
        mem2.wait_msg.opponent_change = 0;

	mem2.game_msg.my_turn = 0;
        mem2.game_msg.result = 0;
	mem2.game_msg.row = 0;
        mem2.game_msg.col = 0;
        mem2.game_msg.turn_end = 0;

	for(int i = 0; i < ROW; i++){
		for(int k = 0; k < COLUMN; k++){
			mem1.game_msg.omok_board[i][k] = '+';
			mem2.game_msg.omok_board[i][k] = '+';
		}
	}
}

void recieveConnection(){
	Message_d message;
	int i;
	for(i=0; i<2; i++){	
		if((msgrcv(mqid, &message, sizeof(message) - sizeof(long), CONNECT, 0)) < 0)
			perror("msgrcv failed");
		else{
			room.clients[i] = message.source;
			printf("player %ld connect\n", room.clients[i]);
		}
	}
}

void sendMessageUser1(){
	Message_d message;
	message.mtype = room.clients[0];
	message.data = mem1;
	
	if(msgsnd(mqid, &message, sizeof(message) - sizeof(long), 0))	
		perror("msgsnd failed");
}

void sendMessageUser2(){
	Message_d message;
        message.mtype = room.clients[1];
        message.data = mem2;

	if(msgsnd(mqid, &message, sizeof(message) - sizeof(long), 0))
              	perror("msgsnd failed");
}

void recieveData(){
	Message_d message;
	if((msgrcv(mqid, &message, sizeof(message) - sizeof(long), CONNECT, 0))<0){
		perror("msgrcv failed");
	}
	else{
		mem = message.data;
		if(message.source == room.clients[0])
			mem1 = message.data;	
		else if(message.source == room.clients[1])
			mem2 = message.data;
		else
			perror("msgrcv source error");
	}
}

void* waitingRoomDataCommunication(){
	int player1_ready = 0;
	int player2_ready = 0;
	recieveConnection();
	
	printf("c\n"); 
	while(player1_ready == 0 || player2_ready == 0){
		recieveData();
		if(user == 1){
			
			if(mem1.wait_msg.connect == 1){
				mem2.wait_msg.opponent_change = 1;
				mem2.wait_msg.opponent_connect = 1;
			}
			else if(mem1.wait_msg.connect == 0){
				mem2.wait_msg.opponent_change = 1;
				mem2.wait_msg.opponent_connect = 0;
			}
			
			if(player1_ready == 0 && mem1.wait_msg.ready == 1){
				mem2.wait_msg.opponent_change = 1;
				mem2.wait_msg.opponent_ready = 1;

				player1_ready=1;
			}
			mem1.wait_msg.status_change = 0;
			sendMessageUser1();
			sendMessageUser2();
		}

		if(user == 2){
			if(mem2.wait_msg.connect == 1){
				mem1.wait_msg.opponent_change = 1;
				mem1.wait_msg.opponent_connect = 1;
			}
			else if(mem2.wait_msg.connect == 0){
				mem1.wait_msg.opponent_change = 1;
				mem1.wait_msg.opponent_connect = 0;
			}

			if(player2_ready == 0 && mem2.wait_msg.ready == 1){
				mem1.wait_msg.opponent_change = 1;
				mem1.wait_msg.opponent_ready=1;

				player2_ready = 1;
			}
			mem2.wait_msg.status_change = 0;
			sendMessageUser2();
			sendMessageUser1();
		}

	}

	pthread_exit(NULL);
}

void* gameRoomDataCommunication(){
	void* ret;
	int turn;
	double accum;
	pthread_t judge_thread;
	
	mem1.game_msg.my_turn = 1;
	
	while(1){
		while(1){

}
