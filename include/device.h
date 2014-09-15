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


#include <libvdeplug.h>

typedef struct thread_arg {
	char device_name[100];
	char string_remote_address[100];
	int connection_type;
} thread_arg;

typedef struct device_midi_thread_arg {
	char string_remote_address[100];
	char port_in[100];
	char port_out[100];
	unsigned short int port;

} device_midi_thread_arg;

VDECONN *conn;

extern int end;
