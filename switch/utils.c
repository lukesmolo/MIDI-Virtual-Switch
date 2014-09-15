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

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<stdarg.h>
#include "MyPage.h"
#include "main.h"
#include "utils.h"




void
print_terminal(int type, MyPage* page, const char* Format, ...) { /*prints a message on the terminal */
	GtkTextIter end;
	GtkWidget *terminal = NULL;
	GtkTextBuffer *buffer = NULL;
	char buf[512]; // you can define your own buffer’s size
	va_list args;

	switch(type) {
		case HOME:
			buffer = main_textbuffer;
			terminal = main_terminal;
			break;
		case PAGE:
			buffer = return_page_field(page, PAGE_TEXTBUFFER);
			terminal = return_page_field(page, PAGE_TERMINAL);
			break;

		default: return;

	}

	va_start(args,Format);
	vsprintf(buf,Format,args);
	va_end(args);

	gtk_text_buffer_insert_at_cursor(buffer, buf, strlen(buf));	/*inserts text into text buffer*/

	gtk_text_buffer_get_iter_at_offset (buffer, &end, -1);	/*sets iter to the end of text buffer*/
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(terminal), buffer);	/*inserts the text buffer into the view*/
	gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(terminal), &end, 0.0, FALSE, 0.0, 0.0);	/*scrolls view until the iter of text buffer*/
}

char *
split_string(char* string) {
	string = strtok(string, "\t");
	return string;
}



