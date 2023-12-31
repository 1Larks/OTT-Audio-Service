#ifndef SERVUTILS_H
#define SERVUTILS_H

#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <ctype.h>
#include <pthread.h>

#define IP "192.168.1.54"
#define PORT 31311
#define SERVER_BACKLOG 10
#define TRUE 1
#define FALSE 0
#define BUFFER_SIZE 200
#define MSGLEN 6
#define IDLEN 5

//type 0-null, 1-user 2-admin 3-artist
//A struct for the client (user), has the user's socket, user type, name, password and some other information
struct client{
    int sock;
    int type;
    char name[16];
    char pass[16];
    long int bytesSent;
    int paused;
    char lastSongID[IDLEN];
    // Each user is given a thread for playing music, I chose to use threading because a regular function with a while loop
    // would block the server
    pthread_t thread;
    // The state variable was created for syncronization purposes, when it's equal to 0 it means its waiting for the
    // client to send either "COTNU" or "PAUSE". 1 is for continue and 2 is for pause
    int state;
};

struct play_song_args{
    struct client* user;
    char ID[IDLEN];
};

//Error Handeling:
//If an error occured on the server itself, close the server and log the error
void ServerErr(const char* msg);
//If an error occured on the client side, no need to close the server, so send the client it's error and let it handle it
void ClientErr(const char* msg, struct client* user);

//Init the server socket, should be used only once and in the main server file
int init_server_socket(struct sockaddr_in address);

//Resets user's relevant data
void resetClient(struct client* user);

//Temporary login system
int login(char* info, struct client* user);

//A functions that gets the user's entry and returns the results that match for that entry
int search(struct client* user, char* entry);

//A functions that sends chunks of the song's information, every chunk is 6 kilobytes, currently under some work
int playSong(char* ID, struct client* user);

void send_song_info(FILE* song, struct client* user);

void* thread_playSong(void* external_args);

//Get the user who sent the command and the command itself and handles it by selecting the right function for the command
void handleCommands(struct client* user, char* buffer);



#endif