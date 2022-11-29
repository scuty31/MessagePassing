#pragma once

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <stdlib.h>

#define QKEY (key_t)60109
#define QPERM 0666
#define MAXOBN 15 
#define MAXPRIOR 10

struct q_entry{
long mtype;
char mtext [MAXOBN][MAXOBN];
};
