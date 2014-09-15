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
#include<sys/select.h>
#include <libvdeplug.h>

#include "midi_ports.h"
#include "net.h"
#include "device.h"

void
wait_ip_request() { /*waits for a midi switch ip request*/

	fd_set socketRead;
	int result;
	char string_remote_ip_address[100];
	struct sockaddr_in Local, From;
	unsigned short int remote_port_number, local_port_number;
	int socketfd, msglen, ris;
	unsigned int Fromlen;
	mes request;

	local_port_number = DEVICE_REQUEST_PORT;


	socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketfd == SOCKET_ERROR) {
		printf ("socket() failed, Err: %d \"%s\"\n", errno,strerror(errno));
		exit(1);
	}
	memset(&Local, 0, sizeof(Local));
	Local.sin_family		=	AF_INET;
	Local.sin_addr.s_addr	=	htonl(INADDR_ANY);        /* wildcard*/
	Local.sin_port		=	htons(local_port_number);

	ris = bind(socketfd, (struct sockaddr*) &Local, sizeof(Local));
	if (ris == SOCKET_ERROR)  {
		printf ("bind() 1 failed, Err: %d \"%s\"\n",errno,strerror(errno));
		exit(1);
	}


	while(end) {	
		memset(request.text, 0, 100);
		request.type = -1;

		do {
			FD_ZERO(&socketRead);
			FD_SET(socketfd, &socketRead);
			result = select(socketfd + 1, &socketRead, 0, 0, 0);
		} while( (result < 0) && (errno == EINTR) );

		memset(&From, 0, sizeof(From));
		Fromlen=sizeof(struct sockaddr_in);
		msglen = recvfrom ( socketfd, &request, sizeof(request), 0, (struct sockaddr*)&From, &Fromlen);
		if (msglen<0) {
			char msgerror[1024];
			sprintf(msgerror,"recvfrom() failed [err %d] ", errno);
			perror(msgerror);
			/* return(1);
			 * */
		}
		sprintf((char*)string_remote_ip_address,"%s",inet_ntoa(From.sin_addr));
		remote_port_number = ntohs(From.sin_port);
		give_response(string_remote_ip_address, remote_port_number, request, IP);
	}
	close(socketfd);
}

void
wait_vde_request() { /*waits for a midi switch vde request*/

	fd_set socketRead;
	union ethframe message;
	mes *request;
	char string_remote_address[100];
	int result;
	int count;
	int socketfd = vde_datafd(conn);


	if (conn == NULL) {
		printf("error: %s\n",strerror(errno));
	}


	while(end) {

		request = &(message.field.data.msg);
		memset(string_remote_address, 0, 100);
		count = 0;
		do {
			FD_ZERO(&socketRead);
			FD_SET(socketfd, &socketRead);
			result = select(socketfd + 1, &socketRead, 0, 0, 0);
		} while( (result < 0) && (errno == EINTR) );
		if(FD_ISSET(socketfd, &socketRead)) {
			count = vde_recv(conn, message.buffer, ETH_FRAME_LEN, 0);
			if(count < 0) {
				printf("error: failed to receive data\n");
			}

		}
		printf("request: %d\n", request->type);
		if(request->type != HI && request->type != SEND_MIDI_PORTS && request->type != -1) { /*avoid messages from it own*/
			printf("ricevuto qualcosa\n");
			sprintf((char*)string_remote_address,"%s", message.field.header.h_source);
			printf("ricevuto da socketfd %d  msg: \"%s\" len %d, from host %.6s\n", socketfd, request->text, count, message.field.header.h_source);

			give_response(string_remote_address, 0, *request, VDE);
			memset(request->text, 0, 100);
			request->type = 0;

		}

	}
	close(socketfd);

}




void
ip_send_midi_ports(char *string_remote_ip_address, unsigned short int remote_port_number) { /*reads from midi_ports.c and sends the ports*/

	struct sockaddr_in Local, To;
	int socketfd, addr_size;
	int ris, result;
	mes msg;
	fd_set socketWrite;

	midi_card *root = return_midi_ports();
	midi_card *tmp_card = root;
	midi_dev *tmp_dev = NULL;
	midi_subdev *tmp_subdev = NULL;
	char tmp_string[100];
	int to_add, send_ok;
	int page_index = remote_port_number - SWITCH_REQUEST_PORT;
	remote_port_number = SWITCH_PORT + page_index;

	socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketfd == SOCKET_ERROR) {
		printf ("socketfd() failed, Err: %d \"%s\"\n", errno,strerror(errno));
		exit(1);
	}

	Local.sin_family		=	AF_INET;
	Local.sin_addr.s_addr	=	htonl(INADDR_ANY);
	Local.sin_port = htons(0); /*o.s decides the port*/

	ris = bind(socketfd, (struct sockaddr*) &Local, sizeof(Local));
	if (ris == SOCKET_ERROR)  {
		printf ("bind() failed, Err: %d \"%s\"\n",errno,strerror(errno));
		exit(1);
	}

	/* assignment of destination address */
	To.sin_family	 =	AF_INET;
	To.sin_addr.s_addr  =	inet_addr(string_remote_ip_address);
	To.sin_port		 =	htons(remote_port_number);

	addr_size = sizeof(struct sockaddr_in);
	send_ok = 1;
	msg.type = SEND_MIDI_PORTS;
	while(tmp_card || send_ok) { /*while there's another port or there's ok to send...*/

		do {
			FD_ZERO(&socketWrite);
			FD_SET(socketfd, &socketWrite);
			result = select(socketfd + 1, 0, &socketWrite, 0, 0);
		} while( (result < 0) && (errno == EINTR) );
		memset(msg.text, 0, 100);
		memset(tmp_string, 0, 100);
		if(tmp_card) {
			if(tmp_card->is_midi) {
				tmp_dev = tmp_card->dev_root;
				to_add = 1;
				while(tmp_dev) {
					memset(tmp_string, 0, 100);
					tmp_subdev = tmp_dev->sub_root;
					while(tmp_subdev) {
						if(tmp_subdev->name[0] != '\0') {
							to_add = 0;
							strcat(tmp_string, tmp_subdev->hw);
							strcat(tmp_string, "\t");
							strcat(tmp_string, tmp_subdev->name);
							strncpy(msg.text, tmp_string, strlen(tmp_string));
							if(FD_ISSET(socketfd, &socketWrite)) {

								ris = sendto(socketfd, &msg, sizeof(msg) , 0, (struct sockaddr*)&To, addr_size);
								if (ris < 0) {
									printf ("sendto() failed, Error: %d \"%s\"\n", errno,strerror(errno));
									exit(1);
								}
								else
									printf("datagram UDP \"%s\" sent to (%s:%d) %ld\n",
											msg.text, string_remote_ip_address, remote_port_number, sizeof(msg));
							}
						} else {
							to_add = 1;
						}
						tmp_subdev = tmp_subdev->next;
					}
					if(to_add) {
						strcat(tmp_string, tmp_dev->hw);
						strcat(tmp_string, "\t");
						strcat(tmp_string, tmp_dev->name);


						strncpy(msg.text, tmp_string, strlen(tmp_string));
						if(FD_ISSET(socketfd, &socketWrite)) {
							ris = sendto(socketfd, &msg, sizeof(msg) , 0, (struct sockaddr*)&To, addr_size);
							if (ris < 0) {
								printf ("sendto() failed, Error: %d \"%s\"\n", errno,strerror(errno));

								exit(1);
							}
							else
								printf("datagram UDP \"%s\" sent to (%s:%d)\n",
										msg.text, string_remote_ip_address, remote_port_number);
						}
					}
					tmp_dev = tmp_dev->next;
				}
			}
			tmp_card = tmp_card->next;
		} else {
			msg.type = OK_TO_READ;
			strncpy(msg.text, OK_MSG, OK_MSG_LENGTH);
			ris = sendto(socketfd, &msg, sizeof(msg) , 0, (struct sockaddr*)&To, addr_size);
			if (ris < 0) {
				printf ("sendto() failed, Error: %d \"%s\"\n", errno,strerror(errno));
				exit(1);
			}
			else
				printf("datagram UDP \"%s\" sent to (%s:%d)\n",
						msg.text, string_remote_ip_address, remote_port_number);
			send_ok = 0;
		}

	}
	close(socketfd);
}


void
vde_send_midi_ports(char *string_remote_address) {

	unsigned short proto = 0x1234;

	char src[6];

	char dest[6];
	union ethframe frame;
	mes *msg;
	int frame_len;
	fd_set socketWrite;
	int socketfd;
	int result, ris;

	
	midi_card *root = return_midi_ports();
	midi_card *tmp_card = root;
	midi_dev *tmp_dev = NULL;
	midi_subdev *tmp_subdev = NULL;
	char tmp_string[100];
	int to_add, send_ok;

	int data_len = sizeof(mes);

	socketfd = vde_datafd(conn);

	msg = &(frame.field.data.msg);
	memset(dest, 'b', 6);
	memset(src, 'd', 6);
	dest[0] = 'd';
	src[0] = 'd';
	frame_len = data_len + ETH_HLEN;
	frame.field.header.h_proto = htons(proto);

	memcpy(frame.field.header.h_dest, dest, 6);
	memcpy(frame.field.header.h_source, src, 6);

	send_ok = 1;
	msg->type = SEND_MIDI_PORTS;
	while(tmp_card || send_ok) { /*while there's another port or there's ok to send...*/

		do {
			FD_ZERO(&socketWrite);
			FD_SET(socketfd, &socketWrite);
			result = select(socketfd + 1, 0, &socketWrite, 0, 0);
		} while( (result < 0) && (errno == EINTR) );
		memset(msg->text, 0, 100);
		memset(tmp_string, 0, 100);
		if(tmp_card) {
			if(tmp_card->is_midi) {
				tmp_dev = tmp_card->dev_root;
				to_add = 1;
				while(tmp_dev) {
					memset(tmp_string, 0, 100);
					tmp_subdev = tmp_dev->sub_root;
					while(tmp_subdev) {
						if(tmp_subdev->name[0] != '\0') {
							to_add = 0;
							strcat(tmp_string, tmp_subdev->name);
							strcat(tmp_string, "\t");
							strcat(tmp_string, tmp_subdev->hw);
							strncpy(msg->text, tmp_string, strlen(tmp_string));
							if(FD_ISSET(socketfd, &socketWrite)) {
								ris = vde_send(conn, frame.buffer, frame_len, 0);

								if (ris < 0) {
									printf ("sendto() failed, Error: %d \"%s\"\n", errno,strerror(errno));
									exit(1);
								}
								else
									printf("msg \"%s\" sent to (%s) %ld\n",
											msg->text, string_remote_address, sizeof(msg));
							}
						} else {
							to_add = 1;
						}
						tmp_subdev = tmp_subdev->next;
					}
					if(to_add) {
						strcat(tmp_string, tmp_dev->name);
						strcat(tmp_string, "\t");
						strcat(tmp_string, tmp_dev->hw);
						strncpy(msg->text, tmp_string, strlen(tmp_string));
						if(FD_ISSET(socketfd, &socketWrite)) {
							ris = vde_send(conn, frame.buffer, frame_len, 0);

							if (ris < 0) {
								printf ("sendto() failed, Error: %d \"%s\"\n", errno,strerror(errno));
								exit(1);
							}
							else
								printf("msg \"%s\" sent to (%s) %ld\n",
										msg->text, frame.field.header.h_source, sizeof(msg));

						}
					}
					tmp_dev = tmp_dev->next;
				}
			}
			tmp_card = tmp_card->next;

		} else {
			msg->type = OK_TO_READ;
			strncpy(msg->text, OK_MSG, OK_MSG_LENGTH);
			ris = vde_send(conn, frame.buffer, frame_len, 0);

			if (ris < 0) {
				printf ("sendto() failed, Error: %d \"%s\"\n", errno,strerror(errno));
				exit(1);
			}
			else {
				printf("msg \"%s\" sent to (%.6s) %ld\n",
						msg->text, frame.field.header.h_source, sizeof(msg));
				send_ok = 0;
			}


		}

	}
}
