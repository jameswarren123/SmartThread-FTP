#include "header.h"

#include "stdbool.h"
#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "string.h"
#include "pthread.h"

// Global Variables
int dataFD;
int terminateFD;
char *forsplit[3];
bool idStop[BUFFER_SIZE];

typedef struct
{
	int socketFD;
	char *file;
	int id;
} ThreadArgs;

//& methods
void *getsplit(void *args)
{
	ThreadArgs *argsT = (ThreadArgs *)args;
	int dataFDs = argsT->socketFD;
	char *arg = argsT->file;
	char buffer[BUFFER_SIZE];
	int readAmount;

    read(dataFDs, buffer, 6);
	if (strcmp(buffer, ERROR) == 0)
    {
        print("Error creating file on server\n");
        return NULL;
    }
    else
    {
        FILE *file = fopen(arg, "wb");
        if (file == NULL)
        {
            perror("bad file\n");
            exit(EXIT_FAILURE);
        }

		while ((readAmount = read(dataFDs, buffer, BUFFER_SIZE)) == BUFFER_SIZE)
        {
            fwrite(buffer, 1, readAmount, file);
			readAmount = read(dataFDs, buffer, BUFFER_SIZE);
			if(strcmp(buffer, TTX) == 0) {
				//print("Terminated\n");
				printf("\n\n%s\n", arg);
				remove(arg);
				return NULL;
			}
        }
		fwrite(buffer, 1, readAmount, file);
        fclose(file);
    }

	return NULL;
}
void *putsplit(void *args)
{
	ThreadArgs *argsT = (ThreadArgs *)args;
	int dataFDs = argsT->socketFD;
	char *arg = argsT->file;
	char buffer[BUFFER_SIZE];
	int readAmount;

    read(dataFDs, buffer, 6);
    FILE *file;
    if(strcmp(buffer, ERROR) == 0) {
        print("Server failed to create file\n");
        return NULL;
    }
    if (access(arg, F_OK) != 0)
    {
        write(1, "invalid file\n", 13);
        error(dataFDs);
        return NULL;
    } else {
        file = fopen(arg, "rb");
        if (file == NULL)
        {
            error(dataFDs);
            perror("File not found\n");
            return NULL;
        }
        ready(dataFDs);
    }


    while ((readAmount = fread(buffer, 1, BUFFER_SIZE, file)) > 0)
    {
        write(dataFDs, buffer, readAmount);
        sleep(5);
        readAmount = read(dataFDs, buffer, BUFFER_SIZE);
        if(strcmp(buffer, TTX) == 0) {
            //print("Terminated\n");
			fclose(file);
			exit(0);
		}
    }

    fclose(file);
	write(dataFDs, "quit\0", 4);
	exit(0);
	return NULL;
}


// Methods
void get(char *arg) {
    char buffer[BUFFER_SIZE];
    int readAmount = read(dataFD, buffer, BUFFER_SIZE);
    print("ID: ");
	write(1, buffer, readAmount);
	print("\n");
    read(dataFD, buffer, 6);
	if (strcmp(buffer, ERROR) == 0)
    {
        print("Error creating file on server\n");
        return;
    }
    else
    {
        FILE *file = fopen(arg, "wb");
        if (file == NULL)
        {
            perror("bad file\n");
            exit(EXIT_FAILURE);
        }
        while ((readAmount = read(dataFD, buffer, BUFFER_SIZE)) == BUFFER_SIZE)
        {
			fwrite(buffer, 1, readAmount, file);
			readAmount = read(dataFD, buffer, BUFFER_SIZE);
			if(strcmp(buffer, TTX) == 0) {
				//print("Terminated\n");
				printf("\n\n%s\n", arg);
				remove(arg);
				return NULL;
			}
		}

		fwrite(buffer, 1, readAmount, file);
        fclose(file);
    }
}
void put(char *arg) {
    char buffer[BUFFER_SIZE];
    int readAmount = read(dataFD, buffer, BUFFER_SIZE);
    print("ID: ");
    write(1, buffer, readAmount);
	print("\n");
    read(dataFD, buffer, BUFFER_SIZE);
    FILE *file;
	//write(1, arg, sizeof(arg));
    if(strcmp(buffer, ERROR) == 0) {
        print("Server failed to create file\n");
        return;
    }
    if (access(arg, F_OK) != 0)
    {
        write(1, "invalid file\n", 13);
        error(dataFD);
        return;
    } else {
        file = fopen(arg, "rb");
        if (file == NULL)
        {
            error(dataFD);
            perror("File not found\n");
            return;
        }
        ready(dataFD);
    }


    while ((readAmount = fread(buffer, 1, BUFFER_SIZE, file)) > 0)
    {
        write(dataFD, buffer, readAmount);
		readAmount = read(dataFD, buffer, BUFFER_SIZE);
        if(strcmp(buffer, TTX) == 0) {
            //print("Terminated\n");
        }
    }

    fclose(file);
}
void delete(char* arg) {
    char buffer[BUFFER_SIZE];
    int readAmount = read(dataFD, buffer, BUFFER_SIZE);
    if(strcmp(buffer, ERROR) == 0) {
        print("Server file delete error\n");
    } else {
        print("File deleted\n");
    }
}
void ls() {
    char buffer[BUFFER_SIZE];
    int readAmount = read(dataFD, buffer, BUFFER_SIZE);
    if(strcmp(buffer, ERROR) == 0) {
        print("Server file delete error\n");
    } else if(strcmp(buffer, READY) != 0) {
        write(1, buffer, readAmount);
        // Consume the READY message that signaled the end
        readAmount = read(dataFD, buffer, BUFFER_SIZE);
    }
}
void cd() {
    char buffer[BUFFER_SIZE];
    int readAmount = read(dataFD, buffer, BUFFER_SIZE);
    if(strcmp(buffer, READY) == 0) {
        print("Directory Changed\n");
    }
}
void mkdir() {
    char buffer[BUFFER_SIZE];
    int readAmount = read(dataFD, buffer, BUFFER_SIZE);
    if(strcmp(buffer, ERROR) == 0) {
        print("Server makedir error\n");
    } else if(strcmp(buffer, READY) == 0) {
        print("Directory Made\n");
    }
}
void pwd() {
    char buffer[BUFFER_SIZE];
    int readAmount = read(dataFD, buffer, BUFFER_SIZE);
    if(strcmp(buffer, ERROR) == 0) {
        print("Server pwd error\n");
    } else if(strcmp(buffer, READY) != 0) {
        print(buffer);
        // Consume the READY message that signaled the end
        readAmount = read(dataFD, buffer, BUFFER_SIZE);
    }
}
void terminate() {
    char buffer[BUFFER_SIZE];
    int readAmount = read(terminateFD, buffer, BUFFER_SIZE);
    if(strcmp(buffer, ERROR) == 0) {
        print("Already finished running\n");
    } else if(strcmp(buffer, READY) == 0) {
        print("terminated\n");
    }
}

// Starts the connection at address and port
int create_connection(char *address, int port) {
    int dataFD = socket(AF_INET, SOCK_STREAM, 0);
    if( dataFD < 0 ) {
        perror("Socket Creation Failed\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in sockaddr;
    memset(&sockaddr, '\0', sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    inet_pton(AF_INET, address, &sockaddr.sin_addr);

    if( connect(dataFD, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) <0 ) {
        perror("Create Connection Fail\n");
        exit(EXIT_FAILURE);
    }

    return dataFD;
}

// Main Loop
void handle_commands() {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    int readAmount;

    while(1) {
        char prompt[] = "myftp> ";
        write(1, prompt, sizeof(prompt));
        readAmount = read(0, buffer, BUFFER_SIZE);
        buffer[readAmount - 1] = '\0'; // -1 here to replace the \n

		char toUse[BUFFER_SIZE];
		strcpy(toUse, buffer);
		char *bufferPointer = toUse;
        // Seperates the input by ' '
        int argIndex = 0;
        char *arg[BUFFER_SIZE];
        while ((arg[argIndex] = strsep(&bufferPointer, " ")) != NULL)
        {
            if (arg[argIndex][0] == '\0')
            {
                argIndex--;
            }
            argIndex++;
        }

		bool inBackground = false;
        // Runs the relevant command based on the input
        if(argIndex != 0) {
            if(strcmp(arg[0], "quit") == 0) {
				write(dataFD, buffer, readAmount);
                exit(EXIT_SUCCESS);
            } else if (strcmp(arg[argIndex - 1], "&") == 0 && strcmp(arg[0], "get") == 0) {
				inBackground = true;
				int splitFD = create_connection(forsplit[0], atoi(forsplit[1]));
				int splitTerFD = create_connection(forsplit[0], atoi(forsplit[2]));
				// inherit execution directory via cd to the pwd of socketFDn
				char consumer[BUFFER_SIZE];
				int consume;
				char newbuf[BUFFER_SIZE];

				write(dataFD, "pwd\0", 4);
                int readAmountcd = read(dataFD, newbuf, BUFFER_SIZE);
				if(strcmp(newbuf, ERROR) == 0) {
					print("Server pwd error\n");
					continue;
				} else if(strcmp(newbuf, READY) != 0) {
					// Consume the READY message that signaled the end
					consume = read(dataFD, consumer, BUFFER_SIZE);
				}

				newbuf[readAmountcd - 1] = '\0';
                char cdbuf[BUFFER_SIZE] = "";
                strcpy(cdbuf, "cd ");
                strcat(cdbuf, newbuf);
				write(splitFD, cdbuf, 3 + readAmountcd);
				consume = read(splitFD, consumer, BUFFER_SIZE);

				write(splitFD, buffer, readAmount);
				int readAmountt = read(splitFD, newbuf, BUFFER_SIZE);
				print("ID: ");
				write(1, newbuf, readAmountt);
				print("\n");

				ThreadArgs *args = malloc(sizeof(ThreadArgs));
				args->socketFD = splitFD;
				args->file = arg[1];

				pthread_t thread;
				pthread_create(&thread, NULL, getsplit, (void *)args);
				continue;
			} else if (strcmp(arg[argIndex - 1], "&") == 0 && strcmp(arg[0], "put") == 0) {
				inBackground = true;
				int splitFD = create_connection(forsplit[0], atoi(forsplit[1]));
				int splitTerFD = create_connection(forsplit[0], atoi(forsplit[2]));
				// inherit execution directory via cd to the pwd of socketFDn
				char consumer[BUFFER_SIZE];
				int consume;
				char newbuf[BUFFER_SIZE];

				write(dataFD, "pwd\0", 4);
                int readAmountcd = read(dataFD, newbuf, BUFFER_SIZE);
				if(strcmp(newbuf, ERROR) == 0) {
					print("Server pwd error\n");
					continue;
				} else if(strcmp(newbuf, READY) != 0) {
					// Consume the READY message that signaled the end
					consume = read(dataFD, consumer, BUFFER_SIZE);
				}

				newbuf[readAmountcd - 1] = '\0';
                char cdbuf[BUFFER_SIZE] = "";
                strcpy(cdbuf, "cd ");
                strcat(cdbuf, newbuf);
				write(splitFD, cdbuf, 3 + readAmountcd);
				consume = read(splitFD, consumer, BUFFER_SIZE);

				write(splitFD, buffer, readAmount);
				int readAmountt = read(splitFD, newbuf, BUFFER_SIZE);
				print("ID: ");
				write(1, newbuf, readAmountt);
				write(1, "\n", 1);

				ThreadArgs *args = malloc(sizeof(ThreadArgs));
				args->socketFD = splitFD;
				args->file = arg[1];

				pthread_t thread;
				pthread_create(&thread, NULL, putsplit, (void *)args);

				continue;
			} else if (strcmp(arg[0], "terminate") == 0) {
				inBackground = true;
				write(dataFD, "checkterminatekey", 22);
				write(terminateFD, buffer, readAmount);
				terminate();
				continue;
            }

			write(dataFD, buffer, readAmount);
			if (inBackground == true) {
				inBackground = false;
			}else if (strcmp(arg[0], "get") == 0) {
                get(arg[1]);
            } else if (strcmp(arg[0], "put") == 0) {
                put(arg[1]);
            } else if (strcmp(arg[0], "delete") == 0) {
                delete(arg[1]);
            } else if (strcmp(arg[0], "ls") == 0) {
                ls();
            } else if (strcmp(arg[0], "cd") == 0) {
                cd();
            } else if (strcmp(arg[0], "mkdir") == 0) {
                mkdir();
            } else if (strcmp(arg[0], "pwd") == 0) {
                pwd();
            } else {
                char erorrMessage[] = "Invalid Command\n";
                write(1, erorrMessage, sizeof(erorrMessage));
            }
        }
    }
    write(dataFD, "Goodbye\n", sizeof("Goodbye\n"));
}


// Entrypoint
int main(int argc, char *argv[]) {
    if (argc < 4)
    {
        printf("Usage: ./myftp ADDRESS PORT TPORT\n");
        return EXIT_FAILURE;
    }
	forsplit[0] = argv[1];
	forsplit[1] = argv[2];
	forsplit[2] = argv[3];
    // Connects to server on the data port
    int dataPort = atoi(argv[2]);
    dataFD = create_connection(argv[1], dataPort);

    // Connects to server on the terminate port
    int terminatePort = atoi(argv[3]);
    terminateFD = create_connection(argv[1], terminatePort);

    handle_commands();
    exit(EXIT_SUCCESS);
}
