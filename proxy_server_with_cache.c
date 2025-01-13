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
#define MAX_BYTES 4096 

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

int connectRemoteServer(char* host_addr, int port_num){

    int remoteSocket = socket(AF_INET, SOCK_STREAM, 0);

    if(remoteSocket<0){
        printf("Error in creating socket. \n");
        return -1;
    }

    struct hostent *host = gethostbyname(host_addr);
    if(host==NULL){
        fprintf(stderr, "No such host exists.\n");
        return -1;
    }

    struct sockaddr_in server_addr;

    bzero((char*) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_num);

    bcopy((char*) host->h_addr, (char*)&server_addr.sin_addr.s_addr, host->h_length);

    if(connect(remoteSocket, (struct sockaddr*)&server_addr, (socklen_t)sizeof(server_addr))<0){
        fprintf(stderr, "Error in connecting !\n");
        return -1;
    }
    return remoteSocket;

}

int handle_request(int clientSocket, ParsedRequest *request, char *tempReq){
    char *buf = (char*)malloc(sizeof(char)*MAX_BYTES);
    strcpy(buf, "GET ");
    strcat(buf, request->path);
    strcat(buf, " ");
    strcat(buf, request->version);
    strcat(buf, "\r\n");

    size_t len = strlen(buf);

    if(ParsedHeader_set(request, "Connection", "close")<0){
        printf("set header key not working \n");
    }

    if(ParsedHeader_get(request,"Host")==NULL){
        if(ParsedHeader_set(request, "Host", request->host)<0){
            printf("Set \"Host\" header key not working\n");
        }
    }

    if(ParsedRequest_unparse_headers(request, buf+len, (size_t)MAX_BYTES-len)<0){
        printf("unparse failed\n");
    }

    int server_port = 80;
    if(request -> port != NULL)
        server_port=atoi(request->port);

    int remoteSocketID = connectRemoteServer(request->host, server_port);

    if(remoteSocketID<0)
        return -1;

    int bytes_send = send(remoteSocketID, buf, strlen(buf), 0);

    bzero(buf, MAX_BYTES);

    bytes_send = recv(remoteSocketID, buf, MAX_BYTES-1, 0);
    char *temp_buffer = (char*)malloc(sizeof(char)*MAX_BYTES);
    int temp_buffer_size = MAX_BYTES;
    int temp_buffer_index=0;

    while(bytes_send>0){
        bytes_send = send(clientSocket, buf, bytes_send, 0);

        for(int i=0; i<bytes_send/sizeof(char); i++){
            temp_buffer[temp_buffer_index]=buf[i];
            temp_buffer_index++;
        }
        temp_buffer_size += MAX_BYTES;
        temp_buffer=(char*)realloc(temp_buffer, temp_buffer_size);

        if(bytes_send<0){
            perror("Error in sending data to client socket. \n");
            break;
        }
        bzero(buf, MAX_BYTES);
        bytes_send = recv(remoteSocketID, buf, MAX_BYTES-1, 0);

    }

    temp_buffer[temp_buffer_index]='\0';
    free(buf);
    add_cache_element(temp_buffer, strlen(temp_buffer), tempReq);
    printf("Done \n");
    free(temp_buffer);

    close(remoteSocketID);
    return 0;
}

void* thread_fn(void* socketNew){
    sem_wait(&semaphore);
    int p;
    sem_getvalue(&semaphore, &p);
    printf("semaphore value: %d\n", p);

    int* t = (int*)(socketNew);
    int socket=*t;
    int bytes_send_client, len;

    char *buffer = (char*)calloc(MAX_BYTES, sizeof(char));
    bzero(buffer, MAX_BYTES);
    
    bytes_send_client = recv(socket, buffer, MAX_BYTES, 0);

    while(bytes_send_client>0){
        len = strlen(buffer);
        if(strstr(buffer, "\r\n\r\n") == NULL){
            bytes_send_client = recv(socket, buffer+len, MAX_BYTES - len, 0);
        }else{
            break;
        }
    }

    char *tempReq = (char*)malloc(strlen(buffer)*sizeof(char)+1);

    for(int i=0; i<strlen(buffer); i++){
        tempReq[i]=buffer[i];
    }

    struct cache_element* temp = find(tempReq);

    if(temp != NULL){
        int size= temp->len/sizeof(char);
        int pos=0;
        char response[MAX_BYTES];
        while(pos<size){
            bzero(response, MAX_BYTES);
            for(int i=0; i<MAX_BYTES; i++){
                response[i]=temp->data[pos];
                pos++;
            }
            send(socket, response, MAX_BYTES, 0);
        }
        printf("Data retrieved from the cache\n");
        printf("%s\n\n",response);

    }else if(bytes_send_client>0){

        len = strlen(buffer);
        ParsedRequest *request = ParsedRequest_create();

        if(ParsedRequest_parse(request, buffer, len)<0){
            printf("Parsing failed\n");
        }else{
            bzero(buffer, MAX_BYTES);
            if(!strcmp(request->method, "GET")){
                if(request -> host && request->path && (checkHTTPversion(request->version)==1)){
                    bytes_send_client = handle_request(socket, request, tempReq);
                    if(bytes_send_client==-1){
                        sendErrorMessage(socket, 500);
                    }
                }
                else
                    sendErrorMessage(socket, 500);
        }
            else{
        printf("This code doesn't support any method other than GET\n");
   }
        }
    ParsedRequest_destroy(request);
    
	}

    else if(bytes_send_client <0){
        perror("Error in receiving from client. \n");
    }else if(bytes_send_client==0){
        printf("Client disconnected \n");
    }

    shutdown(socket, SHUT_RDWR);
    close(socket);
    free(buffer);
    sem_post(&semaphore);
    sem_getvalue(&semaphore, &p);
    printf("Semaphore post value: %d\n",p);
    free(tempReq);
    return NULL;

}

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
		}else{
            Connected_socketId[i]=client_socketId;
        }

        struct sockaddr_in * client_pt = (struct sockaddr_in *)&client_addr;
        struct in_addr ip_addr = client_pt->sin_addr;
        char str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &ip_addr, str, INET_ADDRSTRLEN);
        printf("Client is connected with port number %d and ip address is %s\n",ntohs(client_addr.sin_port), str);

        pthread_create(&tid[i], NULL, thread_fn, (void*)&Connected_socketId[i]);
        i++;
    }
    close(proxy_socketId);
    return 0;
}

