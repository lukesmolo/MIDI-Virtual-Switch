CC      =gcc
#CFLAGS =`pkg-config --cflags gtk+-3.0 gmodule-2.0 gthread-2.0`
#LIBS    =`pkg-config --libs   gtk+-3.0 gmodule-2.0 gthread-2.0`
DEBUG   =-Wall -g -O2
#OTHERS  =-Wl,--export-dynamic
INCLUDE =-I../include
THREADS =-lpthread
MIDI    =-lasound
VDE     =-l vdeplug

OBJ = requests.o device.o midi_ports.o midi_device.o
all: device
clean:
	rm -fv *.o device

midi_ports.o: ../switch/midi_ports.c
	$(CC) -o midi_ports.o -c ../switch/midi_ports.c $(INCLUDE) $(DEBUG) $(MIDI) $(THREADS) $(VDE)

%.o:%.c
	$(CC) -o $@ -c $< $(INCLUDE) $(DEBUG) $(MIDI) $(THREADS) $(VDE)


device : $(OBJ)
	$(CC) -o $@ $^ $(DEBUG) $(MIDI) $(OTHERS) $(VDE) $(THREADS)




