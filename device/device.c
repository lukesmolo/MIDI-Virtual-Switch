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
#include <libvdeplug.h>

#include "device.h"
#include "midi_device.h"
#include "net.h"
#include "requests.h"


int end = 1;

char tmp_local_midi_in_port[100];

void
keep_alive_vde(thread_arg *arg) {

	unsigned short proto = 0x1234;
	unsigned char dest[ETH_ALEN]
		= { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

	unsigned char src[ETH_ALEN]
		= { 0x00, 0x13, 0x34, 0x56, 0x78, 0x90 };

	union ethframe frame;
	mes *msg;

	int frame_len;
	fd_set socketWrite;
	int socketfd;
	int result;

	int data_len = sizeof(mes);

	struct timeval t;
	t.tv_sec = 0;
	t.tv_usec = 0;
	msg = &(frame.field.data.msg);
	frame_len = data_len + ETH_HLEN;
	frame.field.header.h_proto = htons(proto);

	memcpy(frame.field.header.h_dest, dest, ETH_ALEN);
	memcpy(frame.field.header.h_source, src, ETH_ALEN);
	memcpy(msg->text, arg->device_name , strlen(arg->device_name));
	msg->type = HI;
	
	if (conn == NULL) {
		printf("Error opening vde connection: %s\n",strerror(errno));
	}
	socketfd = vde_datafd(conn);

	while(end) {
		do {
			result = select(0, NULL, NULL, NULL, &t); /* waits for timeout*/
		} while( (result < 0) && (errno == EINTR) );

		do {
			FD_ZERO(&socketWrite);
			FD_SET(socketfd, &socketWrite);
			result = select(socketfd + 1, 0, &socketWrite, 0, 0);
		} while( (result < 0) && (errno == EINTR) );

		if(FD_ISSET(socketfd, &socketWrite)) {
			vde_send(conn, frame.buffer, frame_len, 0);
		}

	t.tv_sec = 5;
	t.tv_usec = 0;
	}
}


void
keep_alive_ip(thread_arg *arg) {
	
	struct sockaddr_in Local, To;
	unsigned short int remote_port_number;
	int socketfd, addr_size;
	int ris, result;
	struct timeval t;
	t.tv_sec = 0;
	t.tv_usec = 0;


	fd_set socketWrite;
	mes msg;
	

	msg.type = HI; /*assignment of request code to msg that will be sent*/
	strncpy(msg.text, arg->device_name, strlen(arg->device_name));
	remote_port_number = SWITCH_PORT;
	socketfd = socket(AF_INET, SOCK_DGRAM, 0);

	if (socketfd == SOCKET_ERROR) {
		printf ("socketfd() failed, Err: %d \"%s\"\n", errno,strerror(errno));
		exit(1);
	}

	Local.sin_family		=	AF_INET;
	Local.sin_addr.s_addr	=	htonl(INADDR_ANY);
	Local.sin_port = htons(0); /*o.s decides the port from which to send..*/

	ris = bind(socketfd, (struct sockaddr*) &Local, sizeof(Local));

	if (ris == SOCKET_ERROR)  {
		printf ("bind() failed, Err: %d \"%s\"\n",errno,strerror(errno));
		exit(1);
	}

	/* assignment of destination address */
	To.sin_family	 =	AF_INET;
	To.sin_addr.s_addr  =	inet_addr(arg->string_remote_address);
	To.sin_port		 =	htons(remote_port_number);
	addr_size = sizeof(struct sockaddr_in);

	while(end) {
		do {
			result = select(0, NULL, NULL, NULL, &t); /* waits for timeout*/
		} while( (result < 0) && (errno == EINTR) );

		do {
			FD_ZERO(&socketWrite);
			FD_SET(socketfd, &socketWrite);
			result = select(socketfd + 1, 0, &socketWrite, 0, 0);
		} while( (result < 0) && (errno == EINTR) );


		if(FD_ISSET(socketfd, &socketWrite)) {
			ris = sendto(socketfd, &msg, sizeof(msg) , 0, (struct sockaddr*)&To, addr_size);
			if (ris < 0) {
				printf ("sendto() failed, Error: %d \"%s\"\n", errno,strerror(errno));
				exit(1);
			}
			else
				printf("datagram UDP \"%d\" sent to (%s:%d)\n",
						msg.type, arg->string_remote_address, remote_port_number);

		}
	t.tv_sec = 5;
	t.tv_usec = 0;
	}
	close(socketfd);
	return;
}

void *
keep_alive(void *arg_thread) {
	thread_arg *arg = (thread_arg*)arg_thread;
	if(arg->connection_type == IP)
		keep_alive_ip(arg);
	else if(arg->connection_type == VDE)
		keep_alive_vde(arg);
	free(arg);
	pthread_exit(NULL);
}



void
give_response(char* string_remote_address, unsigned short int remote_port_number, mes request, int connection_type) { /*supplies the request*/
	device_midi_thread_arg *arg;
	int res = 0;
	pthread_t midi_thread;
	switch(connection_type) {
		case IP:	switch(request.type) {
					case ASK_MIDI_PORTS:
						ip_send_midi_ports(string_remote_address, remote_port_number);
						break;
					case ASK_MIDI:
						arg = malloc(sizeof(device_midi_thread_arg));
						memset(arg->port_in, 0, 100);
						memset(arg->port_out, 0, 100);
						memset(arg->string_remote_address, 0, 100);
						arg->port = remote_port_number;
						memcpy(arg->port_in, request.text, strlen(request.text));
						strncpy(arg->string_remote_address, string_remote_address, strlen(string_remote_address));
						res = pthread_create(&midi_thread, NULL, midi_send_ip, arg);
						if(res) {
							printf("ERROR; return code from pthread_create() is %d\n", res);
							return;
						}
						break;

					case GIVE_MIDI:
						arg = malloc(sizeof(device_midi_thread_arg));
						memset(arg->port_in, 0, 100);
						memset(arg->port_out, 0, 100);
						memset(arg->string_remote_address, 0, 100);
						memcpy(arg->port_out, "hw:0,1", strlen("hw:0,1"));
						arg->port = -1;
						memcpy(arg->port_out, request.text, strlen(request.text));
						strncpy(arg->string_remote_address, string_remote_address, strlen(string_remote_address));
						res = pthread_create(&midi_thread, NULL, midi_recv_ip, arg);
						if(res) {
							printf("ERROR; return code from pthread_create() is %d\n", res);
							return;
						}
						printf("dato midi\n");
						break;

					case MIDI_PORT_IN:
						memset(tmp_local_midi_in_port, 0, 100);
						strncpy(tmp_local_midi_in_port, request.text, strlen(request.text));
						break;
					default: break;
				};
				break;
		case VDE:	switch(request.type) {
					case ASK_MIDI_PORTS:
						vde_send_midi_ports(string_remote_address);
						break;
					default: break;
				};
				break;
		default: break;
	}


}

int
what_connection_is(char *connection_string) {
	if(!strcmp(connection_string, "IP")) {
		return IP;
	} else if(!strcmp(connection_string, "VDE")) {
		return VDE;
	} else if(!strcmp(connection_string, "VXVDE")) {
		return VXVDE;
	} else {
		printf("Wrong connection_type, closing ...\n");
		exit(0);
	}
}


int main(int argc, char *argv[]) {

	struct vde_open_args open_args={.port=0,.group=NULL,.mode=0700};
	char _switch[100] = "/tmp/xxx";
	conn = vde_open(_switch, "test", &open_args);

	int ris, connection_type;
	pthread_t thread;
	if(argc != 4) {
		printf("error: expected 3 arguments. Closing ...\n");
		exit(0);
	}
	thread_arg *arg = malloc(sizeof(thread_arg));
	if(arg == NULL) {
		printf("Malloc for thread failed\n");
		exit(0);
	}
	connection_type =  what_connection_is(argv[2]);
	memset(arg->string_remote_address, 0, 100);
	memset(arg->device_name, 0, 100);
	strncpy(arg->string_remote_address, argv[1], strlen(argv[1]));
	strncpy(arg->device_name, argv[3], strlen(argv[3]));
	arg->connection_type = connection_type;	

	ris = pthread_create(&thread, NULL, keep_alive, arg);

	if(ris) {
		printf("ERROR; return code from pthread_create() is %d\n", ris);
		exit(-1);
	}
	switch(connection_type) {
		case IP:	wait_ip_request();
				break;
		case VDE:   wait_vde_request();
				break;
		default: break;
	}


	return 0;
}

