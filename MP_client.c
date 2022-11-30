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

#define CONNECT 32768+1 

typedef struct DataMessage{
	long mtype;
	struct data_buf data;
	long source;
}Message_d;

typedef struct MultipleArg{
	long fd;
	long id;
}MultipleArg;

void initque();
void initMenu();
void initNcurses();
void initMemoryData();
void gameRoom();
void waitingRoom();
void sendConnection();
void sendMessage();
void recievedata();
void* checkWaitingRoomPlayer2Status();
void* checkGameRoomPlayer2TurnEnd();
void printOmokBoard();
void* checkGameRoomMyturn();


pthread_t waiting_thread, game_thread, check_myturn_thread;
data_buf mem;
MultipleArg* mArg;

char* waiting_status[] = {"Wait", "Join", "Ready"};
int ret  = 0;

WINDOW* waiting_player2_status;

int main(){

	initMenu();

	return 0;
}


void initque(){
	mArg = (MultipleArg*)malloc(sizeof(MultipleArg));

        mArg->fd = msgget((key_t)60109, IPC_CREAT | 0666);
        if(mArg->fd == -1){
                printf("error\n");
                exit(0);
        }
        mArg->id = (long)getpid();
}

void initMenu(){
	initNcurses();
	initque();
	initMemoryData();

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
	mem.wait_msg.connect = 0;
	mem.wait_msg.ready = 0;
	mem.wait_msg.status_change = 0;
	mem.wait_msg.opponent_connect = 0;
	mem.wait_msg.opponent_ready = 0;
	mem.wait_msg.opponent_change = 0;

	mem.game_msg.my_turn = 0;
	mem.game_msg.result = 0;
	mem.game_msg.row = 0;
	mem.game_msg.col = 0;
	mem.game_msg.turn_end = 0;
}

void waitingRoom(){
	initNcurses();

	int i, highlight = 0;
	int xStart = 5, yStart = 3;

	char* select[] = {"Ready!!", "Exit"};
	
	mem.wait_msg.connect = 1;
	mem.wait_msg.status_change = 1;
	
	sendMessage(); // wait_msg.connect = 1	

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
        WINDOW* ready_exit_box = newwin(3, 30, yStart + 5, xStart);
        box(ready_exit_box, 0, 0);

	// 선택 메뉴에서 키보드 사용
        keypad(ready_exit_box, TRUE);

	// 화면 새로고침
        refresh();

        wrefresh(player1);
	wrefresh(player1_status);
        wrefresh(player2);
        wrefresh(ready_exit_box);

	pthread_create(&waiting_thread, NULL, checkWaitingRoomPlayer2Status, NULL);
        while(1){
		// 선택한 메뉴를 표시
                for(i = 0; i < 2; i++){
                        if(highlight == i)
                                wattron(ready_exit_box, A_REVERSE);

                        mvwprintw(ready_exit_box, 1, 5 + i * 15, "%s", select[i]);
                        wattroff(ready_exit_box, A_REVERSE);
                }

		int c;

		c = wgetch(ready_exit_box);

		  switch(c){
                        case KEY_LEFT:
                                if(highlight == 0) highlight = 1;
                                highlight--;
                                break;
                        case KEY_RIGHT:
                                if(highlight == 1) highlight = 0;
                                highlight++;
                                break;
                        default:
                                break;
                }

		// 엔터를 누르면 무한루프 종료
                if(c == 10 || c == ' ') break;
        }
	if (highlight == 0){
		// 선택한 메뉴의 표시 효과 없에기
		wattron(ready_exit_box, A_NORMAL);
	        mvwprintw(ready_exit_box, 1, 5, "%s", select[0]);
	        wattroff(ready_exit_box, A_NORMAL);
	        wrefresh(ready_exit_box);
	
		// player1의 상태를 ready로 변경
	        touchwin(player1_status);
	        mvwprintw(player1_status, 0, 0, "%s", waiting_status[2]);
	        wrefresh(player1_status);
		
		mem.wait_msg.ready = 1;
		mem.wait_msg.status_change = 1;
	
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
	}

	else{
		mem.wait_msg.connect = 0;
		mem.wait_msg.ready = 0;
		mem.wait_msg.status_change = 0;
		mem.wait_msg.opponent_connect = 0;
		mem.wait_msg.opponent_ready = 0;
		mem.wait_msg.opponent_change = 0;
		sendMessage();

		endwin();
		return;
	}
}

void sendConnection(){
	Message_d message;
	message.mtype = CONNECT;
	message.source = mArg->id;

	if(msgsnd(mArg->fd, &message, sizeof(message) - sizeof(long), 0))
                perror("msgsnd failed");
}

void sendMessage(){
	Message_d message;
	message.mtype = CONNECT;
	message.data = mem;
	message.source = mArg->id;

	if(msgsnd(mArg->fd, &message, sizeof(message) - sizeof(long), 0))
			perror("msgsnd failed");
			
}

void recieveData(){
	Message_d message;
	if((msgrcv(mArg->fd, &message, sizeof(message) - sizeof(long), mArg->id, 0))==-1)
		perror("msgrcv failed");
	mem = message.data;
}

void* checkWaitingRoomPlayer2Status(){
	mvwprintw(waiting_player2_status, 0, 0, "%s", waiting_status[0]);
        wrefresh(waiting_player2_status);
	
	// player2의 상태가 ready가 될 때 까지 반복
	while(mem.wait_msg.opponent_ready == 0){
		recieveData();
		
		// player2가 연결이 안되어있으면 wait 상태 표시
                if(mem.wait_msg.opponent_connect == 1){
                	touchwin(waiting_player2_status);
                        mvwprintw(waiting_player2_status, 0, 0, "%s", waiting_status[0]);
                        wrefresh(waiting_player2_status);
                }
		// player2가 연결되었으면 join 상태 표시
                else if(mem.wait_msg.opponent_connect == 1){
                	touchwin(waiting_player2_status);
                        mvwprintw(waiting_player2_status, 0, 0, "%s", waiting_status[1]);
                        wrefresh(waiting_player2_status);
                }
			// player2가 준비를 하면 ready 상태 표시
                if(mem.wait_msg.opponent_ready == 1){
                	touchwin(waiting_player2_status);
                        mvwprintw(waiting_player2_status, 0, 0, "%s", waiting_status[2]);
                        wrefresh(waiting_player2_status);
                }
		
	}
	// 쓰레드 종료
	pthread_exit(NULL);
	
}

void gameRoom(){
	initNcurses();

	int xStart = 5, yStart = 3;
	int i, k, xPoint = 3;
	int row = 0, column = 0;
	ret = 0;

	move(yStart, xStart + 2);
	keypad(stdscr, TRUE);
	
	for(i = 0; i<ROW; i++){
		for(k = 0; k<COLUMN; k++){
			mem.game_msg.omok_board[i][k] = '+';
		}
	}

	printOmokBoard();
	
	recieveData();

	while(1){
		pthread_create(&check_myturn_thread, NULL, checkGameRoomMyturn, NULL);
		pthread_join(check_myturn_thread, NULL);
		
		if(ret == 0){
			pthread_create(&game_thread, NULL, checkGameRoomPlayer2TurnEnd, NULL);
			pthread_join(game_thread, NULL);
			printOmokBoard();
			
			if(ret == 1){
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
				mvprintw(row + yStart, xStart + column * xPoint + 2, "%c", mem.game_msg.omok_board[row][column]);
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
					if(mem.game_msg.my_turn == 1 && mem.game_msg.omok_board[row][column] == '+'){
						mem.game_msg.omok_board[row][column]='O';
						mem.game_msg.row = row;
						mem.game_msg.col = column;
						
						sendMessage();
			
						printOmokBoard();

						recieveData();
						
						if(mem.game_msg.result == 1){
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
			mvprintw(i + yStart, 1 + xPoint * k + xStart, "-%c-", mem.game_msg.omok_board[i][k]);
		}
		mvprintw(i + yStart, 1 + xPoint * 15 + xStart, "|");
	}
	refresh();
}

void* checkGameRoomMyturn(){
	ret = 0;

	recieveData();
	if(mem.game_msg.my_turn == 1){
		ret = 1;
		
		pthread_exit(NULL);
	}

	pthread_exit(NULL);
}

void* checkGameRoomPlayer2TurnEnd(){
	ret = 0;
	
	recieveData();

	mem.game_msg.omok_board[mem.game_msg.row][mem.game_msg.col] = 'X';

	if(mem.game_msg.result==2){
		ret = 1;
		pthread_exit(NULL);
	}
	pthread_exit(NULL);
}
