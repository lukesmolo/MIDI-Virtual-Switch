/*
 * Copyright (C) 2014 Luca Sciullo
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <libvdeplug.h>
#include <sys/select.h>
#include <ctype.h>
#include <signal.h>


#include "MyPage.h"
#include "midi_nodes.h"
#include "net.h"
#include "utils.h"
#include "server.h"

pthread_t midi_nodes_threads[THREADS_NUM]; /*array of all threads that manage the life messages midi nodes*/

midi_node *node_root = NULL;	/*root of nodes*/

int end = 1;	/*flag that keeps alive all threads*/


pthread_mutex_t threads_sync_lock;
pthread_cond_t threads_sync_cond;

int threads_sync_count = 0;

void
add_switch_node() {
	if(node_root) { /*the switch has to be the first node*/
		printf("error: switch would not be the first node\n");
		return;
	} else {

		node_root = malloc(sizeof(midi_node));
		memset(node_root->name, 0, 50);
		memset(node_root->address, 0, 100);

		strncpy(node_root->name, SWITCH_NAME, strlen(SWITCH_NAME));
		strncpy(node_root->address, SWITCH_ADDRESS, strlen(SWITCH_ADDRESS));
		node_root->connection_type = LOCAL;
		node_root->next = NULL;
		gettimeofday(&node_root->tv, NULL);
		node_root->is_in = 1;	
		node_root->page = NULL; /*there's no page at this point for this device*/

	}
}

void
create_midi_node(char *name, char *string_remote_address, int connection_type) { /*create a new midi node*/
	midi_node *tmp;
	if(!node_root) {
		node_root = malloc(sizeof(midi_node));
		tmp = node_root;
	} else {
		tmp = node_root;
		while(tmp->next != NULL)
			tmp = tmp->next;
		tmp->next = malloc(sizeof(midi_node));
		tmp = tmp->next;
	}
	/*initializes struct fields*/
	memset(tmp->name, 0, 50);
	memset(tmp->address, 0, 100);

	strncpy(tmp->name, name, strlen(name));
	strncpy(tmp->address, string_remote_address, strlen(string_remote_address));
	tmp->connection_type = connection_type;
	tmp->next = NULL;
	gettimeofday(&tmp->tv, NULL);
	tmp->is_in = 1;	
	tmp->page = NULL; /*there's no page at this point for this device*/

}

int
check_node_is_in(char *name) { /*check if a node is already into the list*/
	midi_node *tmp = node_root;

	while(tmp != NULL && strcmp(tmp->name, name)) {
		tmp = tmp->next;
	}

	if(tmp == NULL)
		return 0;

	else { /*if the node is already in ip list, it resets the node struct fields*/

		tmp->is_in = 1;
		gettimeofday(&tmp->tv, NULL);
		return 1;
	}
}

void *
check_node_to_remove() { /*when timeout expires, it removes ip nodes not reconfirmed*/

	struct timeval t;
	time_t diff;
	int res;
	int timeout = 5;
	midi_node *tmp = NULL;

	while(end) {
		t.tv_usec = 0;
		t.tv_sec = 111111115; /*setting time to wake up select*/
		do {
			res =select(0, NULL, NULL, NULL, &t);
		} while( (res < 0) && (errno == EINTR) ); /*useful to avoid so interruptions*/

		gettimeofday(&t, NULL);
		tmp = node_root;
		while(tmp) {

			diff = t.tv_sec - tmp->tv.tv_sec; /*calculation of the difference between now and the last confirm of a eth node */
			if(diff >= timeout && tmp->is_in) { /*if the timeout is expired..*/
				printf("Timeout of %d expired\n", timeout);
				printf("%s removed\n", tmp->name);
				tmp->is_in = 0;
			}
			tmp = tmp->next;

		}
	}
	return NULL;
}


void
wait_for_eth_nodes() { /*thread that waits for (vde) eth nodes*/

	fd_set socketRead;
	char string_remote_address[100];
	union ethframe message;
	mes *msg;
	int result;
	int count;

	int socketfd = vde_datafd(conn);


	if (conn == NULL) {
		printf("error: %s\n",strerror(errno));
	}

	while(end) {

		memset(string_remote_address, 0, 100);
		count = 0;
		do {
			FD_ZERO(&socketRead);
			FD_SET(socketfd, &socketRead);
			result = select(socketfd + 1, &socketRead, 0, 0, 0);
		} while( (result < 0) && (errno == EINTR) );
		if(FD_ISSET(socketfd, &socketRead)) {

			count = vde_recv(conn, message.buffer, ETH_FRAME_LEN, 0);
			msg = &(message.field.data.msg);

		}
		sprintf((char*)string_remote_address,"%s", message.field.header.h_source);

		if(msg->type == HI) {

			printf("ricevuto da socketfd %d  msg: \"%s\" len %d, from host %.6s\n", socketfd, msg->text, count, message.field.header.h_source);
			if(!check_node_is_in(msg->text)) {
				pthread_mutex_lock(&threads_sync_lock);
				create_midi_node(msg->text, string_remote_address, VDE);
				printf("A new (vde) eth device arrived\n");
				pthread_mutex_unlock(&threads_sync_lock);;
			}
			else
				printf("Eth device already in list\n");
		}
	}
	close(socketfd);
}



void
wait_for_ip_nodes() { /*thread that waits for ip  nodes*/

	fd_set socketRead;
	int result;

	struct sockaddr_in Local, From;
	char string_remote_ip_address[100];
	unsigned short int remote_port_number, local_port_number;
	int socketfd, msglen, ris;
	unsigned int Fromlen;
	mes msg;	

	local_port_number = SWITCH_PORT;

	socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketfd == SOCKET_ERROR) {
		printf ("socket() failed, Err: %d \"%s\"\n", errno,strerror(errno));
		exit(1);
	}
	memset(&Local, 0, sizeof(Local));
	Local.sin_family		=	AF_INET;
	Local.sin_addr.s_addr	=	htonl(INADDR_ANY);        /* wildcard */
	Local.sin_port		=	htons(local_port_number);
	ris = bind(socketfd, (struct sockaddr*) &Local, sizeof(Local));
	if (ris == SOCKET_ERROR)  {
		printf ("bind() 1 failed, Err: %d \"%s\"\n",errno,strerror(errno));
		exit(1);
	}


	while(end) {
		do {
			FD_ZERO(&socketRead);
			FD_SET(socketfd, &socketRead);
			result = select(socketfd + 1, &socketRead, 0, 0, 0);
		} while( (result < 0) && (errno == EINTR) );
		memset(msg.text, 0, 100);
		msg.type = -1;
		memset(&From, 0, sizeof(From));
		Fromlen=sizeof(struct sockaddr_in);
		msglen = recvfrom ( socketfd, &msg, sizeof(msg), 0, (struct sockaddr*)&From, &Fromlen);
		if (msglen<0) {
			char msgerror[1024];
			sprintf(msgerror,"recvfrom() failed [err %d] ", errno);
			perror(msgerror);
			/* return(1);
			 * */
		}
		sprintf((char*)string_remote_ip_address,"%s",inet_ntoa(From.sin_addr));
		remote_port_number = ntohs(From.sin_port);
		printf("Received from socket  %d  msg: \"%s\" len %ld, from host %s, port %d\n", socketfd,
				msg.text, sizeof(msg), string_remote_ip_address, remote_port_number);
		if(msg.type == HI) {

			if(!check_node_is_in(msg.text)) {
				pthread_mutex_lock(&threads_sync_lock);
				create_midi_node(msg.text, string_remote_ip_address, IP);
				printf("A new ip device arrived\n");
				pthread_mutex_unlock(&threads_sync_lock);
			}
			else

				printf("IP device already in list\n");
		}
	}
	close(socketfd);
}

void
threads_Sync() { /*it synchronizes all threads*/
	pthread_mutex_lock(&threads_sync_lock);
	threads_sync_count++;

	if(threads_sync_count < THREADS_NUM) {/*forse + NUM_THREADS meno 1!*/
		pthread_cond_wait(&threads_sync_cond, &threads_sync_lock);
	}

	pthread_cond_signal(&threads_sync_cond);
	pthread_mutex_unlock(&threads_sync_lock);
	return;

}

void*
midi_nodes_Thread(void *index) { /*for all threads, it gives them its function*/
	int ind;
	ind = *((int*) index);
	switch(ind) {
		case 0:	
			wait_for_ip_nodes();
			break;
		case 1:
			wait_for_eth_nodes();
			break;
		case 2:
			check_node_to_remove();
			break;
		default:
			break;
	}

	threads_Sync();
	pthread_exit(index);
}

void *
create_midi_nodes_threads() { /*creates all threads*/
	int *p = NULL;
	int ris;
	int t = 0;
	void *ptr;
	int error;
	struct vde_open_args open_args={.port=0,.group=NULL,.mode=0700};
	conn = vde_open("/tmp/xxx", "test", &open_args);


	for(t = 0; t < THREADS_NUM ; t++) {
		p = malloc(sizeof(int));
		if(p == NULL) {
			printf("Malloc for the index of a thread failed\n");
			exit(0);
		}
		*p = t;
		ris = pthread_create(&midi_nodes_threads[t], NULL, midi_nodes_Thread, p);

		if(ris) {
			printf("ERROR; return code from pthread_create() is %d\n", ris);
			exit(-1);
		}
	}
	for(t = 0; t < THREADS_NUM; t++) {

		error = pthread_join(midi_nodes_threads[t], (void*) &ptr);
		if(error != 0){
			printf("pthread_join() failed: error= %d\n", error);
			exit(-1);
		} else {
			printf("bridge thread %d is dying\n", *((int*)ptr));
			free(ptr); /*deallocating structure in which pthread_join returns value*/
		}
	}
	return NULL;
}


midi_node*
return_midi_nodes() {
	return node_root;
}


void
set_end_to_zero() {
	end = 0;
	return;
}


midi_node*
node_for_page(MyPage *page, char *name ) { /*searches the node for a page basing on name of node or page that contains*/
	midi_node *tmp = node_root;
	if(page) {
		while(tmp && (page != tmp->page))
			tmp = tmp->next;
		if(!tmp)
			return NULL;
		else
			return tmp;
	} else {
		while(tmp && strcmp(tmp->name, name)) {
			tmp = tmp->next;

		}
		if(!tmp)
			return NULL;
		else
			return tmp;

	}
}
