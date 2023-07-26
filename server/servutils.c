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

void resetClient(struct client* user){
    user->sock=0;
    user->type=0;
    user->paused=0;
    user->bytesSent=0;
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

int playSong(struct client* user, char* ID, int ID_Len){
    // Path- the song's path
    // Buffer- the buffer that will be sent and updated every iteration
    // Sync- Every iteration the sync buffer will be used to syncronize the server with the client and serve for other useful things
    char path[11+ID_Len], buffer[6144], sync[6];
    // Add the song's path
    strcat(path, "songs/");
    strcat(path, ID);
    strcat(path, ".wav");
    // Open the song on reaad bytes mode
    FILE* song=fopen(path, "rb");
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
        bzero(sync, strlen(sync));
        read(user->sock, &sync, sizeof(sync));
        
        // We need to check if the song has ended before we continue normally,
        // The reason that I did it that way instead of nesting it in the other if statements is that nesting it in them would be
        // less efficent and it's more easy to read that way
        if ( feof(song) )
        {
            // Song has ended, let the client know and end the function.
            //send(user->sock, "FNISH", 5, 0);
            break; 
        }
        if ( strcmp(sync, "COTNU") == 0 ){
             // If the server recieved "COTNU" (short for continue), send back to the user the same msg,
             // This is useful because it can be used to tell the client when the song has ended and for getting more control
             
             //send(user->sock, "COTNU", 5, 0);
             continue;
        }
        // If the client sends PAUSE instead of COTNU, that means that the client has requested to pause the song
        else if ( strcmp(sync, "PAUSE") == 0){
            user->paused=1;
            //send(user->sock, "COTNU", 5, 0);
            break;
        }
    }
    // Song is over
    if ( !user->paused ){
        user->bytesSent=0;
    }
    fclose(song);
    user->lastSongID=ID;
    return 0;
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
            //search command
            if ( strncmp(cmd,"SRCH:", 5)==0 )
            { 

                search(user, &buffer[5]);

            }
            if ( strncmp(cmd, "PLAY:", 5)==0 )
            {
                user->paused=0;
                playSong(user, &buffer[5], sizeof(&buffer[5]));
            }

        }
        
}
