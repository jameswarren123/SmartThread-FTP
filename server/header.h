#define BUFFER_SIZE 1000
#define STX "-="
#define TTX "=="
#define ETX "=-"
#define READY "Ready"
#define ERROR "Error"

#include "string.h"
#include "unistd.h"

void ready(int FD) {
    write(FD, "Ready", sizeof("Ready"));
}

void error(int FD) {
    write(FD, "Error", sizeof("Error"));
}

void print(char *msg) {
    write(1, msg, strlen(msg));
}

typedef struct
{
    int id;
    char buffer[BUFFER_SIZE];
} Packet;
