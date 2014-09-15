CC      =gcc
CFLAGS =`pkg-config --cflags gtk+-3.0 gmodule-2.0 gthread-2.0`
LIBS    =`pkg-config --libs   gtk+-3.0 gmodule-2.0 gthread-2.0`
DEBUG   =-Wall -g -O2
OTHERS  =-Wl,--export-dynamic
INCLUDE =-I../include
THREADS =-lpthread
MIDI    =-lasound
VDE     =-l vdeplug

OBJ =main.o midi_ports.o midi_nodes.o MyPage.o UI/my_resources.o utils.o notebook.o server.o midi_switch.o

all: main

clean:
	rm -fv *~ *.o main UI/my_resources.h UI/my_resources.c UI/my_resources.o


UI/my_resources.h: UI/my_resources.xml $(UI_FILES)
	glib-compile-resources UI/my_resources.xml \
		--target=$@ --sourcedir=$(srcdir) --c-name _my --generate-header
UI/my_resources.c: UI/my_resources.xml $(UI_FILES)
	glib-compile-resources UI/my_resources.xml \
		--target=$@ --sourcedir=$(srcdir) --c-name _my --generate-source

%.o:%.c
	gcc -o $@ -c $< $(INCLUDE) $(DEBUG) $(MIDI) $(LIBS) $(OTHERS) $(CFLAGS) $(THREADS) $(VDE)


main: $(OBJ)
		$(CC) -o $@ $^ $(DEBUG) $(MIDI) $(LIBS) $(OTHERS) $(CFLAGS) $(THREADS) $(VDE)




