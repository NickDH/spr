#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <mqueue.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#define MAX_SIZE 1024

pthread_mutex_t mutex;
int corns;
const int max_corns=100000;
pthread_t *corn_thread,producer_thread;
int thread_count = 0, thrd_arr_size = 500,is_terminated=0;
mqd_t mq;


void* produce_corn(void* arg){
	corns = 0;
	while(1){
		pthread_mutex_lock(&mutex);
		if(corns +10 < max_corns){
			corns+=10;
		}
		pthread_mutex_unlock(&mutex);
		sleep(2);
	}
}

void* new_client(void* cl_pid){
	int bytes_read,ret;
	int corn_num;
	mqd_t mq_read,mq_write;
    struct mq_attr attr;
    attr.mq_maxmsg = 10;
	attr.mq_msgsize=MAX_SIZE;
	struct mq_attr attr1;
    attr1.mq_maxmsg = 10;
	attr1.mq_msgsize=MAX_SIZE;


	char read_name[MAX_SIZE],write_name[MAX_SIZE],buffer[MAX_SIZE + 1];
	char* exit_str = "EXIT";
	strcpy(read_name,(char*)cl_pid);
	strcat(read_name,"_ts");
	strcpy(write_name,(char*)cl_pid);
	strcat(write_name,"_fs");

	struct stat st;
	stat(read_name, &st);
	if(errno != ENOENT)
	{
		mq_unlink(read_name);
	}

	stat(write_name, &st);
	if(errno != ENOENT)
	{
		mq_unlink(write_name);
	}

	mq_read = mq_open(read_name, O_CREAT|O_RDONLY, 0644, &attr);
	if((mqd_t)-1 == mq_read){
    	printf("Error opening queue for reading for \n");
    	printf( "Error: %s\n", strerror( errno));
    	return ;
    }

    mq_write = mq_open(write_name, O_CREAT|O_WRONLY, 0644, &attr1);
	if((mqd_t)-1 == mq_read){
    	printf("Error opening queue for writing.\n");
    	printf( "Error: %s\n", strerror( errno));
    	return ;
    }
    do{
    	bytes_read = mq_receive(mq_read,buffer, MAX_SIZE, NULL);
	    if(bytes_read==-1){
    		printf("Error getting message for %s\n",read_name);
    		printf( "Error: %s\n", strerror( errno));
    	}
    	if(!strcmp(buffer,exit_str)){
    		break;
    	}
    	corn_num = atoi(&buffer[4]);


    	
    	do{
    		pthread_mutex_lock(&mutex);
    		if(corns-corn_num >= 0){
    			corns-=corn_num;
    			pthread_mutex_unlock(&mutex);
    			break;
    		}
    		else{
    			pthread_mutex_unlock(&mutex);
    			sleep(5);
    		}
    	}while(corns-corn_num < 0);

    	char tmpbuf[65];
    	sprintf(tmpbuf, "%d", corn_num);

    	strcpy(buffer,"SEND:");
    	strcat(buffer, tmpbuf);

    	ret = mq_send(mq_write,buffer,MAX_SIZE,0);
    	if(ret == -1){
	    	printf("Error sending message for %s\n",write_name);
	    	printf( "Error: %s\n", strerror( errno));
	    }

    }while(1);
    printf("Closing thread for: %s\n",(char*)cl_pid);

}
void sig_handler(int signo)
{
	is_terminated=1;
	int i;
	for(i=0;i<thread_count;i++){
		pthread_join(corn_thread[i],NULL);
	}
	free(corn_thread);
	if(mq_unlink("/to_server_q") == 0)
  			printf( "Message queue  removed from system.\n");
  	printf("Server closed.\n");
  	exit(signo);
}

int main(){

	char buffer[MAX_SIZE];
	struct mq_attr attr;
	signal(SIGINT, sig_handler);

	attr.mq_maxmsg = 10;
	attr.mq_msgsize=MAX_SIZE;

	corn_thread = (pthread_t *) malloc(thrd_arr_size * sizeof(pthread_t));

	if( pthread_create( &producer_thread , NULL ,  produce_corn , NULL) != 0)
        {
            printf("could not create producer_thread : %s\n",strerror( errno));
            return 1;
        }

	mq = mq_open("/to_server_q", O_CREAT|O_RDONLY, 0644, &attr);
	if((mqd_t)-1 == mq){
    	printf("Error opening queue\n");
    	printf( "Error: %s\n", strerror( errno));
    }

    do{
    	ssize_t bytes_read;
    	sleep(2);
		bytes_read = mq_receive(mq,buffer, MAX_SIZE, NULL);
		printf("%d\n",bytes_read);
		if(bytes_read==-1){
    		printf("Error getting message\n");
    		printf( "Error: %s\n", strerror( errno));
    	}
		printf("%s\n",buffer);
		if(thrd_arr_size - thread_count == 1)
		{
			thrd_arr_size *= 2;
			corn_thread = (pthread_t *) realloc(corn_thread, thrd_arr_size * sizeof(pthread_t));
		}
		printf("Creating thread for %s\n",buffer);
		if(!is_terminated){
			if( pthread_create( &corn_thread[thread_count] , NULL ,  new_client , (void*)buffer) != 0)
	        {
	            printf("could not create thread for %s : %s\n",buffer,strerror( errno));
	            break;
	        }
    	}

        ++thread_count;
    }while(1);
    
	return 0;
}