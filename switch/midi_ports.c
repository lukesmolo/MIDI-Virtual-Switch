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


#include <alsa/asoundlib.h>
#include "midi_ports.h"     /* Interface to the ALSA system */


int
is_output(snd_ctl_t *ctl,int dev, int sub) { /*return true if the device/subdevice accepts input streams*/
	snd_rawmidi_info_t *info;
	int status;

	snd_rawmidi_info_alloca(&info);
	snd_rawmidi_info_set_device(info, dev);
	snd_rawmidi_info_set_subdevice(info, sub);
	snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_OUTPUT);

	if ((status = snd_ctl_rawmidi_info(ctl, info)) < 0 && status != -ENXIO) {
		return status;
	} else if (status == 0) {
		return 1;
	}

	return 0;
}



int
is_input(snd_ctl_t *ctl, int dev, int sub) {	/*return true if the device/subdevice accepts output streams*/
	snd_rawmidi_info_t *info;
	int status;

	snd_rawmidi_info_alloca(&info);
	snd_rawmidi_info_set_device(info, dev);
	snd_rawmidi_info_set_subdevice(info, sub);
	snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);

	if ((status = snd_ctl_rawmidi_info(ctl, info)) < 0 && status != -ENXIO) {
		return status;
	} else if (status == 0) {
		return 1;
	}

	return 0;
}




midi_subdev*
get_midi_subdevice(midi_dev *dev, snd_ctl_t *ctl, int n_card, int n_dev) {	/*get subdevices information*/

	snd_rawmidi_info_t *info;
	midi_subdev *tmp;
	midi_subdev *root = dev->sub_root;
	const char *sub_name;
	const char *name;
	int subs, subs_in, subs_out;
	int sub, in, out;
	int status;

	sub = 0;
	in = out = 0;

	snd_rawmidi_info_alloca(&info);
	snd_rawmidi_info_set_device(info, n_dev);

	snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);
	snd_ctl_rawmidi_info(ctl, info);
	subs_in = snd_rawmidi_info_get_subdevices_count(info);
	snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_OUTPUT);
	snd_ctl_rawmidi_info(ctl, info);
	subs_out = snd_rawmidi_info_get_subdevices_count(info);
	subs = subs_in > subs_out ? subs_in : subs_out;


	if ((status = is_output(ctl, n_dev, sub)) < 0) {
		printf("cannot get rawmidi information %d:%d: %s",
				n_card, n_dev, snd_strerror(status));
		return NULL;
	} else if (status)
		out = 1;

	if (status == 0) {
		if ((status = is_input(ctl, n_dev, sub)) < 0) {
			printf("cannot get rawmidi information %d:%d: %s",
					n_card, n_dev, snd_strerror(status));
			return NULL;
		}
	} else if (status)
		in = 1;

	if (status == 0)
		return NULL;

	name = snd_rawmidi_info_get_name(info);
	sub_name = snd_rawmidi_info_get_subdevice_name(info);


	dev->n_sub = subs;
	if(sub_name[0] == '\0') { /* if there is not a specific subdevice ..*/
		dev->sub_root = NULL;
		strncpy(dev->name, name, strlen(name));

		if(in)		/*I/O information of dev*/
			dev->io[0] = 'I';
		if(out)
			dev->io[1] = 'O';

	} else { /*if there is a specific subdevice ...*/
		sub = 0;
		while(1) {

			if(!root) {			/* creates the list of subdevices for that device*/
				root = malloc(sizeof(midi_subdev));
				tmp = root;

			} else {
				tmp = root;

				while(tmp->next != NULL)
					tmp = tmp->next;
				tmp->next = malloc(sizeof(midi_subdev));
				tmp = tmp->next;
			}
			
			/*struct of subdevice initialization*/
			memset(tmp->name, 0, 30);	
			memset(tmp->hw, 0, 20);
			memset(tmp->io, 0, 2);
			tmp->sub = sub;
			tmp->next = NULL;
			in = out = 0;

			in = is_input(ctl, n_dev, sub);
			out = is_output(ctl, n_dev, sub);
			snd_rawmidi_info_set_subdevice(info, sub);
			strncpy(tmp->name, sub_name, strlen(sub_name));
			sprintf(tmp->hw, "hw:%d,%d,%d", n_card, dev->dev, tmp->sub);

			if(in)
				tmp->io[0] = 'I';

			if(out)
				tmp->io[1] = 'O';
			if (out) {
				snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_OUTPUT);
				if ((status = snd_ctl_rawmidi_info(ctl, info)) < 0) {
					printf("cannot get rawmidi information %d:%d:%d: %s\n",
							n_card, n_dev, sub, snd_strerror(status));
					break;
				}
			} else {
				snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);
				if ((status = snd_ctl_rawmidi_info(ctl, info)) < 0) {
					printf("cannot get rawmidi information1 %d:%d:%d: %s\n",
							n_card, n_dev, sub, snd_strerror(status));
					break;
				}
			}
			sub_name = snd_rawmidi_info_get_subdevice_name(info);
			if (++sub >= subs)
				break;

		}

	}
	return root;
}


midi_dev*
get_midi_dev(midi_card *port, int n_card, snd_ctl_t *ctl) { /*get device information*/

	midi_dev* tmp = NULL;
	midi_dev* root = port->dev_root;

	int dev = -1;
	int status;
	int n_dev = -1;
	do {
		status = snd_ctl_rawmidi_next_device(ctl, &dev);
		if (status < 0) {
			printf("cannot determine device number: %s", snd_strerror(status));
			break;
		}
		if (dev >= 0) { /*if there is almost a device ...*/
			port->is_midi = 1;
			if(!root) { 		/*creates the list of devices*/
				root = malloc(sizeof(midi_dev));
				tmp = root;

			} else {
				tmp = root;
				while(tmp->next != NULL)
					tmp = tmp->next;
				tmp->next = malloc(sizeof(midi_dev));
				tmp = tmp->next;
			}

			/*struct of device initialization*/
			memset(tmp->name, 0, 30);
			memset(tmp->io, 0, 2);
			memset(tmp->hw, 0, 20);
			tmp->next = NULL;
			tmp->n_sub = 0;
			tmp->dev = dev;
			tmp->sub_root = NULL;
			sprintf(tmp->hw, "hw:%d,%d", n_card, tmp->dev);
			tmp->sub_root = get_midi_subdevice(tmp, ctl, n_card, dev);
		}
		n_dev++;
	} while (dev >= 0);


	port->n_dev = n_dev;
	return root;

}




midi_card*
get_midi_cards(midi_card *root) { /*get cards information*/
	int status;
	midi_card *tmp;
	char* shortname = NULL;
	int n_card = -1;
	snd_ctl_t *ctl;
	if ((status = snd_card_next(&n_card)) < 0) {
		printf("cannot determine card number: %s", snd_strerror(status));
	}
	if (n_card < 0) {
		printf("there are not sound cards\n");
	}	

	while(n_card > -1) {

		if(n_card >= 0) { /*create the list of cards*/
			if(!root) {
				root = malloc(sizeof(midi_card));
				tmp = root;


			} else {
				tmp = root;
				while(tmp->next != NULL)
					tmp = tmp->next;
				tmp->next = malloc(sizeof(midi_card));
				tmp = tmp->next;

			}
			/*card struct initialization*/
			tmp->card = n_card;
			memset(tmp->name, 0, 30);
			memset(tmp->hw, 0, 20);
			tmp->next = NULL;
			tmp->is_midi= 0;
			tmp->n_dev = 0;
			tmp->dev_root = NULL;
			if ((status = snd_card_get_name(n_card, &shortname)) < 0) {
				printf("cannot determine card name: %s", snd_strerror(status));
				break;
			}

			strncpy(tmp->name, shortname, strlen(shortname));

			sprintf(tmp->hw, "hw:%d", n_card);
			if ((status = snd_ctl_open(&ctl, tmp->hw, 0)) < 0) {
				printf("cannot open control for card %d: %s", n_card, snd_strerror(status));
				return NULL;
			}

			tmp->dev_root = get_midi_dev(tmp , n_card, ctl);

			if ((status = snd_card_next(&n_card)) < 0) {
				printf("cannot determine card number: %s", snd_strerror(status));
				break;
			}

		}
	}
	if(ctl)
		snd_ctl_close(ctl);
	return root;

}

void
print_midi_information(midi_card *root) {
	midi_card* tmp = NULL;
	midi_dev *tmp1 = NULL;
	midi_subdev *tmp2 = NULL;
	tmp = root;

	while(tmp) {
		if(tmp->is_midi) {
			tmp1 = tmp->dev_root;
			printf("card name: %s", tmp->name);
			printf("hw: %s\t", tmp->hw);
			printf("devices number: %d\t", tmp->n_dev);
			while(tmp1 != NULL) {
				printf("%s\n", tmp1->name);
				printf("%.2s\n", tmp1->io);
				printf("hw: %s\t", tmp1->hw);
				printf("subdevices number: %d\n", tmp1->n_sub);
				tmp2 = tmp1->sub_root;
				while(tmp2) {
					printf("%.2s\n", tmp2->io);
					printf("hw: %s\t", tmp2->hw);
					printf("subname: %s\n", tmp2->name);
					tmp2 = tmp2->next;
				}

				tmp1 = tmp1->next;
			}
			printf("\n");

		}
		tmp = tmp->next;

	}

}

midi_card*
return_midi_ports(){

	midi_card *root = NULL;

	root = get_midi_cards(root);
	if(!root) {
		printf("No sound cards! Closing ...\n");
		exit(0);
	}


	/*print_midi_information(root);
	if(root)
		free(root);*/
	return root;

}
