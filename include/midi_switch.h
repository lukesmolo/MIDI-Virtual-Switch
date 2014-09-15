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


#ifndef __MIDI_H__
#define __MIDI_H__

#include "MyPage.h"



#define NOTE_ON 0x90
#define NOTE_OFF 0x80
#define INSTR 0xC0
#define VOL 0xB0

#define IS_NOTE_ON ((unsigned char)buffer[0] <= 0x9F && (unsigned char)buffer[0] >= 0x90)
#define IS_NOTE_OFF ((unsigned char)buffer[0] <= 0x8F && (unsigned char)buffer[0] >= 0x80)
#define IS_VOL ((unsigned char)buffer[0] <= 0xBF && (unsigned char)buffer[0] >= 0xB0) && (buffer[1] == 0x07)
#define IS_INSTR ((unsigned char)buffer[0] <= 0xCF && (unsigned char)buffer[0] >= 0xC0)

enum {
	VOL_CHANG,
	INSTR_CHANG,
	CHAN_CHANG,
	STOP
};

enum {
	SWITCH_LOCAL_IN_OUT,
	DEVICE_LOCAL_IN_OUT,
	IN_OUT,
	SWITCH_IN,
	SWITCH_OUT
};
	

typedef struct midi_control_packet {
	int type;
	int value;
} midi_control_packet;

typedef struct midi_local_thread_arg {
	char port_in[100];
	char port_out[100];
	MyPage *page;
} midi_local_thread_arg;

typedef struct midi_ext_thread_arg {
	char node_in_address[100];
	char node_out_address[100];
	int page_index;
} midi_ext_thread_arg;


void switch_local_in_out(midi_local_thread_arg *midi_arg);
void in_out(midi_ext_thread_arg *midi_arg);
int recv_midi_ip(int socketfd, char *buffer, int page_index);
int send_midi_ip(int socketfd, char* address, char *buffer, int page_index);
void what_channel_is(int c, char buffer[]);
#endif
