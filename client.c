#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <sys/epoll.h>
#define THREAD_NUM   2
#define PER_THREAD_MAX_SOCKET_FD 5000
#define MAX_EVENTS 5000
static int processClient();
int main(int argc,char *argv[])
{
	int i = 0;
	pthread_t tid[THREAD_NUM];
	for ( i; i<THREAD_NUM; ++i)
		pthread_create(&tid[i], NULL, (void*)&processClient, NULL);
		
	while(1)
	{
		sleep(1000000);
	}
	return 0;
}

int processClient()
{
	printf("enter thread id is :%lu\n",pthread_self());
	int fd[PER_THREAD_MAX_SOCKET_FD] = {0};
	int ret;
	struct sockaddr_in addr = {0};
	struct in_addr x;
	inet_aton("127.0.0.1",&x);
	addr.sin_family = AF_INET;
	addr.sin_addr = x;
	addr.sin_port = htons(12006);
     	int set = 30;
	int i = 0;
	int fdFlag = 0;
	pthread_detach(pthread_self()); 	
	int epollFd = epoll_create(MAX_EVENTS);
	if(epollFd==-1)  
    	{  
       		printf("epoll_create failed\n");  
        	return epollFd;  
    	}  

	struct epoll_event ev;// epollÊ¼þ½ṹÌ  
	struct epoll_event events[MAX_EVENTS];// Ê¼þ¼à¶ÓÐ 
	for( i; i<PER_THREAD_MAX_SOCKET_FD; ++i )
	{	
     		fd[i] = socket(AF_INET,SOCK_STREAM,0);
     		if(fd[i] == -1)
     		{
         		printf("error:%s\n",strerror(errno));
         		return fd[i];
		}
		// set timer is valid ?
		setsockopt(fd[i], SOL_SOCKET, SO_KEEPALIVE, &set, sizeof(set));
		// set socket non block?
		if ((fdFlag = fcntl(fd[i], F_GETFL, 0)) < 0)
        		printf("F_GETFL error");
    		fdFlag |= O_NONBLOCK;
    		if (fcntl(fd[i], F_SETFL, fdFlag) < 0)
        		printf("F_SETFL error");
		//
     		ret = connect(fd[i],(struct sockaddr*)&addr,sizeof(addr));
     		if(ret == -1)
     		{
			if (errno == EINPROGRESS )
			{
         			printf(" connect error:%s\n",strerror(errno));
				continue;
			}
			else
			{
				printf(" connect error:%s\n",strerror(errno));
         			return fd[i];
			}
	     	}
	}
	for( i=0; i<PER_THREAD_MAX_SOCKET_FD; ++i )
	{
		ev.events=EPOLLOUT;  
		ev.data.fd=fd[i];  
		if(epoll_ctl(epollFd,EPOLL_CTL_ADD,fd[i],&ev)==-1)  
		{  
			printf("epll_ctl:server_sockfd register failed");  
			return -1;  
		}  

     	}
	printf("enter first part\n");
	int nfds;
     	int token_length = 5;
     	char* token_str = "12345";
     	char* ch = "yumgkevin";
	char sendbuf[512] = {0};
	char recvbuf[5120] = {0};
	char socketId[10] = {0};
	while(1)  
	{  
		
		printf("enter second part\n");
        	nfds=epoll_wait(epollFd,events,MAX_EVENTS,-1);  
        	if(nfds==-1)  
	        {  
	        	printf("start epoll_wait failed");  
            		return -1;  
        	}  
	        printf("tid is %lu,active event number is %d\n",pthread_self(), nfds); 
        	for(i=0;i<nfds;i++)  
        	{
/* 
			if ((events[i].events & EPOLLERR) ||
              		(events[i].events & EPOLLHUP) ||
              		 (!(events[i].events & EPOLLIN)) ||
              		 (!(events[i].events & EPOLLOUT)) ) 
	    		{
				printf("enter 1");
	      			fprintf (stderr, "epoll error\n");
	      			close (events[i].data.fd);
	      			continue;
	    		}
			*/
 			if (events[i].events & EPOLLOUT)
			{		
				printf("enter 2");
				memset(sendbuf,0,sizeof(sendbuf));
				memset(socketId,0,sizeof(socketId));
				
     				strcpy(sendbuf,token_str);
				strcat(sendbuf,"hellow, world");
				strcat(sendbuf,ch);
				sprintf(socketId,"%d",events[i].data.fd);
				strcat(sendbuf,socketId);
				strcat(sendbuf,"\r\n");
				
     				ret = send(events[i].data.fd,sendbuf,strlen(sendbuf),0);
     				if(ret == -1)
     				{
					if (errno != EAGAIN)
					{
        					printf("error:%s\n",strerror(errno));
						close(events[i].data.fd);
					}
         				continue;
     				}
				printf("send buf content is %s, size is %d\n", sendbuf, ret);
				// add revelant socket read event
				ev.data.fd = events[i].data.fd;
			       	ev.events = EPOLLIN|EPOLLET;
				epoll_ctl(epollFd,EPOLL_CTL_MOD,events[i].data.fd,&ev);
			}
 			else if (events[i].events & EPOLLIN)
			{
				printf("enter 3");
				int count = 0;
                  		memset(recvbuf, 0 , sizeof(recvbuf));
                  		count = recv(events[i].data.fd, recvbuf, sizeof(recvbuf), 0);
                  		if (count == -1)
                    		{
                      			/* If errno == EAGAIN, that means we have read all
                         		data. So go back to the main loop. */
                      			if (errno != EAGAIN)
                        		{
                          			printf("read error\n");
                          			close(events[i].data.fd);
                        		}
                      			continue;
                    		}
                  		else if (count == 0)
                    		{	
                     			 /* End of file. The remote has closed the
                         		connection. */
                      			close(events[i].data.fd);
                      			continue;
                    		}
				printf("receive data is:%s",recvbuf);
				// add revelant socket write event
				ev.data.fd = events[i].data.fd;
			       	ev.events = EPOLLOUT;
				epoll_ctl(epollFd,EPOLL_CTL_MOD,events[i].data.fd,&ev);
			}
		}
	}
	printf("enter 4");
	for( i=0; i<PER_THREAD_MAX_SOCKET_FD; ++i)
		close(fd[i]);
	return 0;
}			
