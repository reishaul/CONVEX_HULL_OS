#ifndef PROACTOR_HPP
#define PROACTOR_HPP

#include <pthread.h>


//typedef void *(* proactorFunc) (int fd);
typedef void* (*proactorFunc) (int sockfd);

// starts new proactor and returns proactor thread id
pthread_t startProactor (int sockfd, proactorFunc threadFunc);

// stops proactor
int stopProactor(pthread_t tid);


#endif