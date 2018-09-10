// Use this to see if a number has an integer square root
#define EPS 1.E-7


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <math.h>
#include "point.h"

int num;
int maxmsg;
int num_p;
int num_c;
double g_time[2];
mqd_t queue;
mqd_t status_queue;

/* Create the message queue that take in the size and the queue name */
mqd_t createMq(int queue_size, char *queue_name){

	mqd_t qdes;
	char *qname = queue_name;
	mode_t mode = S_IRUSR | S_IWUSR;
	struct mq_attr attr;
	attr.mq_maxmsg  = queue_size;
	attr.mq_msgsize = sizeof(struct point);
	attr.mq_flags   = 0;
	qdes  = mq_open(qname, O_RDWR | O_CREAT, mode, &attr);
	if (qdes == -1 ) {
		perror("mq_open() failed");
		exit(1);
	}
	return qdes;
}

/* Close the message queue */
void closeMq(mqd_t qde){
	if (mq_close(qde) == -1) {
		perror("mq_close() failed");
		exit(2);
	}
}

/* Unlink the message queue */
void unlinkMq(char *qname){
	if (mq_unlink(qname) != 0) {
		perror("mq_unlink() failed");
		exit(3);
	}
}

/* Send item in the message queue as a point struct with two values
The first value store the item number and the second value store the
status number. Status number indicate how many items have been received */
int send(mqd_t qdes, int x, int y){
	struct point pt;
	set_position(x, y, &pt);
	if (mq_send(qdes, (char *)&pt, sizeof(struct point), 0) == -1) {
		perror("mq_send() failed");
		exit(1);
	}
	return 0;
}

/* Receive a item value for an object from the message queue */
int receiveItem(mqd_t qdes){
	struct point pt;
	if (mq_receive(qdes, (char *)&pt, sizeof(struct point), 0) == -1) {
		perror("mq_receive() failed");
		exit(1);
	}
	return get_x_coord(pt);
}

/* Receive a status number for an object from the message queue */
int receiveStatus(mqd_t qdes){
	struct point pt;
	if (mq_receive(qdes, (char *)&pt, sizeof(struct point), 0) == -1) {
		perror("mq_receive() failed");
		exit(1);
	}
	return get_y_coord(pt);
}

/* Calculate the root of the item value and determine if it can be
square rooted and then display them */
void display(int cid, int number){
	double root = sqrt(number);
	if(round(root) == root){
		printf("%d %d %d\n", cid, number, (int)root);
	}
}

/* Producer classify the item number using module and then send them into
the main message queue */
int producer(int pid){
	int i;
	for(i = 0; i < num; i++){
		if(i % num_p == pid){
			send(queue, i, 0);
		}
	}
	exit(0);
	return 0;
}

/* Consumer receive an item number from the main message queue. Before
that, consumer first receive status number in the status queue and check
if status number decreases to zero. If so, it means all items have been
received, then break the loop */
int consumer(int cid){
	int item;
	int status;
	while(1){
		status = receiveStatus(status_queue);
		send(status_queue, 0, status - 1);
		if(status <= 0){
			exit(0);
		}
		item = receiveItem(queue);
		display(cid, item);
	}
	return 0;
}

/* Spawn child processes for producers */
int spawnProducer(int pid){
	pid_t child_pid;
	child_pid = fork();
	if (child_pid < 0){
		printf("fork() failed at producer %d\n", pid);
		exit(1);
	}
	else if(child_pid == 0){
		producer(pid);
	}
	return 0;
}

/* Spawn child processes for consumers */
int spawnConsumer(int cid){
	pid_t child_pid;
	child_pid = fork();
	if (child_pid < 0){
		printf("fork() failed at consumer %d\n", cid);
		exit(1);
	}
	else if(child_pid == 0){
		consumer(cid);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	int i;
	struct timeval tv;

	if (argc != 5) {
		printf("Usage: %s <N> <B> <P> <C>\n", argv[0]);
		exit(1);
	}

	num = atoi(argv[1]);																			/* number of items to produce */
	maxmsg = atoi(argv[2]); 																	/* buffer size                */
	num_p = atoi(argv[3]);  																	/* number of producers        */
	num_c = atoi(argv[4]); 																		/* number of consumers        */

	queue = createMq(maxmsg, "/queue_y65han");								/* Create a main message queue to store the item number */
	status_queue = createMq(maxmsg, "/status_queue_y65han");	/* Create a status queue to store the status number */

	send(status_queue, 0, num);																/* Set the satus number to the maximum number of items to produce */


	gettimeofday(&tv, NULL);
	g_time[0] = (tv.tv_sec) + tv.tv_usec/1000000.;

	for(i = 0; i < num_p; i++){
		spawnProducer(i);																				/* Spawn a producer process for each number of producer */
	}

	for(i = 0; i < num_c; i++){
		spawnConsumer(i);																				/* Spawn a cosumer process for each number of consumer */
	}

	int status;
	while(wait(&status) > 0){}																/* Wait for all the child processes to finish */

    gettimeofday(&tv, NULL);
    g_time[1] = (tv.tv_sec) + tv.tv_usec/1000000.;

    closeMq(queue);
    closeMq(status_queue);																	/* Close the main message queue and status queue */
    unlinkMq("/queue_y65han");
    unlinkMq("/status_queue_y65han");												/* Unlink the main message queue and status queue */

    printf("System execution time: %.6lf seconds\n", \
            g_time[1] - g_time[0]);
	exit(0);
}
