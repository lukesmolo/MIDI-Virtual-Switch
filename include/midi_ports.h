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


typedef struct midi_subdev {		/*struct for subdevice*/
	int sub;
	char name[50];
	char io[2];
	char hw[20];
	struct midi_subdev *next;
} midi_subdev;

typedef struct midi_dev {		/*struct for device*/
	int dev;
	char name[50];
	char io[2];
	char hw[20];
	int n_sub;
	struct midi_dev *next;
	struct midi_subdev *sub_root;

} midi_dev;

typedef struct midi_card {		/*struct for card*/
	int card;
	char name[50];
	char hw[20];
	int n_dev;
	int is_midi;
	struct midi_card *next;
	struct midi_dev *dev_root; 	/*NULL if there are 16 subdevices*/
} midi_card;


midi_card* return_midi_ports(void);
