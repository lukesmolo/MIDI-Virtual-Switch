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


#include<alsa/asoundlib.h>
#include<stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include<alsa/asoundlib.h>
#include <poll.h>


#include "midi_switch.h"
#include "midi_nodes.h"
#include "net.h"

void
what_channel_is(int c, char buffer[]) { /*decide what buffer[0] has to be basing on what packet arrives*/

	printf("channel %x\n", (unsigned char)buffer[0]);

	if(IS_NOTE_ON) {
		buffer[0] = NOTE_ON | (unsigned char)c;
	} else if(IS_NOTE_OFF) {
		buffer[0] = NOTE_OFF | (unsigned char)c;
	} else if(IS_INSTR) {
		buffer[0] = INSTR | (unsigned char)c;
		printf("si\n");
	} else if(IS_VOL) {
		buffer[0] = VOL | (unsigned char)c;
	}

	printf("channel %x\n", (unsigned char)buffer[0]);

}

void
switch_local_in_out(midi_local_thread_arg *midi_arg) { /*midi local input-output functions;*/

	char *port_out = malloc(strlen(midi_arg->port_out));
	char *port_in = malloc(strlen(midi_arg->port_in));

	strncpy(port_in, midi_arg->port_in, strlen(midi_arg->port_in));
	strncpy(port_out, midi_arg->port_out, strlen(midi_arg->port_out));

	MyPage *page = midi_arg->page;	
	int result;
	midi_control_packet contr;
	int local_end = 1;
	int i;

	free(midi_arg);

	int status;
	int mode_in = SND_RAWMIDI_SYNC; /*in this way the buffer for rawmidi_write is always drained before the function exits*/
	int mode_out = SND_RAWMIDI_SYNC;
	snd_rawmidi_t* midi_in = NULL; /*it will be an handle for the input device*/
	snd_rawmidi_t* midi_out = NULL; /*it will be an handle for the output device*/
	char buffer[3]; /*storage for input buffer received*/
	char change_instrument_buffer[2] = {0xC0, 0}; /*storage for instrument changing*/
	char change_volume_buffer[3] = {0xB0, 07, 50}; /*storage for volume changing*/
	int channel = 0; /*channel requested by user*/
	struct pollfd *pfd_in; /*poll struct for socketpair and midi input*/
	struct pollfd *pfd_out; /*poll struct for midi output	*/
	int npfd_in = 2;
	int npfd_out = 1;


	if((status = snd_rawmidi_open(&midi_in, NULL, port_in, mode_in)) < 0) { /*open input*/
		printf("Problem opening MIDI input %s: %s\n", port_in, snd_strerror(status));
		printf("Closing thread\n");
		local_end = 0;
	}

	if((status = snd_rawmidi_open(NULL,&midi_out, port_out, mode_out)) < 0) { /*open output*/
		printf("Problem opening MIDI input %s: %s\n", port_out, snd_strerror(status));
		printf("Closing\n");
		printf("Closing thread\n");
		local_end = 0;
	}
	pfd_in = (struct pollfd *)alloca(npfd_in * sizeof(struct pollfd));
	pfd_out = (struct pollfd *)alloca(npfd_out * sizeof(struct pollfd));

	snd_rawmidi_poll_descriptors(midi_in, pfd_in, 1);
	snd_rawmidi_poll_descriptors(midi_out, pfd_out, 1);

	pfd_in[0].events = POLLIN;
	pfd_in[0].revents = 0;

	pfd_in[1].events = POLLIN;
	pfd_in[1].revents = 0;
	pfd_in[1].fd = page->other.midi_fd[readsocket];

	pfd_out[2].events = POLLOUT;
	pfd_out[2].revents = 0;


	contr.type = -1;
	contr.value = 0;

	while(end && local_end) {


		if (poll(pfd_in, npfd_in, -1) > 0) {

			if (pfd_in[0].revents & POLLIN) {/*check if there's a midi packet to read*/
				if((status = snd_rawmidi_read(midi_in, buffer, 3)) < 0) {
					printf("Problem opening MIDI input %s: %s\n", port_in, snd_strerror(status));
					printf("Closing thread\n");
					local_end = 0;
				}

				if((unsigned char)buffer[0] <= 0xCF && (unsigned char)buffer[0] >= 0xC0) { /*it's a change program packet*/
					; 				}
				if((unsigned char)buffer[0] <= 0xBF && (unsigned char)buffer[0] >= 0xB0) { /*it's a change program packet*/
					;
				}
				what_channel_is(channel, buffer); /*basing on the kind of message and channel requested by user, set buffer[0]*/
				printf("0x%x ", (unsigned char)buffer[0]);
				printf("%d ", (unsigned char)buffer[1]);
				printf("%d ", (unsigned char)buffer[2]);
				printf("\n");

				if (poll(pfd_out, npfd_out, 0) > 0) { /*check if it's possible to write the packet read*/
										     if (pfd_out[0].revents & POLLOUT) {
						if((status = snd_rawmidi_write(midi_out, buffer, 3)) < 0) {
							printf("Problem opening MIDI input %s: %s\n", port_out, snd_strerror(status));
							printf("Closing thread\n");
							local_end = 0;
						} else {
							printf("ok\n");
						}
					}
				}
			}

			if (pfd_in[1].revents & POLLIN) { /*check if there's a msg from socketpair*/

				result = read(page->other.midi_fd[readsocket], &contr, sizeof(midi_control_packet));
				if(result < 0) {
					printf("error: failed to receive data from socketpair\n");
				}
				if(contr.type != -1) {
					switch(contr.type) {
						case VOL_CHANG:
							change_volume_buffer[2] = contr.value;

							if (poll(pfd_out, npfd_out, 0) > 0) { /*check if it's possible to write the packet received from socketpair*/
								if (pfd_out[0].revents & POLLOUT) {
									what_channel_is(channel, change_volume_buffer);
									if((status = snd_rawmidi_write(midi_out, change_volume_buffer, 3)) < 0) {
										printf("Problem opening MIDI input %s: %s\n", port_out, snd_strerror(status));
										printf("Closing thread\n");
										local_end = 0;
									} else {
										printf("ok volume\n");
									}
								}
							}
							break;
						case INSTR_CHANG: change_instrument_buffer[1] = contr.value;
									what_channel_is(channel, change_instrument_buffer);

									if (poll(pfd_out, npfd_out, 0) > 0) {/*check if it's possible to write the packet received from socketpair*/
										if (pfd_out[0].revents & POLLOUT) {
											if((status = snd_rawmidi_write(midi_out, change_instrument_buffer, 2)) < 0) {
												printf("Problem opening MIDI input %s: %s\n", port_out, snd_strerror(status));
												printf("Closing thread\n");
												local_end = 0;
											}
											else {
												printf("ok instrument\n");
											}
										}
									}
									break;
						case CHAN_CHANG:
									channel = contr.value;
									break;

						case STOP:	local_end = 0;
								break;
						default: break;
					}
					contr.type = -1;
					contr.value = 0;
				}
			}
		}
		for(i = 0; i < npfd_in; i++)
			pfd_in[i].revents = 0;
		pfd_out[0].revents = 0;
	}

	if(midi_in)
		snd_rawmidi_close(midi_in);
	if(midi_out)
		snd_rawmidi_close(midi_out);
	midi_in  = NULL;
	midi_out = NULL;
	free(port_in);
	free(port_out);
	page->other.midi_thread = NULL;
	printf("midi thread is dying\n");
	return;
}

void
in_out(midi_ext_thread_arg *midi_arg) { /*function that receives packets and forwards them to the output device*/
	char node_in_address[100];
	char node_out_address[100];
	int page_index;
	int local_end = 1;

	memset(node_in_address, 0, 100);
	memset(node_out_address, 0, 100);

	strncpy(node_in_address, midi_arg->node_in_address, strlen(midi_arg->node_in_address));
	strncpy(node_out_address, midi_arg->node_out_address, strlen(midi_arg->node_out_address));
	page_index = midi_arg->page_index;

	free(midi_arg);

	char buffer[3]; /*storage for input buffer received*/

	struct sockaddr_in Local;
	unsigned short int local_port_number;
	int socketfd, res;
	struct sockaddr_in requiredFrom;
	fd_set socketRead, socketWrite;


	local_port_number = SWITCH_MIDI_RECV_PORT + page_index;
	printf("listening to on: %d\n", local_port_number);
	/* get a datagram socket */
	socketfd = socket(AF_INET, SOCK_DGRAM, 0);

	if (socketfd == SOCKET_ERROR) {
		printf ("socket() failed, Err: %d \"%s\"\n", errno,strerror(errno));
		return;
	}

	/* name the socket */
	memset(&Local, 0, sizeof(Local));
	Local.sin_family		=	AF_INET;
	Local.sin_addr.s_addr	=	htonl(INADDR_ANY);        /* wildcard */
	Local.sin_port		=	htons(local_port_number);

	res = bind(socketfd, (struct sockaddr*) &Local, sizeof(Local));
	if (res == SOCKET_ERROR)  {
		printf ("bind() 1 failed, Err: %d \"%s\"\n",errno,strerror(errno));
		return;
	}

	/* assign our
	 * destination
	 * address */
	memset ( &requiredFrom, 0, sizeof(requiredFrom) );
	requiredFrom.sin_family	 =	AF_INET;
	requiredFrom.sin_addr.s_addr  = inet_addr(node_in_address);
	requiredFrom.sin_port		 = htons(0);

	while(end && local_end) /*reading midi input from recv*/
	{
		memset(buffer, 0, 3);
		do {
			FD_ZERO(&socketRead);
			FD_SET(socketfd,&socketRead);
			FD_ZERO(&socketWrite);
			FD_SET(socketfd,&socketWrite);

			res = select(socketfd + 1, &socketRead, &socketWrite, 0, 0);

		} while( (res < 0) && (errno == EINTR) ); /*useful to avoid so interruptions*/

		if(FD_ISSET(socketfd, &socketRead)) { /*if the socket to read is ready..*/ 
			recv_midi_ip(socketfd, buffer, page_index);

			if(FD_ISSET(socketfd, &socketWrite)) {
				send_midi_ip(socketfd, node_out_address, buffer, page_index);
			}

		}
	}

	close(socketfd);

	return;
}

int
send_midi_ip(int socketfd, char *address, char *buffer, int page_index) { /*send ip midi messages*/
	int res;
	struct sockaddr_in To;
	socklen_t addr_size;

	memset(&To, 0, sizeof(struct sockaddr_in));
	To.sin_family = AF_INET;
	To.sin_addr.s_addr = inet_addr(address); /* htonl(INADDR_ANY); */
	To.sin_port = htons(DEVICE_MIDI_PORT_RECV);
	addr_size = sizeof(struct sockaddr_in);

	res = sendto(socketfd, buffer, 3, 0, (struct sockaddr*)&To, addr_size);
	printf("datagram UDP \"%d\" sent to (%s:%d)\n", buffer[1], address, DEVICE_MIDI_PORT_RECV);

	if (res < 3) {
		printf ("sendto() of midi failed, Error: %d \"%s\"\n", errno,strerror(errno));
		return 0;
	}
	return 1;

}

int
recv_midi_ip(int socketfd, char *buffer, int page_index) { /*receive ip midi messages*/
	struct sockaddr_in From;
	unsigned short int remote_port_number;
	unsigned int Fromlen;
	char string_remote_ip_address[100];
	int  msglen;
	memset(&From, 0, sizeof(From));
	Fromlen = sizeof(struct sockaddr_in);

	msglen = recvfrom ( socketfd, buffer, 3, 0, (struct sockaddr*)&From, &Fromlen);
	if (msglen != 3 && msglen != 2) {
		printf ("Message arrived has not size of a midi packet! Err: %d \"%s\n",errno,strerror(errno));
		printf("size: %d\n", msglen);
		return 0;
	} else {
		if(msglen == 2) {
			;
		}

		remote_port_number = From.sin_port;
		sprintf((char*)string_remote_ip_address,"%s",inet_ntoa(From.sin_addr));
		printf("ricevuto da socketfd %d  msg: \"%d\" len %ld, from host %s, port %d\n", socketfd, buffer[1], msglen, string_remote_ip_address, remote_port_number);
	}

	return 1;

}

