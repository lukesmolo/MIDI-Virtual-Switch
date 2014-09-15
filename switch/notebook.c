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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "MyPage.h"
#include "net.h"
#include "midi_ports.h"
#include "midi_nodes.h"
#include "main.h"
#include "utils.h"




GtkWidget *notebook = NULL;	/*pointer to the notebook*/
my_page *my_page_root = NULL; /*root of pages list*/


void
page_destroy(GtkWidget *button, GtkWidget *page) { /*deletes a page from list and notebook*/
	gint index;
	midi_node* node;
	my_page *tmp = NULL;
	my_page *tmp1 = NULL;

	index = gtk_notebook_page_num(GTK_NOTEBOOK(notebook), page);

	gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), index);
	node = node_for_page(MY_PAGE(page), NULL);
	node->page = NULL;
	print_terminal(HOME, NULL, "page deleted: %d\n", index);

	if(my_page_root->page == page) {
		tmp = my_page_root;
		my_page_root = tmp->next;

	} else {
		tmp = my_page_root;
		while(tmp->next->page != page) {
			tmp = tmp->next;
		}
		tmp1 = tmp;
		tmp = tmp->next;
		tmp1->next = tmp->next;

		free(tmp);
	}
	tmp = my_page_root;
	while(tmp) {
		if(tmp)
			printf("%s\n", tmp->tab_name);
		tmp = tmp->next;

	}

}



void
update_notebook_pages(GtkWidget *notebook, gchar *string) { /* adds a page (device) to the notebook*/

	GtkWidget *tab_label;
	GtkWidget *tab_box; /*box that contains label and button of a notebook tab*/
	GtkWidget *tab_button;
	GtkWidget *page;
	
	static int count = 0; /*var that counts pages*/
	char str[50];


	page = my_page_new();
	count++;

	tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	tab_button = gtk_button_new_from_icon_name("gtk-close", 2);

	my_page *tmp = NULL;

	memset(str, 0, 50);
	strncpy(str, string, strlen(string));
	strcat(str, "_tab");
	initialize_page(MY_PAGE(page), count, string);
	tab_label = gtk_label_new (string); /*string for tab (device)*/
	gtk_widget_set_name (GTK_WIDGET(tab_label), str); /*string to identify the widget (string+ "_tab")*/
	gtk_box_pack_start(GTK_BOX(tab_box), tab_label, TRUE, FALSE, 0); /*inserts label and button into box*/
	gtk_box_pack_end(GTK_BOX(tab_box), tab_button, TRUE, FALSE, 0);
	

	if(!my_page_root) {
		my_page_root = malloc(sizeof(my_page));
		tmp = my_page_root;
	} else {
		tmp = my_page_root;
		while(tmp->next)
			tmp = tmp->next;
		tmp->next = malloc(sizeof(my_page));
		tmp = tmp->next;
	}
	tmp->next = NULL;
	tmp->page = page;
	memset(tmp->tab_name, 0, 50);
	strncpy(tmp->tab_name, string, strlen(string));
	add_data_page_combo_nodes(MY_PAGE(page));	
	print_terminal(HOME, NULL, "device %s added\n", tmp->tab_name);
	gtk_notebook_insert_page (GTK_NOTEBOOK(notebook), GTK_WIDGET(page), GTK_WIDGET(tab_box), -1);
	gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(notebook), GTK_WIDGET(page), TRUE);
	g_signal_connect(tab_button, "clicked", G_CALLBACK(page_destroy), page);
	gtk_widget_show_all(GTK_WIDGET(tab_box));

}


int
in_notebook(gchar *string) { /*check if a page is already into notebook*/

	my_page *tmp = my_page_root;

	while(tmp && strcmp(tmp->tab_name, string)) {
		tmp = tmp->next;
	}
	if(!tmp)
		return 0;
	else
		return 1;
}
