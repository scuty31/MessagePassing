#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <errno.h>
#include "mem.h"

data_buf send_msg;
data_buf recv_msg;

long pid[2];

int mqid;

void initque();
void recieveConnection();
void sendConnection();
void* waitingRoomDataCommunication();
void* gameRoomDataCommunication();
void* judgeOmok(void* g);
void printRatingTransferRate(struct timespec start, struct timespec end);

int main(){
	pthread_t waiting_thread;
	pthread_t game_thread;
	
	initque();
	
	pthread_create(&waiting_thread, NULL, waitingRoomDataCommunication, NULL);
	pthread_join(waiting_thread, NULL);
	
	pthread_create(&game_thread, NULL, gameRoomDataCommunication, NULL);
        pthread_join(game_thread, NULL);

	if(msgctl(mqid, IPC_RMID, 0) == -1)
		printf("msgctl error\n");
	return 0;
}

void initque(){
	mqid = msgget((key_t)60109, IPC_CREAT | 0666);
	if(mqid == -1){
		printf("error\n");
		exit(0);
	}
}

void recieveConnection(){
	int i;
	struct timespec time_end;

	for(i=0; i<2; i++){	
		if((msgrcv(mqid, &recv_msg, sizeof(data_buf) - sizeof(long), SERVER, 0)) == -1)
			perror("msgrcv failed");
		else{
			clock_gettime(CLOCK_MONOTONIC, &time_end);
                        printRatingTransferRate(recv_msg.start, time_end);

			pid[i] = recv_msg.pid;
			printf("player %d connect\n", i+1);
		}
	}
}

void sendConnection(){
	int i;

	for(i = 0; i < 2; i++){
		send_msg.wait_msg.opponent_connect = 1;
		send_msg.mtype = pid[i];
		if(msgsnd(mqid, &send_msg, sizeof(data_buf) - sizeof(long), 0) == -1)
			printf("msgsnd error\n");
	}
}

void* waitingRoomDataCommunication(){
	int i;
	int player1_ready = 0, player2_ready = 0;
	struct timespec time_end;

	recieveConnection();
	clock_gettime(CLOCK_MONOTONIC, &time_end);
        printRatingTransferRate(recv_msg.start, time_end);
	sendConnection();
	
	printf("%ld %ld\n", pid[0], pid[1]);

	for(i = 0; i < 2; i++){
		if(msgrcv(mqid, &recv_msg, sizeof(data_buf) - sizeof(long), SERVER, 0) == -1)
			printf("msgrcv error\n");

		clock_gettime(CLOCK_MONOTONIC, &time_end);
                printRatingTransferRate(recv_msg.start, time_end);
		
		if(recv_msg.pid == pid[0] && recv_msg.wait_msg.ready == 1){
			send_msg.mtype = pid[1];
			send_msg.wait_msg.opponent_ready = 1;
			if(msgsnd(mqid, &send_msg, sizeof(data_buf) - sizeof(long), 0) == -1)
                	        printf("msgsnd error\n");
			printf("player1 is ready\n");
			player1_ready = 1;
		}
		else if(recv_msg.pid == pid[1] && recv_msg.wait_msg.ready == 1){
			send_msg.mtype = pid[0];
			send_msg.wait_msg.opponent_ready = 1;
                        if(msgsnd(mqid, &send_msg, sizeof(data_buf) - sizeof(long), 0) == -1)
                                printf("msgsnd error\n");
			printf("player2 is ready\n");
			player2_ready = 1;
		}
	}

	printf("game start\n");
	pthread_exit(NULL);
}

void printRatingTransferRate(struct timespec start, struct timespec end){
        double accum;

        accum = (end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / 1000000000;
        printf("[Transfer Rate] %.9lf\n",accum);
}


void* gameRoomDataCommunication(){
	void* ret;
	int end = 0;
	struct timespec time_end;

	pthread_t judge_thread;
	
	while(end == 0){
		for(int i = 0; i< 2; i++){
			send_msg.game_msg.my_turn = 1;
			send_msg.mtype = pid[i];
			if(msgsnd(mqid, &send_msg, sizeof(data_buf) - sizeof(long), 0) == -1)
                                 printf("msgsnd error\n");
			
			send_msg.game_msg.my_turn = 0;
                        send_msg.mtype = pid[(i + 1) % 2];
                        if(msgsnd(mqid, &send_msg, sizeof(data_buf) - sizeof(long), 0) == -1)
                                 printf("msgsnd error\n");

			if((msgrcv(mqid, &recv_msg, sizeof(data_buf) - sizeof(long), SERVER, 0)) == -1){
				 printf("msgrcv error\n");
			}
			clock_gettime(CLOCK_MONOTONIC, &time_end);
        		printRatingTransferRate(recv_msg.start, time_end);
			
			pthread_create(&judge_thread, NULL, judgeOmok, (void*)&recv_msg.game_msg);
			pthread_join(judge_thread,(void*) &ret);

			if(*(int*)ret == 1){
				send_msg.game_msg.result = 1;
				send_msg.mtype = pid[i];
				if(msgsnd(mqid, &send_msg, sizeof(data_buf) - sizeof(long), 0) == -1)
                                     printf("msgsnd error\n");

				send_msg.game_msg.result = 2;
				send_msg.game_msg.row = recv_msg.game_msg.row;
				send_msg.game_msg.col = recv_msg.game_msg.col;
				send_msg.mtype = pid[(i + 1) % 2];
				if(msgsnd(mqid, &send_msg, sizeof(data_buf) - sizeof(long), 0) == -1)
                                     printf("msgsnd error\n");

				end = 1;

				printf("game ended\n");
			}
			else{
				send_msg.game_msg.result = 0;
				send_msg.mtype = pid[i];
				if(msgsnd(mqid, &send_msg, sizeof(data_buf) - sizeof(long), 0) == -1)
                                     printf("msgsnd error\n");

				send_msg.game_msg.row = recv_msg.game_msg.row;
                                send_msg.game_msg.col = recv_msg.game_msg.col;
				send_msg.game_msg.result = 0;
                                send_msg.mtype = pid[(i + 1) % 2];
				if(msgsnd(mqid, &send_msg, sizeof(data_buf) - sizeof(long), 0) == -1)
                                     printf("msgsnd error\n");
			}
		}
	}
	pthread_exit(NULL);
}	
					
void* judgeOmok(void* g){
	int i, k, cnt = 0;
	int row, col, ret = 0;
	char board[ROW][COLUMN];

	game_room_msg game= *((game_room_msg*)g);

	for(i = 0; i < ROW; i++){
		for(k = 0; k < COLUMN; k++){
			board[i][k] = game.omok_board[i][k];
		}
	}
	row = game.row;
	col = game.col;


	for(i = 0; i < ROW; i++){
		if(board[i][col] == 'O') cnt++;
		else cnt = 0;

		if(cnt == 5){
			if(i + 1 == ROW || board[i + 1][col] != 'O'){
				ret = 1;
			        pthread_exit((void*)&ret);
			}
		}
	}

	cnt = 0;
	for(i = 0; i < COLUMN; i++){
                if(board[row][i] == 'O') cnt++;
                else cnt = 0;

		if(cnt == 5){
                         if(i + 1 == COLUMN || board[row][i + 1] != 'O'){
                                 ret = 1;
                                 pthread_exit((void*)&ret);
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
                          if(start_row + i + 1 == ROW || board[start_row + i + 1][start_col + i + 1] != 'O'){
                                  ret = 1;
                                  pthread_exit((void*)&ret);
                          }
                  }

		}
        }
	else{
		cnt = 0;
                start_row = 0;
                start_col = col - row;
                for(i = 0; start_col + i < 15; i++){
                        if(board[start_row + i][start_col + i] == 'O') cnt++;
                        else cnt = 0;

			if(cnt == 5){
                          if(start_col + i + 1 == COLUMN || board[start_row + i + 1][start_col + i + 1] != 'O'){
                                  ret = 1;
                                  pthread_exit((void*)&ret);
                          }
                  }

                }

	}

	start_row = row + col;
	start_col = 0;
	cnt = 0;
	for(i = 0; start_row - i >= 0; i++){
		if(board[start_row - i][i] == 'O') cnt++;
		else cnt = 0;

		if(cnt == 5){
                          if(start_row - i - 1 == -1 || board[start_row - i - 1][i + 1] != 'O'){
                                  ret = 1;
                                  pthread_exit((void*)&ret);
                          }
                  }

	}

	ret = 0;
	pthread_exit((void*)&ret);

}
