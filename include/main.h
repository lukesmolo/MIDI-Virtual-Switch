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


#include<gtk/gtk.h>



typedef struct add_n_page {
	GtkWidget *notebook;
	GtkWidget *combo_nodes;
} add_n_page;


typedef struct my_page {
	GtkWidget *page;
	gchar tab_name[50];
	struct my_page *next;
} my_page;


GtkBuilder *gtkBuilder;
GtkWidget *main_terminal;
my_page *my_page_root;
GtkTextBuffer *main_textbuffer;

void add_data_nodes_combo_box(GtkWidget *combo_nodes);
void midi_nodes_refresh(GtkButton *button, gpointer *user_data);
