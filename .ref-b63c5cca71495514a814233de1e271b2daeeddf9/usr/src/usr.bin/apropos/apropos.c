/*
 * Copyright (c) 1987, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1987, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)apropos.c	8.8 (Berkeley) %G%";
#endif /* not lint */

#include <sys/param.h>
#include <sys/queue.h>

#include <ctype.h>
#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../man/config.h"
#include "../man/pathnames.h"

static int *found, foundman;

void apropos __P((char **, char *, int));
void lowstr __P((char *, char *));
int match __P((char *, char *));
void usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	ENTRY *ep;
	TAG *tp;
	int ch, rv;
	char *conffile, **p, *p_augment, *p_path;

	conffile = NULL;
	p_augment = p_path = NULL;
	while ((ch = getopt(argc, argv, "C:M:m:P:")) != EOF)
		switch (ch) {
		case 'C':
			conffile = optarg;
			break;
		case 'M':
		case 'P':		/* backward compatible */
			p_path = optarg;
			break;
		case 'm':
			p_augment = optarg;
			break;
		case '?':
		default:
			usage();
		}
	argv += optind;
	argc -= optind;

	if (argc < 1)
		usage();

	if ((found = malloc((u_int)argc * sizeof(int))) == NULL)
		err(1, NULL);
	memset(found, 0, argc * sizeof(int));

	for (p = argv; *p; ++p)			/* convert to lower-case */
		lowstr(*p, *p);

	if (p_augment)
		apropos(argv, p_augment, 1);
	if (p_path || (p_path = getenv("MANPATH")))
		apropos(argv, p_path, 1);
	else {
		config(conffile);
		ep = (tp = getlist("_whatdb")) == NULL ?
		    NULL : tp->list.tqh_first;
		for (; ep != NULL; ep = ep->q.tqe_next)
			apropos(argv, ep->s, 0);
	}

	if (!foundman)
		errx(1, "no %s file found", _PATH_WHATIS);

	rv = 1;
	for (p = argv; *p; ++p)
		if (found[p - argv])
			rv = 0;
		else
			(void)printf("%s: nothing appropriate\n", *p);
	exit(rv);
}

void
apropos(argv, path, buildpath)
	char **argv, *path;
	int buildpath;
{
	char *end, *name, **p;
	char buf[LINE_MAX + 1], wbuf[LINE_MAX + 1];

	for (name = path; name; name = end) {	/* through name list */
		if (end = strchr(name, ':'))
			*end++ = '\0';

		if (buildpath) {
			char hold[MAXPATHLEN + 1];

			(void)sprintf(hold, "%s/%s", name, _PATH_WHATIS);
			name = hold;
		}

		if (!freopen(name, "r", stdin))
			continue;

		foundman = 1;

		/* for each file found */
		while (fgets(buf, sizeof(buf), stdin)) {
			if (!strchr(buf, '\n')) {
				warnx("%s: line too long", name);
				continue;
			}
			lowstr(buf, wbuf);
			for (p = argv; *p; ++p)
				if (match(wbuf, *p)) {
					(void)printf("%s", buf);
					found[p - argv] = 1;

					/* only print line once */
					while (*++p)
						if (match(wbuf, *p))
							found[p - argv] = 1;
					break;
				}
		}
	}
}

/*
 * match --
 *	match anywhere the string appears
 */
int
match(bp, str)
	char *bp, *str;
{
	int len;
	char test;

	if (!*bp)
		return (0);
	/* backward compatible: everything matches empty string */
	if (!*str)
		return (1);
	for (test = *str++, len = strlen(str); *bp;)
		if (test == *bp++ && !strncmp(bp, str, len))
			return (1);
	return (0);
}

/*
 * lowstr --
 *	convert a string to lower case
 */
void
lowstr(from, to)
	char *from, *to;
{
	char ch;

	while ((ch = *from++) && ch != '\n')
		*to++ = isupper(ch) ? tolower(ch) : ch;
	*to = '\0';
}

/*
 * usage --
 *	print usage message and die
 */
void
usage()
{

	(void)fprintf(stderr,
	    "usage: apropos [-C file] [-M path] [-m path] keyword ...\n");
	exit(1);
}
