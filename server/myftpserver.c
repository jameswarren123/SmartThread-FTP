#include "header.h"

#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"
#include "sys/types.h"
#include "sys/wait.h"
#include "sys/stat.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "string.h"
#include "pthread.h"
#include "fcntl.h"
#include "math.h"
#include <asm-generic/socket.h>

// Global Variables
#define QUEUE_SIZE 3
int dataSocket;
int terminateSocket;
int clientDataFD;
int clientTerminateFD;
int idTable[BUFFER_SIZE];
// Mutex locks
char fileNames[100][256];
int filesEnd = 0;
int readerCount[100];
int waitCount[100];
pthread_mutex_t fileLocks[100];
pthread_cond_t fileConds[100];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void deletes(char *name);
int getFile(char *name);

int getID() {
    pthread_mutex_lock(&lock);
    int index = 1;
    while (idTable[index] != 0)
    {
        index = (index + 1) % BUFFER_SIZE;
    }
    idTable[index] = 1;
    pthread_mutex_unlock(&lock);
    return index;
}
// Methods
void get(char *name, int id) {
    int index = getFile(name);
    pthread_mutex_lock(&fileLocks[index]);
    waitCount[index]++;
    while (readerCount[index] == -1) {
        pthread_cond_wait(&fileConds[index], &fileLocks[index]);
    }
    waitCount[index]--;
    readerCount[index]++;
    pthread_mutex_unlock(&fileLocks[index]);
    char buffer[BUFFER_SIZE];
    // The get method is always going to run in a seperate thread.
    sprintf(buffer, "%d", id);
	write(clientDataFD, buffer, (int)((ceil(log10(id))+1)*sizeof(char)));
    pid_t pid;
    if( (pid = fork()) == 0 ) {
        if( access(name, F_OK) != 0 ) {
            error(clientDataFD);
            exit(EXIT_FAILURE);
        }
        ready(clientDataFD);
        // Opens file and sends bytes through socket
		int file = open(name, O_RDONLY);
		ssize_t bytesRead;
        while( (bytesRead = read(file, buffer, BUFFER_SIZE)) > 0 ) {
			write(clientDataFD, buffer, bytesRead);
			sleep(5);
            // Checks to see if it should be terminated
            if(idTable[id] == 0) {
				idTable[id] = 1;
                close(file);
                write(clientDataFD, TTX, sizeof(TTX));
                exit(EXIT_SUCCESS);
            }
        }
        close(file);
        idTable[id] = 0;
        exit(EXIT_SUCCESS);
    }
    waitpid(pid, NULL, 0);
    pthread_mutex_lock(&fileLocks[index]);
    readerCount[index]--;
    if (readerCount[index] == 0) {
        pthread_cond_broadcast(&fileConds[index]);
    }
    pthread_mutex_unlock(&fileLocks[index]);
    return;
}
void put(char *name, int id) {
    int index = getFile(name);
    pthread_mutex_lock(&fileLocks[index]);
    waitCount[index]++;
    while (readerCount[index] != 0) {
        pthread_cond_wait(&fileConds[index], &fileLocks[index]);
    }
    waitCount[index]--;
    readerCount[index] = -1;
    pthread_mutex_unlock(&fileLocks[index]);
    char buffer[BUFFER_SIZE];
    // The put method is always going to run in a seperate thread.
    sprintf(buffer, "%d", id);
    write(clientDataFD, buffer, (int)((ceil(log10(id))+1)*sizeof(char)));
    pid_t pid;
    if( (pid = fork()) == 0 ) {
        // Opens file and sends bytes through socket
        FILE *file = fopen(name, "w");
        if ( file == NULL ) {
            write(1, "Failed to create file\n", strlen("Failed to create file\n"));
            error(clientDataFD);
            exit(EXIT_FAILURE);
        } else {
            ready(clientDataFD);
        }

        ssize_t bytesRead;
        bytesRead = read(clientDataFD, buffer, 6);
        if( strcmp(buffer, ERROR) == 0 ) {
            write(1, "Client failed to read\n", strlen("Client failed to read\n"));
            exit(EXIT_FAILURE);
        }
		write(1, buffer, strlen(buffer) * 2);
        while( (bytesRead = read(clientDataFD, buffer, BUFFER_SIZE)) == BUFFER_SIZE ) {
			write(1, "here\n", 5);
            // Writes to file
            fwrite(buffer, bytesRead, 1, file);
            // Writes to console
            write(1, buffer, bytesRead);
            memset(buffer, 0, BUFFER_SIZE);

			printf("%d\n", id);
            // Checks to see if it should be terminated
            if(idTable[id] == 0) {
                idTable[id] = 1;
                fclose(file);
                deletes(name);
                write(clientDataFD, TTX, sizeof(TTX));
                exit(EXIT_SUCCESS);
            }
            ready(clientDataFD);
        }
        // Have to do one more write for the remaining little bit.
        // Writes to file
        fwrite(buffer, bytesRead, 1, file);
        // Writes to console
        write(1, buffer, bytesRead);
        memset(buffer, 0, BUFFER_SIZE);
        ready(clientDataFD);

        // Close the file
        fclose(file);

        idTable[id] = 0;
        exit(EXIT_SUCCESS);
    }
    waitpid(pid, NULL, 0);
    pthread_mutex_lock(&fileLocks[index]);
    readerCount[index] = 0;
    pthread_cond_broadcast(&fileConds[index]);
    pthread_mutex_unlock(&fileLocks[index]);
    return;
}
void deletes(char *name) {
    int index = getFile(name);
    pthread_mutex_lock(&fileLocks[index]);
    waitCount[index]++;
    while (readerCount[index] != 0) {
        pthread_cond_wait(&fileConds[index], &fileLocks[index]);
    }
    waitCount[index]--;
    readerCount[index] = -1;
    pid_t pid;
    if( (pid = fork()) == 0 ) {
        char *delete_arg[] = {"rm", name, NULL};
        execvp("rm", delete_arg);
        error(clientDataFD);
        perror("execvp rm");
        exit(EXIT_FAILURE);
    }
    int exitStatus;
    waitpid(pid, &exitStatus, 0);
    readerCount[index]--;
    if (readerCount[index] == 0) {
        pthread_cond_broadcast(&fileConds[index]);
    }
    pthread_mutex_unlock(&fileLocks[index]);
    ready(clientDataFD);
}
int getFile(char *fileName) {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < filesEnd; i++) {
        if(strcmp(fileName, fileNames[i]) == 0) {
            pthread_mutex_unlock(&lock);
            return i;
        }
    }
    strcpy(fileNames[filesEnd], fileName);
    filesEnd++;
    readerCount[filesEnd] = 0;
    waitCount[filesEnd] = 0;
    pthread_mutex_init(&fileLocks[filesEnd], NULL);
    pthread_cond_init(&fileConds[filesEnd], NULL);
    pthread_mutex_unlock(&lock);
    return filesEnd - 1;
}
void ls() {
    pid_t pid;
    if( (pid = fork()) == 0 ) {
        dup2(clientDataFD, STDOUT_FILENO);
        close(clientDataFD);
        char *arg[] = {"ls", "-1", NULL};
        execvp(arg[0], arg);
        error(clientDataFD);
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    int exitStatus;
    waitpid(pid, &exitStatus, 0);
    ready(clientDataFD);
}
void cd(char *dir) {
    chdir(dir);
    ready(clientDataFD);
}
void mksdir(char *name) {
    pid_t pid;
    if((pid = fork()) == 0) {
        dup2(clientDataFD, STDOUT_FILENO);
        close(clientDataFD);
        char *arg[] = {"mkdir", name, NULL};
        execvp("mkdir", arg);
        error(clientDataFD);
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    int exitStatus;
    waitpid(pid, &exitStatus, 0);
    ready(clientDataFD);
}
void pwd() {
    pid_t pid;
    if((pid = fork()) == 0) {
        dup2(clientDataFD, STDOUT_FILENO);
        close(clientDataFD);
        char *arg[] = {"pwd", NULL};
        execvp("pwd", arg);
        error(clientDataFD);
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    int exitStatus;
    waitpid(pid, &exitStatus, 0);
    ready(clientDataFD);

}
void terminate(int id) {
	printf("%d\n", id);
    if(idTable[id] != 0) {
        idTable[id] = 0;
        ready(clientTerminateFD);
    } else {
        error(clientTerminateFD);
    }
}

void handle_commands() {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    int readAmount;
	int termReadAmount;
    // Reads whatever is sent over from the client
    while( (readAmount = read(clientDataFD, buffer, BUFFER_SIZE - 1)) != 0 ) {
		if(strcmp(buffer, "checkterminatekey") == 0) {
			termReadAmount = read(clientTerminateFD, buffer, BUFFER_SIZE -1);
			buffer[termReadAmount] = '\0';
			char *arg[2];
			char *bufPoint = buffer;
			arg[0] = strsep(&bufPoint, " ");
			arg[1] = strsep(&bufPoint, " ");
			write(1, "\n\n\n\n", 4);
			write(1, arg[1], strlen(arg[1]));
			write(1, "\n", 1);
			terminate(atoi(arg[1]));
			continue;
		}

        buffer[readAmount] = '\0';
		write(1, buffer, readAmount);
		write(1, "\n", 1);
        char *temp = strdup(buffer);
        char *bufferPointer = temp;

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

        // Runs the relevant command based on the input
        if(argIndex != 0) {
            if(strcmp(arg[0], "quit") == 0) {
                write(1, "Client Disconnected\n", sizeof("Client Disconnected\n"));
                exit(EXIT_SUCCESS);
            } else if (strcmp(arg[0], "get") == 0) {
                int id = getID();
                get(arg[1], id);
            } else if (strcmp(arg[0], "put") == 0) {
                int id = getID();
                put(arg[1], id);
            } else if (strcmp(arg[0], "delete") == 0) {
                deletes(arg[1]);
            } else if (strcmp(arg[0], "ls") == 0) {
                ls();
            } else if (strcmp(arg[0], "cd") == 0) {
                cd(arg[1]);
            } else if (strcmp(arg[0], "mkdir") == 0) {
                mksdir(arg[1]);
            } else if (strcmp(arg[0], "pwd") == 0) {
                pwd();
            } else if (strcmp(arg[0], "terminate") == 0) {
                terminate(atoi(arg[1]));
            } else {
                char erorrMessage[] = "Invalid Command\n";
                write(1, erorrMessage, sizeof(erorrMessage));
            }
        }
    }
    write(1, "Client Disconnected Ungracefully\n", sizeof("Client Disconnected Ungracefully\n"));
}

// Main Loop
void handle_connections() {
    struct sockaddr_in clientAddr;
    socklen_t clientAddrlen = sizeof(clientAddr);

    // Loops forever.
    while(1) {
        // Waits for connections from the data and socket ports
        if((clientDataFD = accept(dataSocket, (struct sockaddr*) &clientAddr, &clientAddrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }
        write(1, "Client Connected to Data Port\n", sizeof("Client Connected to Data Port\n"));
        if((clientTerminateFD = accept(terminateSocket, (struct sockaddr*) &clientAddr, &clientAddrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }
        write(1, "Client Connected to Terminate Port\n", sizeof("Client Connected to Terminate Port\n"));

        // Forks to create a process to handle everything with the client
        pid_t pid;
        if( (pid = fork()) == 0 ) {
            write(1, "child\n", sizeof("child\n"));
            handle_commands();
        }
    }

}

// Sets up the sockets for accepting clients
int open_socket(int port) {
    int socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if( socketFD < 0 ) {
        perror( "Socket Creation Failed" );
        exit(EXIT_FAILURE);
    }
    setsockopt( socketFD, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int) );
    setsockopt( socketFD, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int) );

    struct sockaddr_in sockaddr;
    memset(&sockaddr, '\0', sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if( bind(socketFD,(struct sockaddr*) &sockaddr, sizeof(sockaddr)) < 0 ) {
        perror("Bind Failed");
        exit(EXIT_FAILURE);
    }

    if( listen(socketFD, QUEUE_SIZE) < 0 ) {
        perror("Listen Failed");
        exit(EXIT_FAILURE);
    }

    return socketFD;
}

// Entrypoint
int main(int argc, char *argv[]) {
    if (argc < 3)
    {
        printf("Usage: ./myftpserver PORT TPORT\n");
        return EXIT_FAILURE;
    }
    // Initialize mutex
    pthread_mutex_init(&lock, NULL);

    // Opens data port for connections
    int dataPort = atoi(argv[1]);
    dataSocket = open_socket(dataPort);

    // Opens terminate port for connections
    int terminatePort = atoi(argv[2]);
    terminateSocket = open_socket(terminatePort);

    handle_connections();
    exit(EXIT_SUCCESS);
}
