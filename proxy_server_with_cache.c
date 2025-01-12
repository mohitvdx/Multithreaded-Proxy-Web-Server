#include "proxy_parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

typedef struct cache_element cache_element;
#define MAX_CLIENTS 10

struct cache_element{
    char* data;
    int len;
    char* url;
    time_t lru_time_track;
    cache_element* next; 
};

cache_element* find(char* url);
int add_cache_element(char* data, int len, char* url);
void remove_cache_element();

int port_number = 8080;
int proxy_socketId;
pthread_t tid[MAX_CLIENTS];
sem_t semaphore;
pthread_mutex_t lock;

cache_element* head;
int cache_size;

int main(int argc, char* argv[]){
    int client_socketId, client_len;
    struct sockaddr_in server_addr, client_addr;
    sem_init(&semaphore, 0, MAX_CLIENTS);
    pthread_mutex_init(&lock, NULL);

    if(argv == 2){
        port_number = atoi(argv[1]);
    }else{
        printf("Too few arguments \n");
        exit(1);
    }

    printf("Starting proxy server at port: %d\n", port_number);
    proxy_socketId = socket(AF_INET, SOCK_STREAM, 0);
    if(proxy_socketId<0){
        perror("Failed to create a socket \n");
        exit(1);
    }
    int reuse =1;
    if(setsockopt(proxy_socketId, SOL_SOCKET, SO_REUSEADDR, (const char*)& reuse, sizeof(reuse))<0){
        perror("setsockopt failed \n");
    }

    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if( bind(proxy_socketId, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 ){
		perror("Port is not free\n");
		exit(1);
	}
	printf("Binding on port: %d\n",port_number);

	int listen_status = listen(proxy_socketId, MAX_CLIENTS);
	if(listen_status < 0 ){
		perror("Error while Listening !\n");
		exit(1);
	}

    int i=0;
    int Connected_socketId[MAX_CLIENTS];

    while(1){
        bzero((char *)&client_addr, sizeof(client_addr));
        client_len = sizeof(client_addr);
        client_socketId = accept(proxy_socketId, (struct sockaddr*)&client_addr, (socklen_t*)&client_len);
        
        if(client_socketId < 0){
			fprintf(stderr, "Error in Accepting connection !\n");
			exit(1);
		}
    }
}

