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


#ifndef __MY_WIDGET_H__
#define __MY_WIDGET_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS



#define MY_TYPE_WIDGET                 (my_page_get_type ())
#define MY_PAGE(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MY_TYPE_WIDGET, MyPage))
#define MY_WIDGET_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MY_TYPE_WIDGET, MyPageClass))
#define MY_IS_WIDGET(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MY_TYPE_WIDGET))
#define MY_IS_WIDGET_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MY_TYPE_WIDGET))
#define MY_WIDGET_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MY_TYPE_WIDGET, MyPageClass))

typedef struct _MyPage             MyPage;
typedef struct _MyPageClass        MyPageClass;


enum {
	BUTTON_PORTS,
	COMBO_PORTS,
	PAGE_TEXTBUFFER,
	PAGE_TERMINAL
};

static const int readsocket = 0;
static const int writesocket = 1;

typedef struct page_midi_ports {
	char name[100];
	struct page_midi_ports *next;
} page_midi_ports;





typedef struct page_other {
	page_midi_ports *ports_root;	
	int index;
	char buf[30];
	int fd[2];
	fd_set fdread;
	int midi_fd[2];
	fd_set midi_fdread;
	GThread  *midi_thread;
	GThread  *wait_ports_thread;
	
} page_other;


struct _MyPage
{
	GtkPaned Paned;


	GtkWidget *button_ports;
	GtkWidget *combo_ports;
	GtkTextBuffer *page_textbuffer;
	GtkWidget *page_terminal;
	GtkWidget *page_combo_nodes;
	GtkWidget *page_node_combo_ports;
	GtkWidget *volume_button;
	GtkWidget *volume_spin;
	GtkWidget *stop_midi_thread_button;
	GtkWidget *instrument_spin;
	GtkWidget *channel_spin;
	page_other other;

};

struct _MyPageClass
{
	GtkPanedClass parent_class;
};


GtkWidget *my_page_new ();
void my_change_text(MyPage *widget, const gchar *text);
void midi_ports_refresh(MyPage *page, GtkButton *button);
gchar* get_ports_combo_box_text(GtkWidget *page, gpointer *user_data);
GtkWidget *return_page_field(MyPage *page, int field);
void add_data_ports_combo_box(MyPage *page);
void ip_wait_for_midi_ports(MyPage *page);
void vde_wait_for_midi_ports(MyPage *page);
void free_ports(page_midi_ports *tmp);
void initialize_page(MyPage *page, int index, char *string);
void add_data_page_combo_nodes(MyPage *page);
void add_data_page_node_combo_ports(GtkWidget *page, gpointer *user_data);
gchar* get_page_combo_nodes_text(GtkWidget *page, gpointer *user_data);
gchar* get_page_node_combo_ports_text(GtkWidget *page, gpointer *user_data);
void launch_midi(GtkWidget *page, gpointer *user_data);
void midi_control(GtkWidget *page, gpointer *user_data);
int what_mode_is(gchar *node_in, gchar* node_out);
GType my_widget_get_type();


G_END_DECLS

#endif /* __MY_WIDGET_H__ */
