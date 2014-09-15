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
#include<stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include<signal.h>

#include "midi_ports.h"
#include "midi_nodes.h"
#include "MyPage.h"
#include "main.h"
#include "utils.h"
#include "notebook.h"


GtkBuilder *gtkBuilder = NULL;	/*main builder*/
GtkWidget *main_terminal = NULL;	/*main window terminal*/
GtkTextBuffer *main_textbuffer = NULL;	/*main textbuffer for the main terminal*/


void
on_window_destroy (GtkWidget *object, GThread *thread) {	/*callback to close the main window*/


	set_end_to_zero();						/*sets to 0 the end flag to close all threads*/
	free(my_page_root);
	g_object_unref(G_OBJECT(gtkBuilder));
	printf("Byebye\n");
	gtk_main_quit ();
}

void
hide_dialog(GtkWidget *button, GtkWidget *dialog) {	/*hides the warning dialog for combo_nodes*/
	gtk_widget_hide(GTK_WIDGET(dialog));
}


void
add_data_nodes_combo_box(GtkWidget *combo_nodes) { /*scans the nodes list and adds nodes to combo_nodes*/

	midi_node *node_root = return_midi_nodes();
	midi_node *node_tmp = NULL;

	node_tmp = node_root;
	while(node_tmp) {
		if(node_tmp->is_in)
			gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_nodes), NULL, node_tmp->name);
		node_tmp = node_tmp->next;
	}

	return;

}

void
midi_nodes_refresh(GtkButton *button, gpointer *user_data) {	/*callback to refresh combo_nodes*/
	gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(user_data));
	add_data_nodes_combo_box(GTK_WIDGET(user_data));
}


gchar*
get_nodes_combo_box_text(GtkWidget *combo_nodes, add_n_page *n_page) {	/*get the selected node from combo_nodes to add it in notebook*/
	gchar *string;
	string = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(n_page->combo_nodes)); /*gets the string*/

	g_print("Node selected: %s\n", string);

	if(string && !in_notebook(string))	/*if the selected device is not in notebook and if the string is not null..*/
		update_notebook_pages(n_page->notebook, string);	/*adds the device into notebook*/
	else {
		if(string) { /*if there's another device with the same name into notebook...*/
			GtkWidget *dialog = GTK_WIDGET(gtk_builder_get_object(gtkBuilder, "add_pages_failure"));	/*warning!*/
			gtk_widget_show(dialog);
		}
	}

	return NULL;

}

int
main(int argc, char *argv[]){

	GThread  *thread;
	GtkCssProvider  *provider;
	GError *error = NULL;
	GtkWidget *window;
	GdkDisplay *display;
	GdkScreen *screen;

	gtk_init(&argc, &argv);

	/*getting css...*/
	provider = gtk_css_provider_new ();
	display = gdk_display_get_default ();
	screen = gdk_display_get_default_screen (display);
	gtk_style_context_add_provider_for_screen (screen, GTK_STYLE_PROVIDER (provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	gtk_css_provider_load_from_path (provider, "./UI/main.css", &error);
	g_object_unref (provider);

	add_n_page n_page;	/*struct for get_nodes_combo_box_text callback*/
	/*GtkWidget *treeview;*/

	gtkBuilder = gtk_builder_new();	/*initializes the main builder*/
	if (!gtk_builder_add_from_file (gtkBuilder, "./UI/main.glade", &error)) {
		g_warning ("%s", error->message);
		g_error_free (error);
	}

	gtk_builder_connect_signals(gtkBuilder, 0); /*connects all signals defined into main.glade file*/
	/*treeview = GTK_WIDGET(gtk_builder_get_object(gtkBuilder, "treeview"));*/
	n_page.combo_nodes = GTK_WIDGET(gtk_builder_get_object(gtkBuilder, "combo_nodes"));
	n_page.notebook = GTK_WIDGET(gtk_builder_get_object(gtkBuilder, "notebook"));
	main_terminal = GTK_WIDGET(gtk_builder_get_object(gtkBuilder, "main_terminal"));
	main_textbuffer = GTK_TEXT_BUFFER(gtk_builder_get_object(gtkBuilder, "main_textbuffer"));
	window = GTK_WIDGET(gtk_builder_get_object(gtkBuilder, "window"));

	notebook = n_page.notebook;
	/*add_data_treeview(treeview);*/

	thread = g_thread_new("create_midi_nodes_threads_Thread", (GThreadFunc)create_midi_nodes_threads, NULL); /*thread that controls the midi nodes*/
	if (!thread) {
		g_error("Cannot start thread.\n");
	}

	add_switch_node();
	add_data_nodes_combo_box(n_page.combo_nodes);

	/*callbacks not defined into main.glade*/
	g_signal_connect(n_page.combo_nodes, "changed", G_CALLBACK(get_nodes_combo_box_text), &n_page);
	g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), thread);

	gtk_widget_show_all(window);
	gtk_main();

	return 0;
}

