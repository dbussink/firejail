/*
 * Copyright (C) 2014-2016 Firejail Authors
 *
 * This file is part of firejail project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "firejail.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/mount.h>

void fs_x11(void) {
	// extract display
	char *d = getenv("DISPLAY");
	if (!d)
		return;
	
	int display;
	int rv = sscanf(d, ":%d", &display);
	if (rv != 1)
		return;
	if (arg_debug)
		printf("DISPLAY %s, %d\n", d, display);

	char *x11file;
	if (asprintf(&x11file, "/tmp/.X11-unix/X%d", display) == -1)
		errExit("asprintf");
	struct stat s;
	if (stat(x11file, &s) == -1)
		return;

	// keep a copy of real /tmp/.X11-unix directory in WHITELIST_TMP_DIR
	rv = mkdir(RUN_WHITELIST_X11_DIR, 1777);
	if (rv == -1)
		errExit("mkdir");
	if (chown(RUN_WHITELIST_X11_DIR, 0, 0) < 0)
		errExit("chown");
	if (chmod(RUN_WHITELIST_X11_DIR, 1777) < 0)
		errExit("chmod");

	if (mount("/tmp/.X11-unix", RUN_WHITELIST_X11_DIR, NULL, MS_BIND|MS_REC, NULL) < 0)
		errExit("mount bind");

	// mount tmpfs on /tmp/.X11-unix
	if (arg_debug || arg_debug_whitelists)
		printf("Mounting tmpfs on /tmp/.X11-unix directory\n");
	if (mount("tmpfs", "/tmp/.X11-unix", "tmpfs", MS_NOSUID | MS_STRICTATIME | MS_REC,  "mode=1777,gid=0") < 0)
		errExit("mounting tmpfs on /tmp");
	fs_logger("mount tmpfs on /tmp/.X11-unix");

	// create an empty file
	FILE *fp = fopen(x11file, "w");
	if (!fp) {
		fprintf(stderr, "Error: cannot create empty file in x11 directory\n");
		exit(1);
	}
	fclose(fp);

	// set file properties
	if (chown(x11file, s.st_uid, s.st_gid) < 0)
		errExit("chown");
	if (chmod(x11file, s.st_mode) < 0)
		errExit("chmod");

	// mount
	char *wx11file;
	if (asprintf(&wx11file, "%s/X%d", RUN_WHITELIST_X11_DIR, display) == -1)
		errExit("asprintf");
	if (mount(wx11file, x11file, NULL, MS_BIND|MS_REC, NULL) < 0)
		errExit("mount bind");
	 fs_logger2("whitelist", x11file);

	free(x11file);
	free(wx11file);
	
	// block access to RUN_WHITELIST_X11_DIR
	 if (mount(RUN_RO_DIR, RUN_WHITELIST_X11_DIR, "none", MS_BIND, "mode=400,gid=0") == -1)
	 	errExit("mount");
	 fs_logger2("blacklist", RUN_WHITELIST_X11_DIR);
}


void x11_start(int argc, char **argv) {
	EUID_ASSERT();
	int i;
	struct stat s;
	pid_t client = 0;
	pid_t server = 0;
	
	// check xpra
	if (stat("/usr/bin/xpra", &s) == -1) {
		fprintf(stderr, "\nError: Xpra program was not found in /usr/bin directory, please install it:\n");
		fprintf(stderr, "   Arch: sudo pacman -S xpra\n");
		fprintf(stderr, "   Debian/Ubuntu/Mint: sudo apt-get install xpra\n");
		fprintf(stderr, "   Fedora/CentOS: sudo yum install xpra\n");
		fprintf(stderr, "   Gentoo: sudo emerge xpra\n\n");
		exit(0);
	}
	
	int display;
	int found = 1;
	for (i = 0; i < 100; i++) {
		display = rand() % 1024;
		char *fname;
		if (asprintf(&fname, "/tmp/.X11-unix/X%d", display) == -1)
			errExit("asprintf");
		if (stat(fname, &s) == -1) {
			found = 1;
			break;
		}
	}
	if (!found) {
		fprintf(stderr, "Error: cannot pick up a random X11 display number, exiting...\n");
		exit(1);
	}

	// build the start command
	int len = 50; // xpra start...
	for (i = 0; i < argc; i++) {
		len += strlen(argv[i]) + 1; // + ' '
	}
	
	char *cmd1 = malloc(len + 1); // + '\0'
	if (!cmd1)
		errExit("malloc");
	
	sprintf(cmd1, "xpra start :%d --exit-with-children --start-child=\"", display);
	char *ptr = cmd1 + strlen(cmd1);
	for (i = 0; i < argc; i++) {
		if (strcmp(argv[i], "--x11") == 0)
			continue;
		ptr += sprintf(ptr, "%s ", argv[i]);
	}
	sprintf(ptr, "\"");
	if (arg_debug)
		printf("xpra server: %s\n", cmd1);	
	
	// build the attach command
	char *cmd2;
	if (asprintf(&cmd2, "xpra attach :%d", display) == -1)
		errExit("asprintf");
	if (arg_debug)
		printf("xpra client: %s\n", cmd2);

	signal(SIGHUP,SIG_IGN);	// fix sleep(1`) below
	server = fork();
	if (server < 0)
		errExit("fork");
	if (server == 0) {
		if (arg_debug)
			printf("Starting xpra...\n");
	
		char *a[4];
		a[0] = "/bin/bash";
		a[1] = "-c";
		a[2] = cmd1;
		a[3] = NULL;
	
		execvp(a[0], a); 
		perror("execvp");
		exit(1);
	}
	sleep(1);
	if (arg_debug) {
		printf("X11 sockets: "); fflush(0);
		int rv = system("ls /tmp/.X11-unix");
		(void) rv;
	}

	// check X11 socket
	char *fname;
	if (asprintf(&fname, "/tmp/.X11-unix/X%d", display) == -1)
		errExit("asprintf");
	if (stat(fname, &s) == -1) {
		fprintf(stderr, "Error: failed to start xpra\n");
		exit(1);
	}
	
	// run attach command
	client = fork();
	if (client < 0)
		errExit("fork");
	if (client == 0) {
		printf("\n*** Attaching to xpra display %d ***\n\n", display);
		char *a[4];
		a[0] = "/bin/bash";
		a[1] = "-c";
		a[2] = cmd2;
		a[3] = NULL;
	
		execvp(a[0], a); 
		perror("execvp");
		exit(1);
	}
	sleep(1);
	
	if (!arg_quiet)
		printf("Xpra server pid %d, client pid %d\n", server, client);
	exit(0);
}
