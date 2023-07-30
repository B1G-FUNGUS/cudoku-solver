#include "clargs.h"

// TODO juggling static memory and passing by address is wacky, clean up

size_t fl_length;
Flag *fl;
SArgV *sargv;

void append(Arg arg) {
	sargv->alist = (Arg *) reallocarray(sargv->alist, ++(sargv->length),
		sizeof(Arg));
	if (sargv->alist == NULL) {
		printf("Ran out of memory!\n");
		exit(1);
	}
	sargv->alist[sargv->length-1] = arg;
}

// Looping through all the flags in the next two functions obviously isn't
// efficient, but you can have at max 255 different flags so I'm not worried

Flag *get_lflag(char *str) {
	for (int i = 0; i < fl_length; i++) {
		if (strcmp(str, fl[i].longname) == 0) {
			return &fl[i];
					printf("a\n");
		}
	}
	printf("Unknown Flag: --%s\n", str);
	exit(1);
}

Flag *get_sflag(char c) {
	for (int i = 0; i < fl_length; i++) {
		if (c == fl[i].shortname) {
			return &fl[i];
		}
	}
	printf("Unknown Flag: -%c\n", c);
	exit(1);
}

char *lflag_val(Flag *flag, int *aip, int argc, char *argv[]) {
	if (flag->takesarg) {
		if (argc <= *aip) {
			printf("--%s takes an argument\n", flag->longname);
			exit(1);
		}
		return argv[++(*aip)];
	} else {
		return NULL;
	}
}

char *sflag_val(Flag *flag, int *aip, int argc, char *argv[], int *cip) {
	if (flag->takesarg) {
		if (argc <= *aip || argv[*aip][++(*cip)] != '\0') {
			printf("-%c takes an argument\n", flag->shortname);
			exit(1);
		}
		*cip = -1;
		return argv[++(*aip)];
	} else {
		return NULL;
	}
}

SArgV *mksargs(int argc, char *argv[], Flag flist[], int flist_length) {
	argc--; // TODO i was under the impression argc started at 0
	fl = flist;
	fl_length = flist_length;
	sargv = (SArgV *) malloc (sizeof(SArgV));
	for (int ai = 1; ai <= argc; ai++) {
		if (argv[ai][0] == '-') {
			if (argv[ai][1] == '-') {
				Flag *flag;
				flag = get_lflag(&argv[ai][2]);
				char *str;
				str = lflag_val(flag, &ai, argc, argv);
				Arg arg = {flag->shortname, str};
				append(arg);
			} else {
				for (int ci = 1; ci && argv[ai][ci]; ci++) {
					Flag *flag;
					flag = get_sflag(argv[ai][ci]);
					char *str;
					str = sflag_val(flag, &ai, argc, argv, 
						&ci);
					Arg arg = {flag->shortname, str};
					append(arg);
				}
			}
		} else {
			Arg arg = {0, argv[ai]};
			append(arg);
		}
	}
	return sargv;
}
