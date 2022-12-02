#pragma once
#include <stdio.h>
#include <time.h>

#define ROW 15
#define COLUMN 15
#define IFLAGS IPC_CREAT
#define ERRO ((data_buf *) -1)
#define BILLION 1000000000L
#define SERVER 1

typedef struct waiting_room_msg{
        int ready;
        int opponent_connect;
        int opponent_ready;
}waiting_room_msg;

typedef struct game_room_msg{
        int my_turn;
	int row;
	int col;
        int result;
        char omok_board[ROW][COLUMN];
}game_room_msg;

typedef struct data_buf{
	long mtype;
	long pid;
	struct timespec start;
        waiting_room_msg wait_msg;
        game_room_msg game_msg;
}data_buf;
