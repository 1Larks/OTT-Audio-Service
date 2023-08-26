#include "servutils.h"

void ServerErr(const char* msg){
    perror(msg);
    exit(EXIT_FAILURE);
}
void ClientErr(const char* msg, struct client* user){
    perror(msg);
    //TODO: handle the errors at client side
    send(user->sock, msg, sizeof(msg), 0);
}

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

void resetClient(struct client* user){
    user->sock=0;
    user->song_connection=0;
    user->type=0;
    user->paused=0;
    user->bytesSent=0;
    user->thread=0;
    user->state=0;
}


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

void lowerCase(char* str){
    for ( int i=0;i<strlen(str)+1;i++ ){
        str[i]=tolower(str[i]);
    }
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
    lowerCase(entry);
    while(keepReading)
    {
        fgets(buffer, 512, songs);
        char lowerBuffer[strlen(buffer)+1];
        strcpy(lowerBuffer, buffer);
        lowerCase(lowerBuffer);
        if ( feof(songs) ){
            keepReading=FALSE;
        }else{
            if(strstr(lowerBuffer, entry)){
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

void send_song_info(FILE* song, struct client* user){
    char wav_header[44];
    fread(wav_header, 1, sizeof(wav_header), song);

    send(user->sock, wav_header, sizeof(wav_header), 0);
}

int playSong(char* ID, struct client* user){
    
    int ID_Len=strlen(ID);

    // Path- the song's path
    // Buffer- the buffer that will be sent and updated every iteration

    char path[11+ID_Len], buffer[6144];
    // Add the song's path
    strcat(path, "songs/");
    strcat(path, ID);
    strcat(path, ".wav");

    // Open the song on read bytes mode
    FILE* song=fopen(path, "rb");

    // Send the song's information to the client (the WAV header, which is 44 bytes long)
    send_song_info(song, user);

    // Check that the song exists in the path mentioned above
    if ( song==NULL ){
        return 1;
    }
    
    if ( user->bytesSent > 0 && strcmp(ID, user->lastSongID) == 0 ){
        // If the function is called after the song has been paused and not finished by itself, and the song is the same as last,
        // start playing the song from the point the user stopped
        fseek(song, user->bytesSent, SEEK_SET);
    }

    while (!user->paused)
    {
        // Read the information from the wav file and stores it in the buffer
        fread(buffer, 1, sizeof(buffer), song);
        // Sends the buffer to the user
        send(user->sock, buffer, sizeof(buffer), 0);
        // Adds the size of the buffer to the user bytesSent field, for tracking time and more useful information
        user->bytesSent+=sizeof(buffer);

        // After sending the buffer, the client needs to tell the server to continue, that creates syncronization and helps to
        // recieve and send more useful information like telling the client if the song had ended
        
        // read(user->sock, &sync, sizeof(sync));
        // sync[5]='\0';

        printf("%d\n", user->sock);
        // We need to check if the song has ended before we continue normally,
        // The reason that I did it that way instead of nesting it in the other if statements is that nesting it in them would be
        // less efficent and it's more easy to read that way
        if ( feof(song) )
        {
            // Song has ended, let the client know and end the function. - old
            //send(user->sock, "FNISH", 5, 0);
            break; 
        }
        // wait for the client to send a "COTNU" or "PAUSE" for syncronization purposes.
        while ( user-> state == 0 ){}
        if ( user-> state == 1 ){
            user->state=0;
            continue;
        }
        // If the client sends PAUSE instead of COTNU, that means that the client has requested to pause the song and the state is 2
        else if ( user->state == 2){
            user->state=0;
            user->paused=1;
            printf("Paused\n");
            //send(user->sock, "COTNU", 5, 0);
            break;
        }

    }
    // Song is over
    if ( !user->paused ){
        user->bytesSent=0;
    }
    fclose(song);
    bzero(buffer, BUFFER_SIZE);
    bzero(path, strlen(path));
    bzero(user->lastSongID, strlen(user->lastSongID));
    strcpy(user->lastSongID, ID);
    return 0;
}

void* thread_playSong(void* external_args){

    struct play_song_args* arguments= external_args;
    char ID[IDLEN];
    strcpy(ID, arguments->ID);
    
    struct client* user=arguments->user;

    if ( playSong(ID, user) == 1 ){
        ServerErr("Error in playing song.\n");
    }
    free(external_args);
}

void handleCommands(struct client* user, char* buffer){
        //for unlogged clients

        if (user->type==0){
            if (login(buffer, user)!=0){
                if ( (send(user->sock, "LOGERR", MSGLEN,0)) <0){
                    ServerErr("Error at sending\n");
                }
            }
            else{
                if ( (send(user->sock, "LOGSUC", MSGLEN, 0)) <0){
                    ServerErr("Error at sending\n");
                }
            }
        }
        else{
            char cmd[6];
            strncpy(cmd, buffer, MSGLEN);
            if ( strncmp(cmd, "COTNU", MSGLEN-1) == 0 ){
                user->state=1;
            }
            else if ( strncmp(cmd, "PAUSE", MSGLEN-1) == 0){
                user->state=2;
            }
            //search command
            if ( strncmp(cmd,"SRCH:", MSGLEN-1)==0 )
            { 

                search(user, &buffer[MSGLEN-1]);

            }
            if ( strncmp(cmd, "PLAY:", MSGLEN-1)==0 )
            {
                user->paused=0;
                
                struct play_song_args* arguments=(struct play_song_args*) malloc(sizeof(struct play_song_args));
                //bzero(arguments->ID, strlen(arguments->ID));
                strcpy(arguments->ID,&buffer[MSGLEN-1]);
                arguments->user=user;
                printf("ID: %s\n", arguments->ID);
                if ( (pthread_create(&user->thread, NULL, thread_playSong, arguments)) != 0 ){
                    ServerErr("Error in creating song thread.\n");
                }
                
                
            }

        }
        bzero(buffer, strlen(buffer));
        
}
