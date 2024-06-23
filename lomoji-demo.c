/* lomoji-demo.c - Last Outpost Emoji Translation Library */
/* Created: Wed May 29 10:35:37 AM EDT 2024 malakai */
/* Copyright © 2024 Jeffrika Heavy Industries */
/* $Id: lomoji-demo.c,v 1.5 2024/06/22 21:40:05 malakai Exp $ */

/* Copyright © 2024 Jeff Jahr <malakai@jeffrika.com>
 *
 * This file is part of liblomoji - Last Outpost Emoji Translation Library
 *
 * liblomoji is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * liblomoji is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with liblomoji.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "lomoji.h"

int main(int argc, char *argv[]) {

	int direction = 0;
	int usefilter = 0;
	int suggestion = 0;
	char *comp;
	char line[8192];
	char *test;
	lomoji_filter *filter[] = {
		filter_uplus,
		NULL
	};

	if(argc == 1) {
		fprintf(stderr,"Try %s -h for help.\n",argv[0]);
		exit(0);
	} if(argc == 2) {
		if( !strcmp(argv[1],"-h")) {
			fprintf(stderr,"%s:\n",argv[0]);
			fprintf(stderr,"\t-h this message\n");
			fprintf(stderr,"\t-v version\n");
			fprintf(stderr,"\t-u translate ascii to utf8\n");
			fprintf(stderr,"\t-a translate utf8 to ascii\n");
			fprintf(stderr,"\t-U translate utf8 to \\U+000000\n");
			fprintf(stderr,"\t-s <keyword> suggest ascii completions\n");
			exit(0);
		} else if ( !strcmp(argv[1],"-v")) {
			fprintf(stderr,"%s\n",LOMOJI_VERSION);
			exit(0);
		} else if ( !strcmp(argv[1],"-u")) {
			direction = 1;
		} else if ( !strcmp(argv[1],"-U")) {
			direction = 0;
			usefilter = 1;
		} else if ( !strcmp(argv[1],"-a")) {
			direction = 0;
		} else if ( !strcmp(argv[1],"-s")) {
			fprintf(stderr,"Try %s -h for help.\n",argv[0]);
			exit(0);
		}
	} else if (argc == 3) {
		if( !strcmp(argv[1],"-s" ) ) {
			suggestion = 1;
			comp = argv[2];
		}
	}

	lomoji_init();
	// lomoji_set_param(LOMOJI_PREFIX,"[-");
	// lomoji_set_param(LOMOJI_SUFFIX,"-]");
	// lomoji_set_param(LOMOJI_UNKNOWN,"[-WHAT?-]");

	if(suggestion) {
		char *ans = lomoji_suggest(comp,0,NULL);
		if(!ans) {
			fprintf(stderr,"no matches for '%s'.\n",comp);
			const char *prefix = lomoji_get_param(LOMOJI_PREFIX);
			if(strncmp(comp,prefix,strlen(prefix))) {
				fprintf(stderr,"(searchs should start with the prefix '%s')\n",
					prefix
				);
			}
		} else {
			fprintf(stdout,"%s\n",ans);
			free(ans);
		}
		lomoji_done();
		exit(0);
	} 

	while(fgets(line,sizeof(line),stdin)) {
		if(direction) {
			test = lomoji_from_ascii(line);
		} else {
			if(usefilter) {
				test = lomoji_to_ascii_ext(lomoji_default_ctx,line,filter);
			} else {
				test = lomoji_to_ascii(line);
			}
		}
		fprintf(stdout,"%s",test);
		free(test);
	}

	/* clean up your mess. */
	lomoji_done();

	exit(0);

}

