
#include "servutils.h"

int init_server_socket(struct sockaddr_in address){
    int sockfd=socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd<0){
        ServerErr("Error at initialising the server socket\n");
    }
    //setting up the server's address
    address.sin_family= AF_INET;
    address.sin_addr.s_addr=inet_addr("10.0.2.15");
    address.sin_port=htons(PORT);

    //binding
    if( (bind(sockfd, (struct sockaddr*) &address, sizeof(address)))<0 ){
        ServerErr("Error at binding\n");
    }

    return sockfd;
}

//TODO: change login system- make it secure, create a LOGIN cmd for saving account info (mostly for client-side)
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
    sockfd=init_server_socket(address);

    if( setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ){  
        ServerErr("error in setsockopt");  
    }  

    //listening
    if (listen(sockfd, SERVER_BACKLOG)<0){
        ServerErr("Error at listening");
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
            ServerErr("Error at select\n");
        }

        //if something happened on the server sock then its an incoming connection
        if(FD_ISSET(sockfd, &readfds)){

            if( (newSocket=accept(sockfd, (struct sockaddr*)&address, (socklen_t*)&addrlen))<0 ){
                ServerErr("Error at accept\n");
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


