#include "com_akkessler_pspider_pspider.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <android/log.h>

// stupid eclipse/CDT how do you not know what NULL is?
#ifndef NULL
#define NULL   ((void *) 0)
#endif

#define APPNAME "PSPIDER"

const int BUFFER_SIZE = 128;
const int RESP_SIZE = 1024;
const int RES_SIZE = 256*256;
const int MIN_READ = 16;
const char * HTTP_OK = "HTTP/1.1 200 OK";
const char * HREF_TAG1 = "<a href=\"http://";
const char * HREF_TAG2 = "<a href=\"www";

typedef struct WebAddress {
	char address[64];
	int depth;
	int visited;
	struct WebAddress * next;
	struct WebAddress * prev;
} WebAddress;

// DLL with a tail pointer
WebAddress * front;
WebAddress * back;

pthread_mutex_t lock;

char * seed;
char * port = "80";
int sock_fd; // no need to be global anymore
char * HOST_NAME;

int placesToVisit = 0;
int maxDepth = 0;

int size = 0; // size of queue

void push_queue(WebAddress * wa){
	if(back == NULL){
		front = wa;
		back = front;
		back->next = NULL;
		back->prev = NULL;
	}
	else{
		back->prev = wa;
		wa->next = back;
		wa->prev = NULL;
		back = wa;
	}
	size++;
}

WebAddress * pop_queue(){
	WebAddress * tmp = front;
	if(tmp == NULL) return NULL;
	size--;
	if(front == back){
		front = NULL;
		back = NULL;
		return tmp;
	}
	front = front->prev;
	front->next = NULL;
	tmp->prev = NULL;
	return tmp;
}

void parseHTML(char * resp, int dep){
	if(dep >= maxDepth) return;
	char * curr = resp;
	char * beginTag, * endTag;
	if(strstr(resp, HTTP_OK) != NULL){
		while( curr && (beginTag = strstr(curr, HREF_TAG1)) ){
			endTag = strstr(beginTag, ">");
			if(endTag){
				WebAddress * wa = (WebAddress *) calloc(1, sizeof(WebAddress));
				wa->depth = dep + 1;
				if(dep+1 >= maxDepth) wa->visited = 1;
				else wa->visited = 0;
				char * token;
				const char s[2] = "\"";
				token = strtok(beginTag, s);
				token = strtok(NULL, s); // +7 gets rid of http://
				strncpy(wa->address, token, 63);
				int k = strlen(token);
				if(k > 64) wa->address[64]='\0';
				else wa->address[k]='\0';

				__android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "READ: %s\n", wa->address);

				pthread_mutex_lock(&lock);

				__android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "IN HERE - %d\n", placesToVisit);

				push_queue(wa);
				if(!wa->visited) placesToVisit++;

				pthread_mutex_unlock(&lock);
			}
			curr = endTag;
		}
	}
	else{__android_log_print(ANDROID_LOG_VERBOSE, "%s\n", "IN HERE - HTTP_ERROR");}
	__android_log_print(ANDROID_LOG_VERBOSE, "%s\n", "IN HERE - EXITING PARSEHTML");
}

//worker thread
void * peon(void * zugzug){
	__android_log_print(ANDROID_LOG_VERBOSE, "%s\n", "IN HERE - ZUGZUG");
	while(placesToVisit){
		pthread_mutex_lock(&lock);
		//if(placesToVisit > 100) return NULL;
		if(placesToVisit > 0){
			WebAddress * wa;
			wa = pop_queue();
			while(wa->visited){
				push_queue(wa);
				wa = pop_queue();
			}

			placesToVisit--;

			pthread_mutex_unlock(&lock);

			struct addrinfo hints, *result;
			memset(&hints, 0, sizeof(struct addrinfo));
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM;

			int s = getaddrinfo(seed, port, &hints, &result);
			if(s != 0){
				__android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "getaddrinfo: %s\n", gai_strerror(s));
				return NULL;
			}
			int sfd;
			if((sfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol)) == -1){
				__android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "Socket Error!\n");
				return NULL;
			}
			if(connect(sfd, result->ai_addr, result->ai_addrlen) == -1){
				__android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "%d\n", sfd);
				close(sfd);
				return NULL;
			}

			freeaddrinfo(result);
			// !!!!!!!!!!!!

			char * a = strstr(wa->address, HOST_NAME);
			if(a != NULL){
				a = a+strlen(HOST_NAME)+1;
				char buffer[128] = "GET /";
				strcat(buffer, a);
				strcat(buffer, " HTTP/1.1\r\nHost: ");
				strcat(buffer, HOST_NAME);
				strcat(buffer, "\r\n\r\n");
				write(sfd, buffer, strlen(buffer));
				//__android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "%s\n", "afterbuffer");
				char resp[RESP_SIZE];
				char res[RES_SIZE];
				//int len = read(sfd, resp, RESP_SIZE-1);
				//while(strstr(res, "</html>") == NULL){
				int len;
				int length;
				while((length < RES_SIZE - 256) && ((len = read(sfd, resp, RESP_SIZE-1)) > MIN_READ)){
					resp[len]='\0';
					length += len;
					strcat(res, resp);
					//len=read(sfd, resp, RESP_SIZE-1);
				}
				close(sfd);

				//__android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "IN HERE IS THE RES: %s\n", resp);
				parseHTML(res, wa->depth);
				//push_queue(wa);
			}
			else{
				close(sfd);
			}
			wa->visited=1;
			pthread_mutex_lock(&lock);
			push_queue(wa);
			//close(sfd);
		}
		else
		{
			pthread_mutex_unlock(&lock);
			break;
		}
		pthread_mutex_unlock(&lock);
	}
	//close(sfd);
	__android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "PEON GOING TO SLEEP\n");
}

JNIEXPORT jstring JNICALL Java_com_akkessler_pspider_pspider_nativeMain
	(JNIEnv * env, jobject obj, jstring address, jint depth, jint thread_ct){

	front = back = NULL;

	pthread_mutex_init(&lock, NULL);

	struct addrinfo hints, *result, *p;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	//char *
	seed = (*env)->GetStringUTFChars(env, address, NULL);
	if(!seed) return (*env)->NewStringUTF(env, "GetStringUTFChars error with seed URL.");
	HOST_NAME = seed+4; // no www.
	maxDepth = (int) depth;
	int spiders = (int) thread_ct;

	int s = getaddrinfo(seed, port, &hints, &result);
	if(s != 0){
		__android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "getaddrinfo: %s\n", gai_strerror(s));
		return (*env)->NewStringUTF(env, "Invalid seed address.");
	}
	for(p=result; p != NULL; p=p->ai_next){
		if((sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
			__android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "Socket Error!\n");
			return (*env)->NewStringUTF(env, "Socketing error.");
		}

		if(connect(sock_fd, p->ai_addr, p->ai_addrlen) == -1){
			__android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "%d\n", sock_fd);
			close(sock_fd);
			continue;
		}
	}

	freeaddrinfo(result);

	char res[RES_SIZE];
	char buffer[128] = "GET / HTTP/1.1\r\nHost: ";
	strcat(buffer, HOST_NAME);
	strcat(buffer, "\r\n\r\n");
	write(sock_fd, buffer, strlen(buffer));

	char resp[RESP_SIZE];
	int len;
	len = read(sock_fd, resp, RESP_SIZE-1);
	while(strstr(resp, "</html>") == NULL){
		resp[len]='\0';
		strcat(res, resp);
		len = read(sock_fd, resp, RESP_SIZE-1);
	}
	resp[len]='\0';
	strcat(res, resp);
	close(sock_fd);
	parseHTML(res, 0);


	pthread_t * threads = calloc(spiders, sizeof(pthread_t));
	int i;
	for(i=0; i<spiders; i++){
		pthread_create(& threads[i], NULL, peon, NULL);
	}

	for(i=0; i<spiders; i++){
		pthread_join(threads[i], NULL);
	}

	(*env)->ReleaseStringUTFChars(env, address, seed);

	pthread_mutex_destroy(&lock);
	free(threads);

	char ret[size*2*64]; // to prevent buffer overflow -> SEGFAULT
	WebAddress * x = pop_queue();
	while(x != NULL){
		/*
		unsigned char size[3];
		sprintf(size, "%03d", x->depth);
		strcat(ret, size);
		strcat(ret, " : ");
		*/
		strcat(ret, x->address);
		strcat(ret, "\n");
		free(x);
		x = pop_queue();
	}
	return (*env)->NewStringUTF(env, ret);
}


