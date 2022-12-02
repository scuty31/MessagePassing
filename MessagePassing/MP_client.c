#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include <signal.h>
#include <errno.h>
#include "mem.h"

void initque();
void initMenu();
void initNcurses();
void initMemoryData();
void gameRoom();
void waitingRoom();
void sendMessage();
void recieveMessage();
void* checkWaitingRoomPlayer2Status();
void* checkGameRoomPlayer2TurnEnd();
void printOmokBoard();
void* checkGameRoomMyturn();


pthread_t waiting_thread, game_thread, check_myturn_thread;
pthread_mutex_t mutex;

data_buf send_msg;
data_buf recv_msg;

long pid;
int mqid;

char* waiting_status[] = {"Wait", "Join", "Ready"};

WINDOW* waiting_player2_status;

int main(){

	initque();
	initMemoryData();
	initMenu();

	return 0;
}


void initque(){

        mqid = msgget((key_t)60109, IPC_CREAT | 0666);
        if(mqid == -1){
                printf("msgget error\n");
                exit(0);
        }
        pid = (long)getpid();
	send_msg.pid = pid;
}

void initMenu(){
	initNcurses();

	char* select[] = {"Game Strat", "Exit"};

	int xStart = 8, yStart = 4;
	int highlight = 0;
	int c, i;

	WINDOW * menu_win;
	menu_win = newwin(6, 21, yStart,  xStart);
	box(menu_win, 0, 0);

	keypad(menu_win, TRUE);

	wattron(menu_win, A_BOLD);
	mvwprintw(menu_win, 0, 6, "Omok Game");
	wattroff(menu_win, A_BOLD);

	refresh();
	wrefresh(menu_win);

	while(1){
		refresh();
		wrefresh(menu_win);

		for(i = 0; i<2; i++){
			if(highlight == i)
				wattron(menu_win, A_REVERSE);

			mvwprintw(menu_win, 2+i, 6, "%s", select[i]);
			wattroff(menu_win, A_REVERSE);
		}
		
		c = wgetch(menu_win);

		switch(c){
			case KEY_UP:
				if(highlight == 0) highlight = 1;
				highlight--;
				break;
			case KEY_DOWN:
				if(highlight == 1) highlight = 0;
				highlight++;
				break;
			default:
				break;
		}
		if(c == 10 || c == ' ') break;
	}

	if(highlight == 0){
		waitingRoom();
	}
	else{
		endwin();
		return;
	}
}

void initNcurses(){
	initscr();
	clear();
	noecho();
	cbreak();
	curs_set(0);
}

void initMemoryData(){
	send_msg.wait_msg.ready = 0;
	send_msg.wait_msg.opponent_connect = 0;
	send_msg.wait_msg.opponent_ready = 0;

	send_msg.game_msg.my_turn = 0;
	send_msg.game_msg.row = 0;
	send_msg.game_msg.col = 0;
	send_msg.game_msg.result = 0;

	for(int i = 0; i < ROW; i++){
		for(int k = 0; k < COLUMN; k++)
			send_msg.game_msg.omok_board[i][k] = '+';
	}
}



void waitingRoom(){
	initNcurses();

	int i, highlight = 0;
	int xStart = 5, yStart = 3;

	char* select[] = {"Ready!!"};
	
	mvprintw(1, 0, "%ld", pid);
	sendMessage();

	WINDOW* player1 = newwin(5, 15, yStart, xStart);
        box(player1, 0, 0);
        wattron(player1, A_BOLD);
        mvwprintw(player1, 0, 2, "player 1(Me)");
        wattroff(player1, A_BOLD);

	WINDOW* player2 = newwin(5, 15, yStart, xStart + 15);
        box(player2, 0, 0);
        wattron(player2, A_BOLD);
        mvwprintw(player2, 0, 4, "player 2");
        wattroff(player2, A_BOLD);

	WINDOW* player1_status = newwin(1, 7, yStart + 2, xStart + 5);
	mvwprintw(player1_status, 0, 0, "%s", waiting_status[1]);

	// player2의 상태 메세지
	waiting_player2_status = newwin(1, 7, yStart + 2, xStart + 20);

	// 대기실의 선택 메뉴
        WINDOW* ready_box = newwin(3, 15, yStart + 5, xStart);
        box(ready_box, 0, 0);

	// 선택 메뉴에서 키보드 사용
        keypad(ready_box, TRUE);

	// 화면 새로고침
        pthread_mutex_lock(&mutex);
	refresh();
	
        wrefresh(player1);
	wrefresh(player1_status);
        wrefresh(player2);
        wrefresh(ready_box);
	pthread_mutex_unlock(&mutex);

	pthread_create(&waiting_thread, NULL, checkWaitingRoomPlayer2Status, NULL);
        while(1){
		// 선택한 메뉴를 표시
                wattron(ready_box, A_REVERSE);
                mvwprintw(ready_box, 1, 5, "%s", select[0]);
                wattroff(ready_box, A_REVERSE);

		wrefresh(ready_box);
		int c;

		c = wgetch(ready_box);

		// 엔터를 누르면 무한루프 종료
                if((c == 10 || c == ' ') && recv_msg.wait_msg.opponent_connect == 1){
		       	wattron(ready_box, A_NORMAL);
                	mvwprintw(ready_box, 1, 5, "%s", select[0]);
                	wattroff(ready_box, A_NORMAL);
                	wrefresh(ready_box);

                	// player1의 상태를 ready로 변경
                	touchwin(player1_status);
                	mvwprintw(player1_status, 0, 0, "%s", waiting_status[2]);
                	wrefresh(player1_status);
	
	                send_msg.wait_msg.ready = 1;
	                sendMessage();
			
			pthread_join(waiting_thread, NULL);

	                WINDOW* start = newwin(5, 30, yStart + 5, xStart);
	                for(int i = 0; i < 3; i++){
	                        mvwprintw(start, 0, 0, "Start in %d seconds!!", 3 - i);
	                        wrefresh(start);
	                        sleep(1);
	                }
	
       		        endwin();

       		        gameRoom();
			
			break;
		}
        }
}

void sendMessage(){
	send_msg.mtype = SERVER;
	clock_gettime(CLOCK_MONOTONIC, &send_msg.start);
	if(msgsnd(mqid, &send_msg, sizeof(data_buf) - sizeof(long), 0) == -1)
                printf("msgsnd error\n");
}

void recieveMessage(){
	if((msgrcv(mqid, &recv_msg, sizeof(data_buf) - sizeof(long), pid, 0)) == -1)
		printf("msgrcv error\n");
}

void* checkWaitingRoomPlayer2Status(){
	pthread_mutex_lock(&mutex);
	mvwprintw(waiting_player2_status, 0, 0, "%s", waiting_status[0]);
        wrefresh(waiting_player2_status);
	pthread_mutex_unlock(&mutex);

	for(int i = 0; i < 2; i++){
		recieveMessage();

		if(i == 0){
			mvwprintw(waiting_player2_status, 0, 0, "%s", waiting_status[1]);
		        wrefresh(waiting_player2_status);
		}
		else if(i == 1){
			mvwprintw(waiting_player2_status, 0, 0, "%s", waiting_status[2]);
                        wrefresh(waiting_player2_status);
		}
	}
	mvprintw(0,0, "%d",recv_msg.wait_msg.opponent_ready);
	refresh();
	// 쓰레드 종료
	pthread_exit(NULL);
}

void gameRoom(){
	initNcurses();

	int xStart = 5, yStart = 3;
	int i, k, xPoint = 3;
	int row = 0, column = 0;
	void* ret;

	move(yStart, xStart + 2);
	keypad(stdscr, TRUE);

	printOmokBoard();

	while(1){
		pthread_create(&check_myturn_thread, NULL, checkGameRoomMyturn, NULL);
		pthread_join(check_myturn_thread, &ret);
		
		if(*(int*)ret == 0){
			pthread_create(&game_thread, NULL, checkGameRoomPlayer2TurnEnd, NULL);
			pthread_join(game_thread, &ret);
			printOmokBoard();
			
			if(*(int*)ret == 1){
				mvprintw(yStart + 15, xStart, "Lose...");
				refresh();

				for(int i = 0; i< 3; i++){
					mvprintw(yStart + 15 + 1, xStart, "End in %d seconds", 3 - i);
					refresh();
					sleep(1);
				}
				endwin();
				return;
			}
		}
		
		else{
			while(1){
				printOmokBoard();
				attron(A_REVERSE);
				mvprintw(row + yStart, xStart + column * xPoint + 2, "%c", send_msg.game_msg.omok_board[row][column]);
				attroff(A_REVERSE);

				int c;
				c = getch();

				switch(c){
				case KEY_UP:
					if(row == 0)
						row = 14;
					else
						row--;
					break;
				case KEY_DOWN:
					if(row == 14)
						row = 0;
					else
						row++;
					break;
				case KEY_LEFT:
					if(column == 0)
						column = 14;
					else
						column--;
					break;
				case KEY_RIGHT:
					if(column == 14)
						column = 0;
					else
						column++;
					break;
				default:
					break;
				}	

				if(c == 10|| c == ' '){
					if(send_msg.game_msg.omok_board[row][column] == '+'){
						send_msg.game_msg.omok_board[row][column]='O';
						send_msg.game_msg.row = row;
						send_msg.game_msg.col = column;
						
						sendMessage();
			
						printOmokBoard();
						
						usleep(100000);
						recieveMessage();

						if(recv_msg.game_msg.result == 1){
							mvprintw(yStart + 15, xStart, "Win!!");
							refresh();

							for(int i = 0; i<3; i++){
								mvprintw(yStart + 15 + 1, xStart, "End in %d seconds", 3-i);
								refresh();
								sleep(1);
							}
						endwin();
						return;
						}
						break;
					}
				}
			}
		}
	}
	endwin();
}


void printOmokBoard(){
	int xStart = 5, yStart = 3, xPoint = 3;
	for(int i = 0; i < ROW; i++) {
		mvprintw(i + yStart, xStart, "|");
		for (int k = 0; k < COLUMN; k++) {
			mvprintw(i + yStart, 1 + xPoint * k + xStart, "-%c-", send_msg.game_msg.omok_board[i][k]);
		}
		mvprintw(i + yStart, 1 + xPoint * 15 + xStart, "|");
	}
	refresh();
}

void* checkGameRoomMyturn(){
	int ret = 0;

	recieveMessage();
	if(recv_msg.game_msg.my_turn == 1){
		ret = 1;
		
		pthread_exit((void*)&ret);
	}

	pthread_exit((void*)&ret);
}

void* checkGameRoomPlayer2TurnEnd(){
	int ret = 0;
	
	recieveMessage();

	send_msg.game_msg.omok_board[recv_msg.game_msg.row][recv_msg.game_msg.col] = 'X';

	if(recv_msg.game_msg.result==2){
		ret = 1;
		pthread_exit((void*)&ret);
	}
	pthread_exit((void*)&ret);
}
