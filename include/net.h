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


#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>

#define SWITCH_PORT 60000
#define SWITCH_REQUEST_PORT 50000

#define SWITCH_MIDI_RECV_PORT 40500

#define DEVICE_REQUEST_PORT 50000
#define DEVICE_MIDI_PORT_RECV 50500

#define OK_MSG "ok"
#define OK_MSG_LENGTH strlen(OK_MSG)

#define SOCKET_ERROR   ((int)-1)
#define SIZEBUF 100000L

enum {
	HI,
	OK_TO_READ,
	ASK_MIDI_PORTS,
	SEND_MIDI_PORTS,
	ASK_MIDI,
	GIVE_MIDI,
	MIDI_PORT_IN,
	MIDI_PORT_OUT
};

enum {
	LOCAL,
	IP,
	VDE,
	VXVDE
};



typedef struct mes {
	int type;
	char text[100];
} mes;

union mes_frame {
	struct mes msg;
	unsigned char data[ETH_DATA_LEN];
};

union ethframe
{
	struct
	{
		struct ethhdr    header;
		union mes_frame data;
	} field;
	unsigned char    buffer[ETH_FRAME_LEN];
};



