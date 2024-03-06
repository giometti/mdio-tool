/*
mdio-tool allow for direct access to mdio registers in a network phy.

Routines are based on mii-tool: http://freecode.com/projects/mii-tool

mdio-tool comes with ABSOLUTELY NO WARRANTY; Use with care!

Copyright (C) 2013 Pieter Voorthuijsen

mdio-tool is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

mdio-tool is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with mdio-tool.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/sockios.h>

#ifndef __GLIBC__
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#endif
#include "mii.h"

static int skfd = -1;		/* AF_INET socket for ioctl() calls. */
static struct ifreq ifr;

/*--------------------------------------------------------------------*/

static int mdio_read(int skfd, int location)
{
    struct mii_data *mii = (struct mii_data *)&ifr.ifr_data;
    mii->reg_num = location;
    if (ioctl(skfd, SIOCGMIIREG, &ifr) < 0) {
	fprintf(stderr, "SIOCGMIIREG on %s failed: %s\n", ifr.ifr_name,
		strerror(errno));
	return -1;
    }
    return mii->val_out;
}

static void mdio_write(int skfd, int location, int value)
{
    struct mii_data *mii = (struct mii_data *)&ifr.ifr_data;
    mii->reg_num = location;
    mii->val_in = value;
    if (ioctl(skfd, SIOCSMIIREG, &ifr) < 0) {
	fprintf(stderr, "SIOCSMIIREG on %s failed: %s\n", ifr.ifr_name,
		strerror(errno));
    }
}

/*
 * Usage
 */

static void usage(const char *name)
{
        printf("usage:\n");
        printf("\t%s [<options>] r <dev> <reg>\n", name);
        printf("\t%s [<options>] w <dev> <reg> <val>\n", name);
        printf("where <options> can be:\n");
        printf("\t-h | --help                       : show this message\n");
        exit(EXIT_SUCCESS);
}

/*
 * Main
 */

int main(int argc, char **argv)
{
        int c;
        struct option long_options[] = {
                { "help",       no_argument, 0, 'h' },
		{ /* terminator */ }
        };
	int option_index = 0;
	int addr, dev, val;
	struct mii_data *mii = (struct mii_data *)&ifr.ifr_data;

        /* Check the command line */
        while (1) {
                c = getopt_long(argc, argv, "h",
                                long_options, &option_index);
                if (c == -1)
                        break;
                switch (c) {
                case 'h' :
                        usage(argv[0]);

                case ':' :
                case '?' :
                default :
                        exit(EXIT_FAILURE);
                }
        }
        if (argc - optind < 2)
		usage(argv[0]);

	/* Open a basic socket. */
	if ((skfd = socket(AF_INET, SOCK_DGRAM,0)) < 0) {
		perror("socket");
		return -1;
	}

	/* Get the vitals from the interface. */
	strncpy(ifr.ifr_name, argv[optind + 1], IFNAMSIZ);
	if (ioctl(skfd, SIOCGMIIPHY, &ifr) < 0) {
		if (errno != ENODEV)
		fprintf(stderr, "SIOCGMIIPHY on '%s' failed: %s\n",
			argv[optind + 1], strerror(errno));
		return -1;
	}

	if(argv[optind + 0][0] == 'r') {
		addr = strtol(argv[optind + 2], NULL, 16);
		printf("0x%.4x\n", mdio_read(skfd, addr));
	}
	else if(argv[optind + 0][0] == 'w') {
		addr = strtol(argv[optind + 2], NULL, 16);
		val = strtol(argv[optind + 3], NULL, 16);
		mdio_write(skfd, addr, val);
	}
	else {
		printf("Fout!\n");
	}

	close(skfd);
}
