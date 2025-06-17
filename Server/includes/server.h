#ifndef _SERVER_H_
#define _SERVER_H_

extern pthread_mutex_t board_lock;

#define PORT 5566
#define MAX_BUFFER 10000
#define MAX_CONNECTIONS 2


extern int serverSocket;



#endif