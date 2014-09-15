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
#include <sys/select.h>
#include <libvdeplug.h>

#include "server.h"
#include "net.h"


void
send_ip_request(char *string_remote_ip_address, int req, int page_index, char *msg_text) {

	struct sockaddr_in Local, To;
	unsigned short int remote_port_number;
	int socketfd, addr_size;
	int ris, result;

	fd_set socketWrite;
	mes request;
	

	request.type = req; /*assignment of request code to msg that will be sent*/
	memset(request.text, 0, 100);
	if(msg_text) {
		strncpy(request.text, msg_text, strlen(msg_text));
	}
	remote_port_number = DEVICE_REQUEST_PORT;
	socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketfd == SOCKET_ERROR) {
		printf ("socketfd() failed, Err: %d \"%s\"\n", errno,strerror(errno));
		exit(1);
	}

	Local.sin_family		=	AF_INET;
	Local.sin_addr.s_addr	=	htonl(INADDR_ANY);

	Local.sin_port = htons(SWITCH_REQUEST_PORT + page_index);
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
	do {
		FD_ZERO(&socketWrite);
		FD_SET(socketfd, &socketWrite);
		result = select(socketfd + 1, 0, &socketWrite, 0, 0);
	} while( (result < 0) && (errno == EINTR) );


	if(FD_ISSET(socketfd, &socketWrite)) {
		ris = sendto(socketfd, &request, sizeof(request) , 0, (struct sockaddr*)&To, addr_size);
		if (ris < 0) {
			printf ("sendto() failed, Error: %d \"%s\"\n", errno,strerror(errno));
			exit(1);
		}
		else
			printf("datagram UDP \"%s\" sent to (%s:%d)\n",
					request.text, string_remote_ip_address, remote_port_number);

	}
	close(socketfd);
	return;
}

void
send_vde_request(char *string_remote_address, int req, int page_index, char *msg_text) {

	unsigned short proto = 0x1234;
	unsigned char src[ETH_ALEN]
		= { 0x00, 0x12, 0x34, 0x56, 0x78, 0x90 };
	unsigned char dest[ETH_ALEN]
		= { 0x00, 0x13, 0x34, 0x56, 0x78, 0x90 };

	union ethframe frame;
	mes *request;

	int frame_len;
	fd_set socketWrite;
	int socket;
	int result;

	int data_len = sizeof(mes);

	request = &(frame.field.data.msg);
	frame_len = data_len + ETH_HLEN;
	frame.field.header.h_proto = htons(proto);

	memcpy(frame.field.header.h_dest, dest, ETH_ALEN);
	memcpy(frame.field.header.h_source, src, ETH_ALEN);
	strncpy(request->text, "midi ports request" , strlen("midi ports request") );

	request->type = req;
	if(msg_text) {
		strncpy(request->text, msg_text, strlen(msg_text));
	} else {
		memset(request->text, 0, 100);
	}


	if (conn == NULL) {
		printf("Error opening vde connection: %s\n",strerror(errno));
		exit(1);
	}
	socket = vde_datafd(conn);

	do {
		FD_ZERO(&socketWrite);
		FD_SET(socket, &socketWrite);
		result = select(socket + 1, 0, &socketWrite, 0, 0);
	} while( (result < 0) && (errno == EINTR) );

	if(FD_ISSET(socket, &socketWrite))
		vde_send(conn, frame.buffer, frame_len, 0);

}
