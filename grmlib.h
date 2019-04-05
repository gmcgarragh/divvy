/*******************************************************************************
**
**    Copyright (C) 1998-2019 Greg McGarragh <greg.mcgarragh@colostate.edu>
**
**    This source code is licensed under the GNU General Public License (GPL),
**    Version 3.  See the file COPYING for more details.
**
*******************************************************************************/

/* Crap from grmlib */


void check_arg_count(int i, int argc, int n, const char *s);
int strtoi_errmsg(const char *string, const char *name, int *value);
int strtoi_errmsg_exit(const char *string, const char *name);
char *remove_pad(char *s, int side);
