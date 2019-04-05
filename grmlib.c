/*******************************************************************************
**
**    Copyright (C) 1998-2019 Greg McGarragh <greg.mcgarragh@colostate.edu>
**
**    This source code is licensed under the GNU General Public License (GPL),
**    Version 3.  See the file COPYING for more details.
**
*******************************************************************************/

/* Crap from grmlib */

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*******************************************************************************
 *
 ******************************************************************************/
void check_arg_count(int i, int argc, int n, const char *s) {

     if (i + n >= argc) {
          fprintf(stderr, "ERROR: not enough arguments for %s\n", s);
          exit(1);
     }
}



/*******************************************************************************
 *
 ******************************************************************************/
int strtoi_errmsg(const char *string, const char *name, int *value) {

     char *endptr;

     errno = 0;

     if (string == NULL) {
          fprintf(stderr, "ERROR: invalid value for %s: NULL\n", name);
          return -1;
     }

     if (string == NULL) {
          fprintf(stderr, "ERROR: invalid value for %s: empty string\n", name);
          return -1;
     }

     *value = strtol(string, &endptr, 0);

     if (errno == ERANGE) {
          if (*value == INT_MIN) {
               fprintf(stderr, "ERROR: invalid value for %s, too small: %s\n", name, string);
               return -1;
          }
          else {
               fprintf(stderr, "ERROR: invalid value for %s, too large: %s\n", name, string);
               return -1;
          }
     }

     if (*endptr != '\0') {
          fprintf(stderr, "ERROR: invalid value for %s: %s\n", name, string);
          return -1;
     }
/*
     if (sscanf(string, "%d", value) != 1) {
          fprintf(stderr, "ERROR: invalid value for %s: %s\n", name, string);
          return -1;
     }
*/
     return 0;
}



int strtoi_errmsg_exit(const char *string, const char *name) {

     int value;

     if (strtoi_errmsg(string, name, &value))
          exit(1);

     return value;
}



char *remove_pad(char *s, int side) {

     int i;

     if (side == 0 || side == 2) {
          for (i = 0; s[i] == ' '; ++i) ;
          s = s + i;
     }
     if (side == 1 || side == 2) {
          for (i = strlen(s) - 1; s[i] == ' '; --i) ;
          s[i+1] = '\0';
     }

     return s;
}
