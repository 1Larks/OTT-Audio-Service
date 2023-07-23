#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>

#define PORT 31311
#define SERVER_BACKLOG 10
#define TRUE 1
#define FALSE 0
#define BUFFER_SIZE 200
#define MSGLEN 6

void error(const char* msg){
    perror(msg);
    exit(EXIT_FAILURE);}

//type 0-null, 1-user 2-admin 3-artist
struct client{
    int sock;
    int type;
    char name[16];
    char pass[16];
    long int bytesSent;
    int paused;
};

void resetClient(struct client* cli){
    cli->sock=0;
    cli->type=0;
    cli->paused=0;
    cli->bytesSent=0;
}

int initSockfd(struct sockaddr_in address){
    int sockfd=socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd<0){
        error("Error at initialising the server socket\n");
    }

    //setting up the server's address
    address.sin_family= AF_INET;
    address.sin_addr.s_addr=inet_addr("10.0.2.15");
    address.sin_port=htons(PORT);

    //binding
    if( (bind(sockfd, (struct sockaddr*) &address, sizeof(address)))<0 ){
        error("Error at binding\n");
    }

    return sockfd;
}

//TEMP
int login(char* info, struct client* user){
    int i=0;
    while (info[i]!=':'){
        i++;
    }
    strncpy(user->name, info, i);
    printf("%s\n", user->name);
    char* password=&(info[i+1]);
    strcpy(user->pass, password);
    printf("username: %s, pass: %s\n", user->name, user->pass);
    user->type=1;
    return 0;
}

int search(struct client* user, char* entry){
    FILE *songs=fopen("songs.txt", "r");
    char buffer[512];
    int numLines=0, keepReading=TRUE, currLine=0;
    char result[2048]="";
    if ( songs==NULL ){
        send(user->sock, "SRCERR", MSGLEN, 0);
        return 1;
    }
    while(keepReading)
    {
        fgets(buffer, 512, songs);

        if ( feof(songs) ){
            keepReading=FALSE;
        }else{

            if(strstr(buffer, entry)){
                int j=strlen(result), i;
                for(i=0;buffer[i]!='\0';i++){
                    result[i+j]=buffer[i];
                }
                result[i+j]='\0';
            }

        }

    }
    send(user->sock, "SRCSUC", MSGLEN, 0);
    send(user->sock, result, sizeof(result), 0);
    fclose(songs);
    return 0;
}

int playSong(struct client* user, char* ID, int ID_Len){
    char path[7+ID_Len], buffer[6144];
    strcat(path, "songs/");
    strcat(path, ID);
    FILE* song=fopen(path, "rb");
    if ( song==NULL ){
        return 1;
    }
    while ( !feof(song) && !user->paused)
    {
        fread(buffer, sizeof(buffer), 1, song);
        send(user->sock, buffer, sizeof(buffer), 0);
        user->bytesSent+=sizeof(buffer);
        fseek(song, user->bytesSent, SEEK_SET);
    }

    if ( !user->paused ){
        user->bytesSent=0;
    }
    send(user->sock, "Done", 4, 0);
    return 0;
}

//TODO: change login system- make it secure, create a LOGIN cmd for saving account info (mostly for client-side but still)
void handleCommands(struct client* user, char* buffer){
        //for unlogged clients
        if (user->type==0){
            if (login(buffer, user)!=0){
                if ( (send(user->sock, "LOGERR", MSGLEN,0)) <0){
                    error("Error at sending\n");
                }
            }
            else{
                if ( (send(user->sock, "LOGSUC", MSGLEN, 0)) <0){
                    error("Error at sending\n");
                }
            }
        }
        else{
            char cmd[6];
            strncpy(cmd, buffer, MSGLEN);
            //search command
            if ( strncmp(cmd,"SRCH:", 5)==0 )
            { 

                search(user, &buffer[5]);

            }
            if ( strncmp(cmd, "PLAY:", 5) )
            {
                user->paused=0;
                playSong(user, &buffer[5], sizeof(&buffer[5]));
                
                
            }

            }
        
}

int main(){
    //sd is socket descriptor
    int sockfd, addrlen, newSocket, activity, i, valread, sd, max_sd, readInfo;
    int opt=TRUE;
    struct client clientSocket[SERVER_BACKLOG];
    struct sockaddr_in address;

    char buffer[BUFFER_SIZE];

    fd_set readfds;
    
    //initialise all client sockets
    for (i=0;i<SERVER_BACKLOG; i++){
        resetClient(&clientSocket[i]);
    }

    //initialise server socket
    sockfd=initSockfd(address);

    if( setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ){  
        error("error in setsockopt");  
    }  

    //listening
    if (listen(sockfd, SERVER_BACKLOG)<0){
        error("Error at listening");
    }

    addrlen=sizeof(address);

    //TODO create an authentication system!
    while(TRUE){
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        max_sd=sockfd;

        //add child sockets to set
        for (i=0; i<SERVER_BACKLOG; i++){
            sd=clientSocket[i].sock;

            //if valid socket descriptor add to read list
            if (sd>0){
                FD_SET(sd, &readfds);
            }

            //highest file descriptor num
            if(sd>max_sd){
                max_sd=sd;
            }
        }

        activity=select(max_sd+1, &readfds, NULL, NULL, NULL);
        if (activity<0 && errno!=EINTR){
            error("Error at select\n");
        }

        //if something happened on the server sock then its an incoming connection
        if(FD_ISSET(sockfd, &readfds)){

            if( (newSocket=accept(sockfd, (struct sockaddr*)&address, (socklen_t*)&addrlen))<0 ){
                error("Error at accept\n");
            }

            //printing connection info
            printf("New connection, sock: %d\nip: %s\nport: %d\n",
             newSocket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            for (i=0;i<SERVER_BACKLOG;i++){
                if (clientSocket[i].sock==0){
                    clientSocket[i].sock=newSocket;
                    break;
                }
            }

        }
        for (i = 0; i < SERVER_BACKLOG; i++)  
        {  
            sd = clientSocket[i].sock;  
                 
            if (FD_ISSET( sd , &readfds))  
            {  
                bzero(buffer, strlen(buffer));
                //Check if it was for closing , and also read the 
                //incoming message 
                if ((valread = read( sd , buffer, BUFFER_SIZE)) == 0)  
                {  
                    //Somebody disconnected , get his details and print 
                    getpeername(sd , (struct sockaddr*)&address , \
                        (socklen_t*)&addrlen);  
                    printf("Client disconnected , ip %s , port %d \n" , 
                          inet_ntoa(address.sin_addr) , ntohs(address.sin_port));  
                         
                    //Close the socket and mark as 0 in list for reuse 
                    close( sd );  
                    resetClient(&clientSocket[i]);  
                }  
                //Handle command
                else 
                {  
                    printf("Recieved a msg from client: %s\n", buffer);
                    //set the string terminating NULL byte on the end 
                    //of the data read 
                    buffer[valread] = '\0';  
                    handleCommands(&clientSocket[i], buffer);
                }  
            }  
        }
    }

    return 0;
}


