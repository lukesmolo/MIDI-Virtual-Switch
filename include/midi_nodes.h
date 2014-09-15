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


#define SWITCH_NAME "Switch"
#define SWITCH_ADDRESS "127.0.0.1"
#define THREADS_NUM 3
#include "MyPage.h"

int end;

typedef struct midi_node {
	char name[50];
	char address[100];
	int connection_type;
	int is_in;
	struct timeval tv;
	struct midi_node *next;
	MyPage *page;	
} midi_node;

void add_switch_node();
void create_midi_node(char *name, char *string_remote_address, int connection_type);
int check_ip_is_in(char *name);
void *check_node_to_remove();
void *create_midi_nodes_threads();
midi_node *return_midi_nodes();
void set_end_to_zero();
midi_node *node_for_page(MyPage *page, char *name);
