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
Room room;
int mqid;

int user;
int ret;

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
			printf("player %d connect\n", i+1);
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
	
	while(player1_ready == 0 || player2_ready == 0){
		recieveData();
		if(mem1.wait_msg.status_change == 1){
			
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

		if(mem2.wait_msg.status_change == 1){
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
	ret = 0;
	int end = 0;

	pthread_t judge_thread;
	
	while(end == 0){
		for(int i = 0; i< 2; i++){
			if(i == 0){
				mem1.game_msg.my_turn = 1;
				sendMessageUser1();

				mem2.game_msg.my_turn = 0;
				sendMessageUser2();
			}

			if(i== 1){
				mem2.game_msg.my_turn = 1;
                                sendMessageUser2();

                                mem1.game_msg.my_turn = 0;
                                sendMessageUser1();
			}
			
			Message_d message;
	
			if((msgrcv(mqid, &message, sizeof(message) - sizeof(long), CONNECT, 0))<0){
				perror("msgrcv failed");
			}
			else{
				if(message.source == room.clients[0]){
					mem1 = message.data;
					pthread_create(&judge_thread, NULL, judgeOmok, (void*)&mem1);
				}
				else if(message.source == room.clients[1]){
					mem2 = message.data;
					pthread_create(&judge_thread, NULL, judgeOmok, (void*)&mem2);
				}

				pthread_join(judge_thread, NULL);

				if(ret == 1){
					if(i == 0){
						mem1.game_msg.result = 1;
						sendMessageUser1();

						mem2.game_msg.row = mem1.game_msg.row;
						mem2.game_msg.col = mem1.game_msg.col;
						mem2.game_msg.result = 2;
						sendMessageUser2();

						end = 1;
					}
					else{
						mem2.game_msg.result = 1;
						sendMessageUser2();

						mem1.game_msg.row = mem2.game_msg.row;
						mem1.game_msg.col = mem2.game_msg.col;
						mem1.game_msg.result = 2;
						sendMessageUser1();

						end = 1;
					}
					break;
				}
				else{
					if(i == 0){
						mem1.game_msg.result = 0;
						sendMessageUser1();

						mem2.game_msg.row = mem1.game_msg.row;
                                                mem2.game_msg.col = mem1.game_msg.col;
                                                sendMessageUser2();
					}
					else{
						mem2.game_msg.result = 0;
                                                sendMessageUser2();

                                                mem1.game_msg.row = mem2.game_msg.row;
                                                mem1.game_msg.col = mem2.game_msg.col;
                                                sendMessageUser1();
					}
				}
			}
		}
	}
}	
					
void* judgeOmok(void* m){
	int i, k, cnt = 0;
	int row, col, ret = 0;
	char board[ROW][COLUMN];

	data_buf mem = *((data_buf*)m);
	
	for(i = 0; i<ROW; i++){
		for(k = 0; k<COLUMN; k++){
			board[i][k] = mem.game_msg.omok_board[i][k];
		}
	}
	row = mem.game_msg.row;
	col = mem.game_msg.col;
	
	for(i=0; i<ROW; i++){
		if(board[i][col] == 'O') cnt++;
		else cnt = 0;

		if(cnt == 5){
			if(board[i + 1][col] != '0'){
				ret = 1;
				pthread_exit(NULL);
			}
		}
	}

	cnt = 0;
	for(i = 0; i< COLUMN; i++){
		if(board[row][i] == 'O') cnt++;
		else cnt = 0;

		if(cnt == 5){
			if(board[row][i + 1] != 'O'){
				ret = 1;
				pthread_exit(NULL);
			}
		}
	}

	int start_row, start_col;
	if(row >= col){
		cnt = 0;
		start_row = row - col;
		start_col = 0;
		for(i = 0; start_row + i < 15; i++){
			if(board[start_row + i][start_col + i] == 'O') cnt++;
			else cnt = 0;

			if(cnt == 5){
				if(board[i][col + 1] != 'O'){
					ret = 1;
					pthread_exit(NULL);
				}
			}
		}
	}
	else{
		cnt = 0;
		start_row = 0;
		start_col = col - row;
		for(i = 0; start_col + 1 < 15; i++){
			if(board[start_row + i][start_col + i] == 'O') cnt++;
			else cnt = 0;

			if(start_col < 15 && cnt == 5){
				if(board[start_row + i + 1][start_col + i] != 'O'){
					ret = 1;
					pthread_exit(NULL);
				}
			}
		}
	}

	start_row = row + col;
	start_col = 0;
	cnt = 0;
	for(i = 0; start_row - i < 0; i++){
		if(board[start_row -i][i] == 'O') cnt++;
		else cnt = 0;

		if(start_row - i - 1 >= 0 && cnt == 5){
			if(board[start_row - i - 1][i + 1] != 'O'){
				ret = 1;
				pthread_exit(NULL);
			}
		}
	}

	ret = 0;
	printf("%d", ret);
	pthread_exit(NULL);
}
