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

#define CONTROL_PORT "3482"  // the port users will be connecting to
#define DATA_PORT "3491"
#define BACKLOG 10   // how many pending connections queue will hold

const int MAX_FILE_LENGTH = 256;
const int MAX_NUM_DIR = 65535;

void parseCommand(char* buffer, char* port, char* command)
{
    strcpy(command, (strtok(buffer, "/")));
    strcpy(port, (strtok(NULL, "/")));
}

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
    return filenames;
}

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
int receiveCommand(int new_fd, char* buffer){
        return recv(new_fd, buffer, 20, 0);

}

int main(void)
{
   // char* filenames = stepThroughDir();

   // printf("%s", filenames);

    int controlSock;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int controlRV, dataRV, rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

   if ((rv = getaddrinfo(NULL, CONTROL_PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    
        // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((controlSock = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(controlSock, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(controlSock, p->ai_addr, p->ai_addrlen) == -1) {
            close(controlSock);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }
    if (listen(controlSock, BACKLOG) == -1) {
    perror("listen");
    exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");
    char* filenames;
    char buffer[257];
    char port[10];
    char command[257];
    memset(port, 0, 10);
    memset(command, 0, 257);
    memset(buffer, 0, 257);

    int new_fd;
    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(controlSock, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
            close(controlSock); // child doesn't need the listener
            if (receiveCommand(new_fd, buffer))
                perror("send");
            parseCommand(buffer, port, command);
            /* child process accept 

            *
            * 
            * */
            int sockfd, numbytes;  
    struct addrinfo hints, *servinfo, *p;
    int rv;


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(s, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure


            if(strcmp(command, "listdir") == 0){
                filenames = stepThroughDir();
                int size = strlen(filenames);
                send(sockfd, &size, 4, 0);
            }
            close(new_fd);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this
    }

    return 0;
}