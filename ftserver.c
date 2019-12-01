/*
* Name: Andrew Freitas
* OSID: Freitand
* Project 2
* 12/01/2019
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

const int MAX_FILE_LENGTH = 256;
const int MAX_NUM_DIR = 65535;
/*
* Parses the command and sets the 
* different variables dependent on the command
*/
void parseCommand(char* buffer, char* port, char* command, char* filename)
{
    strcpy(command, (strtok(buffer, "/")));
    if(strstr(command, "-g") != NULL)
        strcpy(filename, strtok(NULL, "/"));
    strcpy(port, (strtok(NULL, "/")));
}
/*
* Steps through the current directory and parses all
* of the filenames by appending each with a "/" for sending
* and returns a string containing all filenames
*/
char* stepThroughDir()
{
    char* filenames = malloc(MAX_FILE_LENGTH * MAX_NUM_DIR * sizeof(char));
    memset(filenames, 0, sizeof filenames);

    DIR* currentDir;
    struct dirent *fileInDir;
    int x = 0;

    currentDir = opendir(".");
    if(currentDir > 0){
        while((fileInDir = readdir(currentDir)) != NULL)
        {
            if(fileInDir->d_type != DT_DIR){
                strcat(filenames, fileInDir->d_name);
                strcat(filenames, "/");
            }
        }

    }
    filenames[strlen(filenames) - 1] = 0; // remove he final "/" for formatting later
    return filenames;
}
/*
* This was taken from Beej's guide
* This sets up the child's signal handler
* and may not be necessary for this project depending on
* whether it continues to fork or not.
*/
void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}
/*
* This was taken from Beej's guide
* and is a helper function for connection
*
*/
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
/*
* This function receives data from the connection 
* and inserts it into the buffer given
* and returns the amount of data received
*/
int receiveCommand(int new_fd, char* buffer){
    return recv(new_fd, buffer, 20, 0);

}
/*
* This function takes in three parameters
* and sends data to new_fd, with a size, 
* and buffer, returning the amount of data sent
*/
int sendData(int new_fd, int size, void* buffer){
    return send(new_fd, buffer, size, 0);
    
}
/*
* This code was slightly modified from Beej's guide
* to ensure that the proper pointer or reference is passed in
* and is intended to further breakup the code for reuse
*/
void bindHostConnection(int* controlSock, int yes, struct addrinfo **p, struct addrinfo *servinfo)
{
    for(*p = servinfo; *p != NULL; *p = (*p)->ai_next) {
        if ((*controlSock = socket((*p)->ai_family, (*p)->ai_socktype,
                (*p)->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(*controlSock, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(*controlSock, (*p)->ai_addr, (*p)->ai_addrlen) == -1) {
            close(*controlSock);
            perror("server: bind");
            continue;
        }

        break;
    }
    if (*p == NULL)  {
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
    }

}
/*
* This code was slightly modified from Beej's guide
* to ensure that the proper pointer or reference is passed in
* and is intended to further breakup the code for reuse
*/
int bindClientConnection(int* sockfd, struct addrinfo **p, struct addrinfo *servinfo)
{

    for(*p = servinfo; *p != NULL; *p = (*p)->ai_next) {
        if ((*sockfd = socket((*p)->ai_family, (*p)->ai_socktype,
                (*p)->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(*sockfd, (*p)->ai_addr, (*p)->ai_addrlen) == -1) {
            close(*sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }
    if (*p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

}
/*
* This is taken from Beej's guide
* and is intended to setup the program
* to handle forking and may be removed if forking is removed
*/
void sigchildSetup(struct sigaction sa)
{
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

}
/*
* This is taken from Beej's guide
* and is intended to setup the socket
* for the server
*/
void socketSetup(struct addrinfo *hints, int isHost)
{

    memset(hints, 0, sizeof *hints);
    hints->ai_family = AF_UNSPEC;
    hints->ai_socktype = SOCK_STREAM;
    hints->ai_flags = AI_PASSIVE; // use my IP
}
/*
* This checks to make sure that the 
* client is sending an approved command to the control
* socket and returns true or false otherwise
*/
int validCommand(char* command)
{
    if(strcmp(command, "-l") == 0 || strcmp(command, "-g") == 0)
        return 1;
    return 0;
}
/*
* This function sends the directory listing 
* of the current working directory
* to the client to the socket provided
*/
void sendListDir(int socket)
{
    char* filenames;
    filenames = stepThroughDir();
    int size = strlen(filenames);
    sendData(socket, 4, &size);
    sendData(socket, strlen(filenames), filenames);
    free(filenames);
}
/*
* This function sends the file using the filename
* and socket given to the function
* and returns 1 if the file cannot be found
*/
int sendFile(int controlSocket, int dataSocket, char* filename)
{
    int total;
    char buffer[512], totalbuffer[512], *result;
    FILE* file;
    file = fopen(filename, "r");
    total = 0;
    if(file == NULL)
    {
        return 1;
    }
    sendData(controlSocket, 2, "ok");

    fseek(file, 0 , SEEK_END);
    unsigned fileSize = ftell(file);
    printf("The file length is: %d", fileSize);
    sendData(dataSocket,4, &fileSize);
    rewind(file);
    while((result = fgets(buffer, 512, file)) != NULL)
    {
        sendData(dataSocket, strlen(buffer), buffer);
    }
    return 0;

}
int main(int argc, char* argv[])
{
    printf("Server open on %s\n", argv[1]);
    
    struct sigaction sa;
    sigchildSetup(sa);

    int controlConnection, controlSocket;
    struct addrinfo hints, *servinfo, *p, *clientinfo, *q;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int controlRV, dataRV, rv, dataSocket, numbytes;


    socketSetup(&hints, 1);

    if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    bindHostConnection(&controlConnection, yes, &p, servinfo);

    freeaddrinfo(servinfo); // all done with this structure


    if (listen(controlConnection, 1) == -1) {
    perror("listen");
    exit(1);
    }


    char* filenames;
    char buffer[257], port[10], command[257], filename[MAX_FILE_LENGTH];
    memset(filename, 0, MAX_FILE_LENGTH);
    memset(port, 0, 10);
    memset(command, 0, 257);
    memset(buffer, 0, 257);

    while(1) { 
        sin_size = sizeof their_addr;
        controlSocket = accept(controlConnection, (struct sockaddr *)&their_addr, &sin_size);
        if (controlSocket == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);

        char hostname[1024];
        char service[20];
        getnameinfo(&their_addr,sizeof their_addr, hostname, sizeof hostname, service, sizeof service, 0);
        printf("Connection from: %s\n", hostname);



        receiveCommand(controlSocket, buffer);
        parseCommand(buffer, port, command, filename);

        if(validCommand(command)){
            memset(buffer, 0, 257);
            strcpy(buffer, "ok");
            sendData(controlSocket, 2, buffer);
        }
        else{
            memset(buffer, 0, 257);
            sendData(controlSocket, 2, "no"); // this is lost but is necessary to line up sending calls
            strcpy(buffer, "INVALID COMMAND");
            unsigned size = strlen(buffer);
            sendData(controlSocket, 4, &size);
            sendData(controlSocket, strlen(buffer), buffer);
            continue;
        }
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        if ((rv = getaddrinfo(s, port, &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return 1;
        }

        // loop through all the results and connect to the first we can

        bindClientConnection(&dataSocket, &q, servinfo);

        inet_ntop(q->ai_family, get_in_addr((struct sockaddr *)q->ai_addr),
                s, sizeof s);

        freeaddrinfo(servinfo); // all done with this structure

        if(strcmp(command, "-l") == 0)
        {
            printf("Sending directory contents to %s:%s\n", hostname, port);

            sendListDir(dataSocket);
            
        }
        if(strcmp(command, "-g") == 0)
        {
            printf("File %s requested on port %s\n", filename, port);
            if(sendFile(controlSocket, dataSocket, filename) == 1)
            {
                sendData(controlSocket, 2, "no"); // this is lost but necessary because the client is expecting a message

                printf("File not found. Sending error message to %s:%s\n", hostname, argv[1]);
                char errorMessage[15] = "FILE NOT FOUND";
                unsigned length = strlen(errorMessage);
                sendData(controlSocket, 4, &length);
                sendData(controlSocket, length, errorMessage);
            }
        }
    close(dataSocket);
    }
        return 0;
}