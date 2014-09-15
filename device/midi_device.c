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

#include "net.h"
#include "device.h"

int play_midi(char buf[], snd_rawmidi_t* midi_out) { /*play midi contained in buffer*/
	int status;
	if((status = snd_rawmidi_write(midi_out, buf, 3)) < 0) {
			printf("Problem opening MIDI input %s:\n", snd_strerror(status));
			printf("Closing\n");
			return 0;
	}
	return 1;
}


void*
midi_send_ip(void * thread_arg) { /*read midi from input port and send to switch*/

	fd_set socketWrite;
	device_midi_thread_arg arg;

	struct sockaddr_in Local, To;
	char string_remote_ip_address[100];
	unsigned short int remote_port_number;
	int socketfd, addr_size;
	int ris;
	int page_index;
	int local_end = 1;

	int mode_in = SND_RAWMIDI_SYNC; /*in this way the buffer for rawmidi_write is always drained before the function exits*/
	snd_rawmidi_t* midi_in = NULL; /*it will be an handle for the input device*/
	char buffer[3];
	char port_in[100];
	int status;


	arg = *((device_midi_thread_arg*)thread_arg);
	remote_port_number = arg.port; /*from remote port it catches the page index*/
	page_index = remote_port_number - SWITCH_REQUEST_PORT;
	memcpy(port_in, arg.port_in, strlen(arg.port_in));
	strncpy(string_remote_ip_address, arg.string_remote_address, strlen(arg.string_remote_address));
	remote_port_number = SWITCH_MIDI_RECV_PORT + page_index;
	free(thread_arg);

	/* get a datagram socket */
	socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketfd == SOCKET_ERROR) {
		printf ("socket() failed, Err: %d \"%s\"\n", errno,strerror(errno));
		exit(1);
	}


	/* name the socket */
	Local.sin_family		=	AF_INET;
	Local.sin_addr.s_addr	=	htonl(INADDR_ANY);
	Local.sin_port = htons(0); /*o.s decides the local port */

	ris = bind(socketfd, (struct sockaddr*) &Local, sizeof(Local));
	if (ris == SOCKET_ERROR)  {
		printf ("bind() failed, Err: %d \"%s\"\n",errno,strerror(errno));
		exit(1);
	}

	/* assign our destination address */
	To.sin_family	 =	AF_INET;
	To.sin_addr.s_addr  =	inet_addr(string_remote_ip_address);
	To.sin_port		 =	htons(remote_port_number);

	addr_size = sizeof(struct sockaddr_in);
	/* send to the address */
	if((status = snd_rawmidi_open(&midi_in, NULL, port_in, mode_in)) < 0) { /*open midi input*/
		printf("Problem opening MIDI input %s: %s\n", port_in, snd_strerror(status));
		printf("Closing device midi thread ...\n");
		local_end = 0;
	}
	else {
		printf("opened %s\n", port_in);
	}

	while(end && local_end) {
		if((status = snd_rawmidi_read(midi_in, buffer, 3)) < 0) {
			printf("Problem opening MIDI input %s: %s\n", port_in, snd_strerror(status));
			printf("Closing device midi thread ...\n");
			local_end = 0;
		}
		do {
			FD_ZERO(&socketWrite);
			FD_SET(socketfd,&socketWrite);

			ris = select(socketfd + 1, 0, &socketWrite, 0, 0);

		} while( (ris < 0) && (errno == EINTR) ); /*useful to avoid so interruptions*/

		if(FD_ISSET(socketfd, &socketWrite)) { /*if the socket to read is ready..*/

			ris = sendto(socketfd, buffer, strlen(buffer) , 0, (struct sockaddr*)&To, addr_size);
			if (ris < 0) {
				printf ("sendto() failed, Error: %d \"%s\"\n", errno,strerror(errno));
			}
			else
				printf("midi datagram UDP sent to (%s:%d)\n", string_remote_ip_address, remote_port_number);
		}
	}
	close(socketfd);
	pthread_exit(0);
}

void *
midi_recv_ip(void *thread_arg) { /*receive midi packets and play them*/


	device_midi_thread_arg arg;
	int local_end = 1;
	struct sockaddr_in Local, From;
	char string_remote_ip_address[100];
	unsigned short int local_port_number;
	unsigned short int remote_port_number;
	int socketfd, msglen, res;
	unsigned int Fromlen;
	char buffer[3];
	//unsigned short int required_remote_port_number;
	//char string_required_remote_ip_address[100];
	char port_out[100];
	//struct sockaddr_in requiredFrom;

	arg = *((device_midi_thread_arg*)thread_arg);
	memcpy(port_out, arg.port_out, strlen(arg.port_out));
	strncpy(string_remote_ip_address, arg.string_remote_address, strlen(arg.string_remote_address));
	local_port_number = DEVICE_MIDI_PORT_RECV;
	free(thread_arg);

	/*midi*/
	snd_rawmidi_t* midi_out = NULL; /*it will be an handle for the output device*/
	int status; /*status to playing midi*/
	int mode_out = SND_RAWMIDI_SYNC;

	if((status = snd_rawmidi_open(NULL,&midi_out, port_out, mode_out)) < 0) { /*open input*/
		printf("Problem opening MIDI input %s: \n", snd_strerror(status));
		printf("Closing\n");
		exit(1);
	}

	/* get a datagram socket */
	socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketfd == SOCKET_ERROR) {
		printf ("socket() failed, Err: %d \"%s\"\n", errno,strerror(errno));
		exit(1);
	}

	/* name the socket */
	memset(&Local, 0, sizeof(Local));
	Local.sin_family		=	AF_INET;
	Local.sin_addr.s_addr	=	htonl(INADDR_ANY);        /* wildcard */
	Local.sin_port		=	htons(local_port_number);
	res = bind(socketfd, (struct sockaddr*) &Local, sizeof(Local));

	if (res == SOCKET_ERROR)  {
		printf ("bind() 1 failed, Err: %d \"%s\"\n",errno,strerror(errno));
		exit(1);
	}

	
	while(end && local_end) /*reading midi input from recv*/
	{
		printf("arrivo anche io\n");
		memset(&From, 0, sizeof(From));
		Fromlen=sizeof(struct sockaddr_in);
		msglen = recvfrom ( socketfd, buffer, 3, 0, (struct sockaddr*)&From, &Fromlen);
		if (msglen<0) {
			char msgerror[1024];
			sprintf(msgerror,"recvfrom() failed [err %d] ", errno);
			perror(msgerror);
			/* return(1);
			 * */
		}
		sprintf((char*)string_remote_ip_address,"%s",inet_ntoa(From.sin_addr));
		remote_port_number = ntohs(From.sin_port);
		printf("ricevuto da socketfd %d  msg: \"%s\" len %d, from host %s, port %d\n", socketfd,
				buffer, msglen, string_remote_ip_address, remote_port_number);

		if(!play_midi(buffer, midi_out))
			printf("error playing midi\n");

	}
	close(socketfd);
	pthread_exit(0);

}
