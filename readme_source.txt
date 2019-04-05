DESCRIPTION
------------
A stupid little program that distributes processes using OpenMP (for shared memory multi-core environments) or with MPI (for distributed memory environments [clusters]).  Of course MPI can be used for the shared memory environments also.  Are there other solutions?  Yes.  But this is mine.  I have not encountered bugs in it for over 8 years.  Sorry, there are no comments in it but it is less than 500 lines of C so should be fairly readable.


INSTALLATION
------------
The source maybe obtained by git cloning from github.com with:

git clone https://github.com/gmcgarragh/divvy.git

The main code is C89 compliant and is dependent only on the C standard library, an MPI library, and a compiler that supports OpenMP.  The build system requires GNU Make, with which the following steps will compile the code:

1) Copy make.inc.example to make.inc.
2) Edit make.inc according to the comments within.
3) Run make.

The default setup in make.inc.example is for GCC and should work on any GNU/Linux or MacOS systems with programming tools installed and probably most modern UNIX systems.

For other platforms/environments, such as MS Windoze Visual C++, it is up to user to set up the build within that platform/environment.

After the build the binary will be located in the same directory as the source.  It is up to the user to move it to or link to it from other locations.


USAGE
-----
The main argument is a file with one command per line.  These commands are the commands that will be distributed.  In GNU/Linux or MacOS the commands will be executed within /bin/sh which is usually bash.  I have no idea about windoze.  The shell ';' syntax can be used to put multiple commands per line therefore providing a mechanism for synchronization, for example in case there are commands that need to be ran before other commands.

To get a full list of command line options execute divvy with the --help option.

MPI usage is just like any other MPI usage using mpiexec or the older mpirun.  At least two processes are required as one is the master and there must be at least one slave.


CONTACT
-------
For questions, comments, or bug reports contact Greg McGarragh at greg.mcgarragh@colostate.edu.

Bug reports are greatly appreciated!  If you would like to report a bug please include a reproducible case.
