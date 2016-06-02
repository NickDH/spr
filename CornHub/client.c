#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <mqueue.h>


#define MAX_SIZE 1024
int main(){
	mqd_t mq;
    char buffer[MAX_SIZE + 1];
    int ret;
    mq = mq_open("/to_server_q", O_WRONLY);
    sprintf( buffer, "/cq%i", getpid());
    if((mqd_t)-1 == mq){
    	printf("Error opening queue\n");
    	printf( "Error: %s\n", strerror( errno));
    	return -1;
    }

    printf("%s\n",buffer);
    ret = mq_send(mq,buffer,MAX_SIZE,0);
    if(ret){
    	printf("Error sending message\n");
    	printf( "Error: %s\n", strerror( errno));
    }
    mqd_t mq_read,mq_write;
    struct mq_attr attr;
    attr.mq_maxmsg = 10;
	attr.mq_msgsize=MAX_SIZE;
	struct mq_attr attr1;
    attr1.mq_maxmsg = 10;
	attr1.mq_msgsize=MAX_SIZE;
	char read_name[MAX_SIZE],write_name[MAX_SIZE];
	strcpy(read_name,buffer);
	strcat(read_name,"_fs");
	strcpy(write_name,buffer);
	strcat(write_name,"_ts");
	int turns,i,bytes_read;

	mq_read = mq_open(read_name, O_CREAT | O_RDONLY, 0644, &attr);

	if((mqd_t)-1 == mq_read){
    	printf("Error opening queue for reading.\n");
    	printf( "Error: %s\n", strerror( errno));
    	return -1;
    }
    printf("opened read queue\n");
    mq_write = mq_open(write_name, O_CREAT | O_WRONLY, 0644, &attr1);
	if((mqd_t)-1 == mq_read){
    	printf("Error opening queue for writing.\n");
    	printf( "Error: %s\n", strerror( errno));
    	return -1;
    }
    printf("opened write queue\n");
    turns = rand()%100;
    printf("start comunication\n");
 
    for( i = 0; i<=turns; i++){
    	

    	char tmpbuf[65];
    	sprintf(tmpbuf, "%d", rand()%10);

    	strcpy(buffer,"GET:");
    	strcat(buffer, tmpbuf);

    	printf("loop: %d\n",i);

    	printf("%s\n",buffer);

    	ret = mq_send(mq_write,buffer,MAX_SIZE,0);
    	if(ret){
	    	printf("Error sending message\n");
	    	printf( "Error: %s\n", strerror( errno));
	    }
	    bytes_read = mq_receive(mq_read,buffer, MAX_SIZE, NULL);
	    if(bytes_read==-1){
    		printf("Error getting message\n");
    		printf( "Error: %s\n", strerror( errno));
    	}
    	else{
    		printf("%s\n",buffer);
    	}
    }
    ret = mq_send(mq_write,"EXIT",MAX_SIZE,0);
    	if(ret){
	    printf("Error sending message\n");
	    printf( "Error: %s\n", strerror( errno));
	    }
  	if(mq_unlink(read_name) == 0)
  			printf( "Message queue  removed from system: %s\n",read_name);
  	else{
  		printf("error unlinking mq: %s\n",read_name);
  		printf( "Error: %s\n", strerror( errno));
  	}
  	if(mq_unlink(write_name) == 0)
  			printf( "Message queue  removed from system: %s\n",write_name);
  	else{
  		printf("error unlinking mq: %s\n",write_name);
  		printf( "Error: %s\n", strerror( errno));
  	}
	return 0;
}