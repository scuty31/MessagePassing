#pragma once
#include <stdio.h>
#include <signal.h>
#include <sys/shm.h>
#include <time.h>

#define ROW 15
#define COLUMN 15
#define IFLAGS IPC_CREAT
#define ERRO ((data_buf *) -1)
#define BILLION 1000000000L

typedef struct waiting_room_msg{
        int connect;
        int ready;
	int status_change;
        int opponent_connect;
        int opponent_ready;
	int opponent_change;
}waiting_room_msg;

typedef struct game_room_msg{
        int my_turn;
	int row;
	int col;
	int turn_end;
        int result;
        char omok_board[ROW][COLUMN];
}game_room_msg;

typedef struct data_buf{
	struct timespec start;
	struct timespec stop;	
        waiting_room_msg wait_msg;
        game_room_msg game_msg;
}data_buf;
