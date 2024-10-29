/*******************************************************************************
**
**    Copyright (C) 2010-2019 Greg McGarragh <greg.mcgarragh@colostate.edu>
**
**    This source code is licensed under the GNU General Public License (GPL),
**    Version 3.  See the file COPYING for more details.
**
*******************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <sys/wait.h>

#include <omp.h>

#define MPICH_SKIP_MPICXX
#include <mpi.h>

#include "grmlib.h"
#include "version.h"


const char *program_name = "divvy";
const char *PROGRAM_NAME = "divvy";


#define MAX_CMDS	4194304
#define MAX_CMD_LENGTH	16384
#define MAX_CMD_TOKENS	4096


typedef struct {
    int help;
    int exit_on_error;
    int omp;
    int mpi;
    int verbose;
    int version;
} options_data;


int divvy_mpi_slave();
int execute_cmd(char *cmd);
void usage();
void version();


int main(int argc, char *argv[])
{
    char cmd[MAX_CMD_LENGTH + 2];

    char **cmd_files;
    char **cmd_list;

    int i;
    int ii;
    int j;
    int k;
    int n;

    int mpi_size;
    int mpi_rank;
    int n_threads;
    int n_cmd_files;
    int n_cmds;
    int cmd_len;
    int n_procs;
    int cur_rank;

    FILE *fp;

    clock_t start_clock;
    time_t  start_time;

    MPI_Status mpi_status;

    options_data options;


    /*--------------------------------------------------------------------------
     *
     *------------------------------------------------------------------------*/
    start_clock = clock();
    start_time  = time(NULL);


    /*--------------------------------------------------------------------------
     *
     *------------------------------------------------------------------------*/
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    if (mpi_size > 1 && mpi_rank != 0) {
        if (divvy_mpi_slave()) {
            fprintf(stderr, "ERROR: divvy_mpi_slave()\n");
            exit(1);
        }

        MPI_Finalize();

        exit(0);
    }


    /*--------------------------------------------------------------------------
     *
     *------------------------------------------------------------------------*/
    options.help          = 0;
    options.exit_on_error = 1;
    options.omp           = 0;
    options.mpi           = 1;
    options.verbose       = 1;
    options.version       = 0;

    cmd_files = (char **) malloc(argc * sizeof(char *));

    n = 0;
    for (i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            if (strcmp(argv[i], "--help") == 0) {
                 usage();
                 exit(0);
            }
            else if (strcmp(argv[i], "--exit_on_error") == 0)
                 options.exit_on_error = 1;
            else if (strcmp(argv[i], "--no-exit_on_error") == 0)
                 options.exit_on_error = 0;
            else if (strcmp(argv[i], "--omp") == 0) {
                 options.omp = 1;
                 options.mpi = 0;
            }
            else if (strcmp(argv[i], "--mpi") == 0) {
                 options.omp = 0;
                 options.mpi = 1;
            }
            else if (strcmp(argv[i], "--n_threads") == 0) {
                 ii = i;
                 check_arg_count(i, argc, 1, argv[ii]);
                 n_threads = strtoi_errmsg_exit(argv[++i], argv[ii]);
                 omp_set_num_threads(n_threads);
            }
            else if (strcmp(argv[i], "--verbose") == 0)
                 options.verbose = 1;
            else if (strcmp(argv[i], "--no-verbose") == 0)
                 options.verbose = 0;
            else if (strcmp(argv[i], "--version") == 0) {
                 version();
                 exit(0);
            }
            else {
                 printf("Invalid option: %s, use --help for more information\n",
                        argv[i]);
                 exit(1);
            }
        }
        else {
            cmd_files[n] = argv[i];
            ++n;
        }
    }

    if (n < 0) {
        printf("Not enough arguments, use --help for more information\n");
        exit(1);
    }

    n_cmd_files = n;


    /*--------------------------------------------------------------------------
     *
     *------------------------------------------------------------------------*/
    if (options.mpi && mpi_size == 1) {
        fprintf(stderr, "ERROR: Must use more than one process for option --mpi\n");
        MPI_Finalize();
        exit(0);
    }


    /*--------------------------------------------------------------------------
     *
     *------------------------------------------------------------------------*/
    n_cmds = 0;

    cmd_list = (char **) malloc(MAX_CMDS * sizeof(char *));

    for (i = 0; i < n_cmd_files; ++i) {
        if ((fp = fopen(cmd_files[i], "r")) == NULL) {
            fprintf(stderr, "ERROR: Problem opening file for reading: %s... %s\n",
                    cmd_files[i], strerror(errno));
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        for (j = 0; fgets(cmd, MAX_CMD_LENGTH + 2, fp); ++j) {
            if (j >= MAX_CMDS) {
                fprintf(stderr, "ERROR: too many commands in command file, increase MAX_CMDS\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            for (k = 0; cmd[k] != '#' && cmd[k] != '\n' && cmd[k] != '\0'; ++k) ;

            if (k >= MAX_CMD_LENGTH + 1) {
                fprintf(stderr, "ERROR: command length to large: file %s, line %d, increase MAX_CMD_LENGTH\n",
                        cmd_files[i], j + 1);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            cmd[k] = '\0';

            remove_pad(cmd, 0);

            cmd_list[n_cmds++] = strdup(cmd);
        }

        fclose(fp);
    }

    free(cmd_files);


    /*--------------------------------------------------------------------------
     *
     *------------------------------------------------------------------------*/
    if (options.omp) {
#pragma omp parallel
{
#pragma omp for schedule(dynamic, 1)
        for (i = 0; i < n_cmds; ++i) {
            if (options.verbose)
                printf("n_cmds = %d, i_cmd = %d\n", n_cmds, i);

            if (execute_cmd(cmd_list[i])) {
                fprintf(stderr, "ERROR: execute_cmd(), i_cmd = %d, cmd = %s\n",
                        i, cmd_list[i]);
                if (options.exit_on_error)
                    exit(1);
            }
        }
    }
}

    /*--------------------------------------------------------------------------
     *
     *------------------------------------------------------------------------*/
    else {
        if (mpi_rank == 0) {
            if (n_cmds + 1 < mpi_size) {
                printf("WARNING: more processes (%d) started than required (%d)\n",
                       mpi_size, n_cmds + 1);
/*
                fprintf(stderr, "ERROR: too many processes started, use a maximum of %d\n",
                        n_cmds + 1);
                MPI_Abort(MPI_COMM_WORLD, 1);
*/
            }

            for (i = 0; i < n_cmds; ++i) {
                if (i == mpi_size - 1)
                    break;

                    if (options.verbose)
                        printf("Init loop: n_cmds = %d, i_cmd = %d, cur_rank = %d\n",
                               n_cmds, i, i + 1);

                    cmd_len = strlen(cmd_list[i]) + 1;

                    MPI_Send(&i, 1, MPI_INT, i + 1, 0, MPI_COMM_WORLD);
                    MPI_Send(&cmd_len, 1, MPI_INT, i + 1, 1, MPI_COMM_WORLD);
                    MPI_Send(cmd_list[i], cmd_len, MPI_CHAR, i + 1, 2, MPI_COMM_WORLD);
                    MPI_Send(&options.exit_on_error, 1, MPI_INT, i + 1, 3, MPI_COMM_WORLD);
            }

            n_procs = i;

            for (     ; i < n_cmds; ++i) {
                MPI_Recv(&cur_rank, 1, MPI_INT, MPI_ANY_SOURCE, 4, MPI_COMM_WORLD, &mpi_status);

                if (options.verbose)
                    printf("Main loop: n_cmds = %d, i_cmd = %d, cur_rank = %d\n",
                           n_cmds, i, cur_rank);

                cmd_len = strlen(cmd_list[i]) + 1;

                MPI_Send(&i, 1, MPI_INT, cur_rank, 0, MPI_COMM_WORLD);
                MPI_Send(&cmd_len, 1, MPI_INT, cur_rank, 1, MPI_COMM_WORLD);
                MPI_Send(cmd_list[i], cmd_len, MPI_CHAR, cur_rank, 2, MPI_COMM_WORLD);
                MPI_Send(&options.exit_on_error, 1, MPI_INT, cur_rank, 3, MPI_COMM_WORLD);
            }

            ii = -1;
            for (i = 0; i < n_procs; ++i) {
                MPI_Recv(&cur_rank, 1, MPI_INT, MPI_ANY_SOURCE, 4, MPI_COMM_WORLD, &mpi_status);

                if (options.verbose)
                    printf("cleanup loop: cur_rank = %d\n", cur_rank);

                MPI_Send(&ii, 1, MPI_INT, cur_rank, 0, MPI_COMM_WORLD);
            }

            for (     ; i < mpi_size - 1; ++i)
                MPI_Send(&ii, 1, MPI_INT, i + 1, 0, MPI_COMM_WORLD);
        }
    }


    /*--------------------------------------------------------------------------
     *
     *------------------------------------------------------------------------*/
    MPI_Finalize();

    for (i = 0; i < n_cmds; ++i)
        free(cmd_list[i]);
    free(cmd_list);


    printf("Wall clock time: %.2f sec\n", difftime(time(NULL), start_time));


    exit(0);
}



/*******************************************************************************
 *
 ******************************************************************************/
int divvy_mpi_slave()
{
    char cmd[MAX_CMD_LENGTH + 2];

    int i;
    int mpi_rank;
    int cmd_len;
    int exit_on_error;

    MPI_Status mpi_status;

    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    while(1) {
        MPI_Recv(&i, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &mpi_status);

        if (i < 0)
            break;

        MPI_Recv(&cmd_len, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &mpi_status);
        MPI_Recv(cmd, cmd_len, MPI_CHAR, 0, 2, MPI_COMM_WORLD, &mpi_status);
        MPI_Recv(&exit_on_error, 1, MPI_INT, 0, 3, MPI_COMM_WORLD, &mpi_status);

        if (execute_cmd(cmd)) {
            fprintf(stderr, "ERROR: execute_cmd(), i_cmd = %d\n", i);
            if (exit_on_error)
                MPI_Abort(MPI_COMM_WORLD, 1);
        }

        MPI_Send(&mpi_rank, 1, MPI_INT, 0, 4, MPI_COMM_WORLD);
    }

    return 0;
}



/*******************************************************************************
 *
 ******************************************************************************/
static int parse_cmd(char *cmd, char *argv[], int max)
{
    int i;

    char *token;

    token = strtok(cmd, " ");

    i = 0;
    do {
        if (i >= max) {
            fprintf(stderr, "ERROR: too many command tokens, increase MAX_HOSTS\n");
            return -1;
        }
        argv[i++] = token;
    } while ((token = strtok(NULL, " ")));

    argv[i++] = NULL;

    return i;
}



/*******************************************************************************
 *
 ******************************************************************************/
int execute_cmd(char *cmd)
{
    int status;

    const char *argv2[MAX_CMD_TOKENS + 1];

    if (vfork() == 0) {
        argv2[0] = "/bin/bash";
        argv2[1] = "-c";
        argv2[2] = cmd;
        argv2[3] = NULL;
        execv(argv2[0], (char * const *) argv2);
        fprintf(stderr, "ERROR: execv()\n");
        return -1;
    }

    if (wait(&status) < 0) {
        fprintf(stderr, "ERROR: wait()\n");
        return -1;
    }

    if (status) {
        fprintf(stderr, "ERROR: status = %d\n", status);
        return -1;
    }

    return 0;
}



void usage()
{
    version();
    printf("\n");
    printf("Usage: divvy [OPTIONS] [FILE 1 | FILE 2 | FILE 3 | ...]\n");
    printf("\n");
    printf("FILE is a file with a list of commands. One command per line.\n");
    printf("\n");
    printf("Options:\n");
    printf("    --help:             Print this help content.\n");
    printf("    --exit_on_error:    If a command fails terminate with a status of 1. (default)\n");
    printf("    --no-exit_on_error: If a command fails do not terminate with a status of 1.\n");
    printf("    --omp:              Use OpenMP for paralleization.\n");
    printf("    --mpi:              Use MPI for paralleization (default). Requires execuation using mpiexec.\n");
    printf("    --n_threads:        The number of threads to use when using OpenMP\n");
    printf("    --verbose:          Print verbose output. (default)\n");
    printf("    --no-verbose:       Do not print verbose output.\n");
    printf("    --version:          Print source Git hash and build date information.\n");
    printf("\n");
}



void version()
{
    printf("%s, %s, %s\n", PROGRAM_NAME, build_sha_1, build_date);
}
