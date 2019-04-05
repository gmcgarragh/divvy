#*******************************************************************************
#
# Copyright (C) 2010-2019 Greg McGarragh <greg.mcgarragh@colostate.edu>
#
# This source code is licensed under the GNU General Public License (GPL),
# Version 3.  See the file COPYING for more details.
#
#*******************************************************************************
.SUFFIXES: .c

include make.inc

OBJECTS = divvy.o \
          grmlib.o \
          version.o

EXTRA_CLEANS = version.c

all: divvy

divvy: $(OBJECTS)
	$(CC) $(CCFLAGS) -o divvy $(OBJECTS) \
        $(INCDIRS) $(LIBDIRS) $(LINKS)

version.o: version.c version.h

# Requires UNIX/LINUX date command and git
version.c:
	echo "#include \"version.h\"" > version.c; \
        echo "" >> version.c; \
        echo "const char *build_date  = \"`date +"%F %X %z"`\";" >> version.c; \
        echo "const char *build_sha_1 = \"`git describe --abbrev=7 --dirty --always --tags`\";" >> version.c

README: readme_source.txt
	fold --spaces --width=80 readme_source.txt > README
	sed -i 's/[ \t]*$$//' README

clean:
	rm -f *.o divvy $(EXTRA_CLEANS)

.c.o:
	$(CC) $(CCFLAGS) $(INCDIRS) -c -o $*.o $<

# Requires gcc
depend:
	@files=`find . -maxdepth 1 -name "*.c" | sort`; \
        if test $${#files} != 0; then \
             if (eval gcc -v 1> /dev/null 2>&1); then \
                  echo gcc -MM -w $$files "> depend.inc"; \
                  gcc -MM -w $$files > depend.inc; \
             else \
                  echo makedepend -f- -Y -- -- $$files "> depend.inc"; \
                  makedepend -f- -Y -- -- $$files > depend.inc; \
             fi \
        else \
             echo -n '' > depend.inc; \
        fi

include depend.inc
