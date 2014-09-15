#MIDI Virtual Switch

MIDI Virtual Switch is a virtual switch for the interconnection of several digital
musical instruments. From the Switch it is possible to monitor and possibly modify
all the MIDI packets travelling through it.

Instruments can be connected by using a physical network, or a virtual network,
		or directly a MIDI connection.

In case of a physical network or a virtual network, a computer is needed as
intermediate node in the network. It reads MIDI from the instrument and it
forwards the traffic to the Switch.




##Environment Configuration
Basing on this idea, several different configurations can be created.
For instance, during the experimental phase, configurations as the images below have been tested.
The first represents a way for sending MIDI packets made by a digital keyboard
through a physical network. Once packets have reached the board ([an Udoo
board, in this case](http://www.udoo.org/)), they can be possibly
modified and played.
The same for the second image, even if in this case a virtual network has been
used for sending packets.

In both cases, MIDI Virtual Switch can run either on the laptop, or on the
board.<br>

<img src="/pics/midi_network_with_switch.png" width="280">
<img src="/pics/midi_network.png" width="280">

##Goal
The most optimistic goal of MIDI Virtual Switch is to try to make the same work of a physical
mixer on a real musical stage, where instead of using analogic connections for
musical instruments, they are connected by using networks, physical or virtual
ones. Images below represent exactly this idea.<br>

<img src="/pics/example.png" width="280">
<img src="/pics/example1.png" width="280">

##GUI
A very basic GUI is provided and it is required only by the Switch.

##Requirements
* You need to install ALSA
* You need to load the [VirMidi](http://alsa.opensrc.org/VirMidi) kernel module for reading/writing MIDI packets.
* You need to install GTK+3 for using the GUI in the Switch
* You need to install [vde2](http://wiki.v2.cs.unibo.it/wiki/index.php?title=Main_Page) for using the Virual Network
* You need to install a software synthesizer for playing MIDI packets, for
instance [Timidity++](http://timidity.sourceforge.net/), [Fluidsynth](http://www.fluidsynth.org/), etc...

##Usage
Clone the repository both on the computer you want to use as Switch and on all
your intermediate nodes:
```
$ git clone --recursive git@github.com:lukesmolo/MIDI-Virtual-Switch.git
```
On your Switch, compile the program going inside the _switch_ directory:
```
$ cd switch
$ make
```
Run the program:
```
$ ./main
```

On your intermediate node, compile the program going inside the _device_ directory:
```
$ cd device
$ make
```

Run the program, specifying the address of the Switch, the type of connection
and a name for the intermediate node:
```
$ ./device 127.0.0.1 IP keyboard
```
or
```
$ ./device /tmp/xxx VDE keyboard
```
or
```
$ ./device 224.1.2.3 VXVDE keyboard
```

#Benchmark
Since MIDI is incapsulated inside a network packet (IP, or ETH), an extra delay is introduced. In order to understand if it is heavy for the all computation, some
tests have been made: for example, let's see the behaviour of a MIDI flow of 10000 packets .<br>
In the **first table** only a flow entirely on the Virtual Switch computer is
considered, using different connections.
Each value in tables means the time required by a packet that has to be read from a digital
instrument, has to be sent and then has to be played.<br>
Actually, a min time equal to 0 does not mean an unreal time, but simply it is not
detected in the order of _ms_, hence a potential delay is not a problem for human hearing.

| Connection | Max time (ms) | Min time (ms) | Mean time (ms) |
| ---------- | -------- | -------- | --------- |
| **MIDI <sup>1</sup>** | 186.00 | 0 | 44.74 |
| **UDP <sup>2</sup>**| 186.00 | 0 | 44.75 |
| **VDE**| 186.00 | 0 | 44.74 |

<sup>1 Default MIDI connection of the digital instrument and a USB 2.0 adapter
have been used</sup><br>
<sup>2 Network loopback interface has been used</sup>


In the **second table** a flow from an _Udoo_ board to the Virtual Switch is
considered.

| Connection | Max time (ms) | Min time (ms) | Mean time (ms) |
| ---------- | -------- | -------- | --------- |
| **UDP** | 186.00 | 0 | 44.75 |
| **UDP <sup>1</sup>**| 186.00 | 0 | 44.73 |
| **VDE**| 192.00 | 0 | 49.57 |
| **VDE <sup>1</sup>**| 192.00 | 0 | 47.96 |

<sup>1 There was an extra physical switch in the network</sup>

As you can see, it seems there are no big differences between the pure MIDI
connection and the MIDI incapsulation, both when the flow is on the same computer and when it travels through a network. Furthermore, a fixed maximum time seems to be an upper limit. It could represent the worst case.


##Status of the project
Actually this project is a proof of concept, since several features have still to be
implemented and something has to be redesigned. This was also my Bachelor's degree.

Now I am working on MIDI Virtual Switch2, where the GUI is moved to web
interface for a better portability and more features are implemented.

#Contributing
I would be very glad if you want to contribute to improving this project.<br>
Please fork it and submit a pull request! :sunglasses:

##License
MIDI Virtual Switch is released under the GPLv2 license.

##TODO
* There are still some most common MIDI features to implement
* Opening more connections to a single intermediate node
* Splitting MIDI channels in order to redirect their flow on different devices
* Move graphical interface to web
