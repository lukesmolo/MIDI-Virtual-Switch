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


#include<string.h>
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

#include "MyPage.h"
#include "main.h"
#include "midi_ports.h"
#include "midi_nodes.h"
#include "utils.h"
#include "server.h"
#include "net.h"
#include "device.h"
#include "midi_switch.h"


G_DEFINE_TYPE(MyPage, my_page, GTK_TYPE_PANED); /*defines the new type, child of PANED class*/

static void
my_page_class_init (MyPageClass *klass) {

	GObjectClass *gobject_class;
	GtkWidgetClass *page_class;

	gobject_class = G_OBJECT_CLASS (klass);
	page_class = GTK_WIDGET_CLASS (klass);



	/* Setup the template GtkBuilder xml for this class
	*/
	gtk_widget_class_set_template_from_resource (page_class, "/my/UI/MyPage.glade");

	gtk_widget_class_bind_template_child (page_class, MyPage , button_ports);
	gtk_widget_class_bind_template_child (page_class, MyPage , combo_ports);
	gtk_widget_class_bind_template_child (page_class, MyPage , page_textbuffer);
	gtk_widget_class_bind_template_child (page_class, MyPage , page_terminal);
	gtk_widget_class_bind_template_child (page_class, MyPage , page_combo_nodes);
	gtk_widget_class_bind_template_child (page_class, MyPage , page_node_combo_ports);
	gtk_widget_class_bind_template_child (page_class, MyPage , volume_button);
	gtk_widget_class_bind_template_child (page_class, MyPage , instrument_spin);
	gtk_widget_class_bind_template_child (page_class, MyPage , stop_midi_thread_button);
	gtk_widget_class_bind_template_child (page_class, MyPage , channel_spin);
	gtk_widget_class_bind_template_child (page_class, MyPage , volume_spin);

	/* Declare the callback ports that this widget class exposes, to bind with <signal>
	 * connections defined in the GtkBuilder xml
	 */
	gtk_widget_class_bind_template_callback (page_class, midi_ports_refresh);
	gtk_widget_class_bind_template_callback (page_class, midi_nodes_refresh);
	gtk_widget_class_bind_template_callback (page_class, get_ports_combo_box_text);
	gtk_widget_class_bind_template_callback (page_class, get_page_combo_nodes_text);
	gtk_widget_class_bind_template_callback (page_class, get_page_node_combo_ports_text);
	gtk_widget_class_bind_template_callback (page_class, add_data_page_node_combo_ports);
	gtk_widget_class_bind_template_callback (page_class, launch_midi);
	gtk_widget_class_bind_template_callback (page_class, midi_control);

}

static void
my_page_init (MyPage *page) {
	/*initializes page other struct*/
	page->other.ports_root = NULL;
	page->other.index = -1;
	page->other.midi_thread = NULL;
	page->other.wait_ports_thread = NULL;


	/*initializes page widgets*/
	gtk_widget_init_template (GTK_WIDGET (page));

}

void
initialize_page(MyPage *page, int index, char *string) {

	midi_node* node;
	page->other.index = index;
	socketpair(AF_UNIX, SOCK_STREAM, 0, page->other.fd); /*socketpair to confirm that thread can read*/
	socketpair(AF_UNIX, SOCK_STREAM, 0, page->other.midi_fd); /*socketpair to send control messages to midi thread*/

	node = node_for_page(NULL, string); /*research of the node for this page*/

	node->page = MY_PAGE(page);
	switch(node->connection_type) {
		case LOCAL: page->other.wait_ports_thread = NULL;
				local_wait_for_midi_ports(page); /*this is not a thread like IP, VDE*/
				add_data_ports_combo_box(page);
				break;
		case IP: page->other.wait_ports_thread = g_thread_new("wait_ports_thread", (GThreadFunc)ip_wait_for_midi_ports, MY_PAGE(page)); /*launching the page thread to wait for midi ports*/
			   break;
		case VDE: page->other.wait_ports_thread = g_thread_new("wait_ports_thread", (GThreadFunc)vde_wait_for_midi_ports, MY_PAGE(page)); /*launching the page thread to wait for midi ports*/
			    break;
		default: break;
	}
	if (!page->other.wait_ports_thread && node->connection_type != LOCAL) {
		g_error("Cannot start thread.\n");
	}



}

/***********************************************************
 *                       Callbacks                         *
 ***********************************************************/
void
midi_ports_refresh(MyPage *page, GtkButton *button) {

	int result;
	struct timeval t;
	midi_node * node = node_for_page(page, NULL);

	t.tv_sec = 5;
	t.tv_usec = 0;

	GtkComboBoxText *combo_ports = GTK_COMBO_BOX_TEXT(page->combo_ports);
	gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(combo_ports));
	free_ports(page->other.ports_root); /*frees all the list*/
	page->other.ports_root = NULL;
	if(node->connection_type != LOCAL) {
		switch(node->connection_type) {
			case IP:	send_ip_request(node->address, ASK_MIDI_PORTS , page->other.index, NULL);
					break;
			case VDE:	send_vde_request(node->address, ASK_MIDI_PORTS , page->other.index, NULL);
					break;
			default: break;
		}
		do {
			FD_ZERO(&(page->other.fdread));
			FD_SET(page->other.fd[readsocket], &(page->other.fdread));

			result = select(page->other.fd[readsocket] + 1, &(page->other.fdread), NULL, NULL, &t); /* waiting for the confirm to read*/
		} while( (result < 0) && (errno == EINTR) );

		if(FD_ISSET(page->other.fd[readsocket], &(page->other.fdread))) {
			result = read(page->other.fd[readsocket], page->other.buf, OK_MSG_LENGTH);
			add_data_ports_combo_box(page);
			print_terminal(PAGE, page, "All midi ports received\n");
		} else {
			print_terminal(PAGE, page, "failed to receive midi ports of device\n");
		}
	} else {
		local_wait_for_midi_ports(page);
		add_data_ports_combo_box(page);
	}
}


gchar*
get_ports_combo_box_text(GtkWidget *page, gpointer *user_data) { /*gets the midi ports selected*/
	gchar *string;

	string = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(user_data));
	if(string) {
		print_terminal(PAGE, MY_PAGE(page),"Port selected: \t%s\n", string);
		print_terminal(HOME, MY_PAGE(page),"Port selected: \t%s\n", string);
	}

	g_print("Port selected: %s\n", string);
	return string;

}

gchar*
get_page_combo_nodes_text(GtkWidget *page, gpointer *user_data) {	/*gets the selected node from page_combo_nodes*/
	gchar *string;

	string = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(user_data)); /*gets the string*/
	if(string && user_data) {
		print_terminal(PAGE, MY_PAGE(page),"Node selected: \t%s\n", string);
		print_terminal(HOME, MY_PAGE(page),"Node selected: \t%s\n", string);
	}

	if(user_data);
	add_data_page_node_combo_ports(page, NULL);

	return string;

}

gchar*
get_page_node_combo_ports_text(GtkWidget *page, gpointer *user_data) {	/*gets the selected node from page_combo_nodes*/
	gchar *string;

	string = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(user_data)); /*gets the string*/
	if(string && page) {
		print_terminal(PAGE, MY_PAGE(page),"Port of node selected: \t%s\n", string);
		print_terminal(HOME, MY_PAGE(page),"Port of node selected: \t%s\n", string);
	}

	g_print("Port of node selected: %s\n", string);

	return string;

}

void
add_data_page_node_combo_ports(GtkWidget *page, gpointer *user_data) {
	MyPage *tmp_page = MY_PAGE(page);
	GtkWidget *page_node_combo_ports = tmp_page->page_node_combo_ports;
	gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(page_node_combo_ports));
	page_midi_ports *tmp = tmp_page->other.ports_root;	
	while(tmp) {
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(page_node_combo_ports), NULL, tmp->name);
		tmp = tmp->next;
	}
}

void
launch_midi(GtkWidget *page, gpointer *user_data) { /*create the thread that manages midi messages*/

	midi_local_thread_arg  *l_midi_arg;
	midi_ext_thread_arg  *e_midi_arg;
	gchar *port_in;
	gchar *port_out;
	char msg_text[100];
	int mode;
	midi_node* node_in;
	midi_node* node_out;


	MyPage *tmp_page = MY_PAGE(page);
	GtkWidget *combo_ports = tmp_page->combo_ports;
	GtkWidget *page_node_combo_ports = tmp_page->page_node_combo_ports;
	GtkWidget *page_combo_nodes = tmp_page->page_combo_nodes;
	gchar *node_out_name =  gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(page_combo_nodes));

	node_in = node_for_page(tmp_page, NULL); /*research of the node for this page basig on its page*/
	node_out = node_for_page(NULL, node_out_name); /*research of the node for this page its name*/

	port_in = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo_ports)); /*gets the string*/
	port_out = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(page_node_combo_ports)); /*gets the string*/
	port_in = split_string(port_in);
	port_out = split_string(port_out);

	mode = what_mode_is(node_in->name, node_out_name);

	if (tmp_page->other.midi_thread) {
		g_thread_unref(tmp_page->other.midi_thread);
	}

	switch(mode) {
		case SWITCH_LOCAL_IN_OUT:
			l_midi_arg = malloc(sizeof(midi_local_thread_arg));
			memset(l_midi_arg->port_in, 0, 100);
			memset(l_midi_arg->port_out, 0, 100);
			strncpy(l_midi_arg->port_in, port_in, strlen(port_in));
			strncpy(l_midi_arg->port_out, port_out, strlen(port_out));
			l_midi_arg->page = tmp_page;
			tmp_page->other.midi_thread = g_thread_new("switch_midi_thread", (GThreadFunc)switch_local_in_out, l_midi_arg); /*launching the page thread to wait for midi ports*/
			break;
		case DEVICE_LOCAL_IN_OUT:
			break;
		case IN_OUT:
			memset(msg_text, 0, 100);
			strncpy(msg_text, port_in, strlen(port_in));
			send_ip_request(node_in->address, ASK_MIDI, tmp_page->other.index, msg_text);
			e_midi_arg = malloc(sizeof(midi_ext_thread_arg));
			memset(e_midi_arg->node_in_address, 0, 100);
			memset(e_midi_arg->node_out_address, 0, 100);
			strncpy(e_midi_arg->node_in_address, node_in->address, strlen(node_in->address));
			strncpy(e_midi_arg->node_out_address, node_out->address, strlen(node_out->address));
			e_midi_arg->page_index = tmp_page->other.index;
			tmp_page->other.midi_thread = g_thread_new("switch_midi_thread", (GThreadFunc)in_out, e_midi_arg);
			break;
		case SWITCH_IN:
			break;
		case SWITCH_OUT:
			break;
		default: break;
	}
	if (!tmp_page->other.midi_thread) {
		printf("Cannot start switch midi thread.\n");
	}
	free(port_in);
	free(port_out);
	return;


}

void
midi_control(GtkWidget *page, gpointer *user_data) { /*get users UI midi changes*/


	MyPage *tmp_page = MY_PAGE(page);
	midi_control_packet control_packet;

	GtkWidget *from = GTK_WIDGET(user_data);
	int volume = 63;	
	int instrument = 0;
	int channel = 0;

	if(from == tmp_page->volume_button) {
		volume = gtk_scale_button_get_value(GTK_SCALE_BUTTON(from));
		control_packet.type = VOL_CHANG;
		control_packet.value = volume;
		printf("Volume changed : %d\n", volume);
	} else if(from == tmp_page->volume_spin) {
		instrument = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(from));
		control_packet.type = VOL_CHANG;
		control_packet.value = volume;
		printf("Volume changed : %d\n", volume);
	} else if(from == tmp_page->instrument_spin) {
		instrument = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(from));
		printf("Instrument changed: %d\n", instrument);
		control_packet.type = INSTR_CHANG;
		control_packet.value = instrument;
	} else if(from == tmp_page->channel_spin) {
		channel = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(from));
		control_packet.type = CHAN_CHANG;
		control_packet.value = channel;
		printf("Channel changed: %d\n", channel);
	}  else if(from == tmp_page->stop_midi_thread_button) {
		control_packet.type = STOP;
		control_packet.value = 0;
	}

	send(tmp_page->other.midi_fd[writesocket], &control_packet, sizeof(midi_control_packet), MSG_NOSIGNAL); /*socketpair changing*/
}


/***********************************************************
 *                            API                          *
 ***********************************************************/

GtkWidget*
my_page_new () {
	return g_object_new (MY_TYPE_WIDGET, NULL);
}

void
free_ports(page_midi_ports *tmp) { /*recursively free local midi ports list*/
	if(!tmp) {
		return;
	}
	else {
		free_ports(tmp->next);
		free(tmp);
	}
}


void
add_data_ports_combo_box(MyPage *page) { /*add midi ports to combo_ports*/

	GtkWidget *combo_ports;
	combo_ports = page->combo_ports;
	page_midi_ports *tmp = page->other.ports_root;
	while(tmp) {
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_ports), NULL, tmp->name);
		printf("%s\n", tmp->name);
		tmp = tmp->next;
	}

}


void
ip_wait_for_midi_ports(MyPage *page) {
	fd_set socketRead;
	int result;

	struct sockaddr_in Local, From;
	char string_remote_ip_address[100];
	unsigned short int remote_port_number, local_port_number;
	int socketfd, msglen, ris;
	unsigned int Fromlen;
	mes msg;
	local_port_number = SWITCH_PORT + page->other.index; /*the port to listen is the default MIDI_SWITCH_PORT plus the page index*/
	page_midi_ports *tmp = page->other.ports_root;


	socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketfd == SOCKET_ERROR) {
		printf ("socket() failed, Err: %d \"%s\"\n", errno,strerror(errno));
		exit(1);
	}
	memset(&Local, 0, sizeof(Local));
	Local.sin_family		=	AF_INET;
	Local.sin_addr.s_addr	=	htonl(INADDR_ANY);        /* wildcard */
	Local.sin_port		=	htons(local_port_number);
	printf("listening to on: %d\n", ntohs(Local.sin_port));
	ris = bind(socketfd, (struct sockaddr*) &Local, sizeof(Local));
	if (ris == SOCKET_ERROR)  {
		printf ("bind() 1 failed, Err: %d \"%s\"\n",errno,strerror(errno));
	}


	while(end) {	
		memset(msg.text, 0, 100);
		msg.type = -1;
		do {
			FD_ZERO(&socketRead);
			FD_SET(socketfd, &socketRead);
			result = select(socketfd + 1, &socketRead, 0, 0, 0);
		} while( (result < 0) && (errno == EINTR) );

		memset(&From, 0, sizeof(From));
		Fromlen=sizeof(struct sockaddr_in);

		if(FD_ISSET(socketfd, &socketRead)) {
			msglen = recvfrom (socketfd, &msg, sizeof(msg), 0, (struct sockaddr*)&From, &Fromlen);
			if (msglen<0) {
				char msgerror[1024];
				sprintf(msgerror,"recvfrom() failed [err %d] ", errno);
				perror(msgerror);
				/* return(1);
				 * */
			}
			sprintf((char*)string_remote_ip_address,"%s",inet_ntoa(From.sin_addr));
			remote_port_number = ntohs(From.sin_port);
			if(msg.type == SEND_MIDI_PORTS) {

				if(!page->other.ports_root) {
					page->other.ports_root = malloc(sizeof(page_midi_ports));
					tmp = page->other.ports_root;
				} else {
					tmp = page->other.ports_root;
					while(tmp->next)
						tmp = tmp->next;
					tmp->next = malloc(sizeof(page_midi_ports));
					tmp = tmp->next;
				}
				memset(tmp->name, 0, 100);
				strncpy(tmp->name, msg.text, strlen(msg.text));
				tmp->next = NULL;
			} else if (msg.type == OK_TO_READ){
				send(page->other.fd[writesocket], OK_MSG, OK_MSG_LENGTH, MSG_NOSIGNAL); /*sends ok to thread that is waiting for reading the midi ports list*/
			}
		}
	}
	close(socketfd);
}

void
vde_wait_for_midi_ports(MyPage *page) {

	fd_set socketRead;
	char string_remote_address[100];
	union ethframe message;
	mes *msg;
	int result;
	int count;

	page_midi_ports *tmp = page->other.ports_root;

	int socketfd = vde_datafd(conn);


	if (conn == NULL) {
		printf("error: %s\n",strerror(errno));
	}


	while(end) {	
		memset(msg->text, 0, 100);
		msg->type = -1;
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
		if(msg->type == SEND_MIDI_PORTS) {

			if(!page->other.ports_root) {
				page->other.ports_root = malloc(sizeof(page_midi_ports));
				tmp = page->other.ports_root;
			} else {
				tmp = page->other.ports_root;
				while(tmp->next)
					tmp = tmp->next;
				tmp->next = malloc(sizeof(page_midi_ports));
				tmp = tmp->next;
			}
			strncpy(tmp->name, msg->text, strlen(msg->text));
			tmp->next = NULL;
		} else if (msg->type == OK_TO_READ){
			send(page->other.fd[writesocket], OK_MSG, OK_MSG_LENGTH, MSG_NOSIGNAL); /*sends ok to thread that is waiting for reading the midi ports list*/
			printf("ok to read remote midi ports\n");
		}
	}
	close(socketfd);
}

void
local_wait_for_midi_ports(MyPage *page) { /*scan local midi ports and add them to switch page midi ports list*/
	midi_card *root = return_midi_ports();
	midi_card *tmp_card = root;
	midi_dev *tmp_dev = NULL;
	midi_subdev *tmp_subdev = NULL;
	int to_add;
	char tmp_string[100];
	page_midi_ports *tmp = page->other.ports_root;
	while(tmp_card) { /*while there's another port ...*/

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
							if(!page->other.ports_root) {
								page->other.ports_root = malloc(sizeof(page_midi_ports));
								tmp = page->other.ports_root;
							} else {
								tmp = page->other.ports_root;
								while(tmp->next)
									tmp = tmp->next;
								tmp->next = malloc(sizeof(page_midi_ports));
								tmp = tmp->next;
							}

							memset(tmp->name, 0, 100);
							strncpy(tmp->name, tmp_string, strlen(tmp_string));
							tmp->next = NULL;
						} else {
							to_add = 1;
						}
						tmp_subdev = tmp_subdev->next;
					}
					if(to_add) {
						strcat(tmp_string, tmp_dev->hw);
						strcat(tmp_string, "\t");
						strcat(tmp_string, tmp_dev->name);
						if(!page->other.ports_root) {
							page->other.ports_root = malloc(sizeof(page_midi_ports));
							tmp = page->other.ports_root;
						} else {
							tmp = page->other.ports_root;
							while(tmp->next)
								tmp = tmp->next;
							tmp->next = malloc(sizeof(page_midi_ports));
							tmp = tmp->next;
						}

						memset(tmp->name, 0, 100);
						strncpy(tmp->name, tmp_string, strlen(tmp_string));
						tmp->next = NULL;

					}
					tmp_dev = tmp_dev->next;
				}
			}

			tmp_card = tmp_card->next;

		}
	}

}


GtkWidget *
return_page_field(MyPage *page, int field) { /*returns a field of the page struct*/

	switch(field) {
		case BUTTON_PORTS:
			return page->button_ports;
		case COMBO_PORTS:
			return page->combo_ports;
		case PAGE_TEXTBUFFER:
			return page->page_textbuffer;
		case PAGE_TERMINAL:
			return page->page_terminal;
		default:
			break;
	}
	return NULL;
}

void
add_data_page_combo_nodes(MyPage *page) {
	GtkWidget *page_combo_nodes = page->page_combo_nodes;
	add_data_nodes_combo_box(page_combo_nodes);
	return;
}

int
what_mode_is(gchar *in, gchar* out) {/*decide what kind of mode is for the midi flow*/


	int mode;	
	char node_in[100];
	char node_out[100];

	memset(node_in, 0, 100);
	memset(node_out, 0, 100);

	strncpy(node_in, in, strlen(in));
	strncpy(node_out, out, strlen(out));

	free(out);

	if(!strcmp(node_in, node_out)) {
		if(!strcmp(node_in, SWITCH_NAME))
			mode = SWITCH_LOCAL_IN_OUT;
		else
			mode = DEVICE_LOCAL_IN_OUT;

	} else if(!strcmp(node_in, SWITCH_NAME)) {

		mode = SWITCH_IN;

	} else if(!strcmp(node_out, SWITCH_NAME)) {

		mode = SWITCH_OUT;
	}

	else {
		return mode = IN_OUT;
	}
	return mode;	
}


