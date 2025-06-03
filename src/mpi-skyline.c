/****************************************************************************
 *
 * mpi-skyline.c - mpi implementaiton of the skyline operator
 *
 * Copyright (C) 2025 Lorenzo Cinelli, mat: 0001070967
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * --------------------------------------------------------------------------
 *
 * Questo programma calcola lo skyline di un insieme di punti in D
 * dimensioni letti da standard input. Per una descrizione completa
 * si veda la specifica del progetto sulla piattaforma "Virtuale".
 *
 * Per compilare:
 *
 *      mpicc -std=c99 -Wall -Wpedantic -O2 mpi-skyline.c -o mpi-skyline
 *
 * Per eseguire il programma:
 *
 *      mpirun [-n NPROC] --stdin 0 ./mpi-skyline < input > output
 *
 ****************************************************************************/

#if _XOPEN_SOURCE < 600
#define _XOPEN_SOURCE 600
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>

#include "hpc.h"

typedef struct {
    float *P;   /* coordinates P[i][j] of point i               */
    int N;      /* Number of points (rows of matrix P)          */
    int D;      /* Number of dimensions (columns of matrix P)   */
} points_t;

/**
 * Read input from stdin. Input format is:
 *
 * d [other ignored stuff]
 * N
 * p0,0 p0,1 ... p0,d-1
 * p1,0 p1,1 ... p1,d-1
 * ...
 * pn-1,0 pn-1,1 ... pn-1,d-1
 *
 */
void read_input( points_t *points ) {
    char buf[1024];
    int N, D;
    float *P;

    if (1 != scanf("%d", &D)) {
        fprintf(stderr, "FATAL: can not read the dimension\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    assert(D >= 2);
    if (NULL == fgets(buf, sizeof(buf), stdin)) { /* ignore rest of the line */
        fprintf(stderr, "FATAL: can not read the first line\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    if (1 != scanf("%d", &N)) {
        fprintf(stderr, "FATAL: can not read the number of points\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    P = (float*)malloc( D * N * sizeof(*P) );
    assert(P);
    for (int i=0; i<N; i++) {
        for (int k=0; k<D; k++) {
            if (1 != scanf("%f", &(P[i*D + k]))) {
                fprintf(stderr, "FATAL: failed to get coordinate %d of point %d\n", k, i);
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
            }
        }
    }
    points->P = P;
    points->N = N;
    points->D = D;
}

void free_points( points_t* points ) {
    free(points->P);
    points->P = NULL;
    points->N = points->D = -1;
}

/* Returns 1 iff |p| dominates |q| */
int dominates( const float * p, const float * q, int D ) {
    /* The following loops could be merged, but the keep them separated
       for the sake of readability */
    for (int k=0; k<D; k++) {
        if (p[k] < q[k]) {
            return 0;
        }
    }
    for (int k=0; k<D; k++) {
        if (p[k] > q[k]) {
            return 1;
        }
    }
    return 0;
}

/**
 * Compute the skyline of `points`. At the end, `s[i] == 1` iff point
 * `i` belongs to the skyline. The function returns the number `result` of
 * points that belongs to the skyline. The caller is responsible for
 * allocating the array `s` of length at least `points->N`.
 */
int skyline( const points_t *points, int *s ) {
    int data_size[] = {points->D, points->N};
    float *P = points->P;
    int *my_s;
    int r;
    int result;
    int rank;
    int comm_size;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

    MPI_Bcast(
	data_size,	//Buffer
	2,		//Count
	MPI_INT,	//Datatype
	0,		//Source
	MPI_COMM_WORLD	//Communicator
    );

    const int D = data_size[0];
    const int N = data_size[1];
    const int MY_N = (N + comm_size - 1) / comm_size;
    r=0;
    my_s = (int*)malloc(MY_N * sizeof(*my_s)); assert(my_s);

    if(rank != 0) {
        P = (float*)malloc( D * N * sizeof(*P) ); assert(P);
    }

    MPI_Bcast(
	P,		//Buffer
	N*D,		//Count
	MPI_FLOAT,	//Datatype
	0,		//Source
	MPI_COMM_WORLD	//Communicator
    );

    for (int i=0; i<MY_N; i++) {
        my_s[i] = (rank*MY_N+i < N) ? 1 : 0;
    }

    for (int i=0; i<N; i++) {
        if( i<rank*MY_N || i>=(rank+1)*MY_N || (i>=rank*MY_N && i<(rank+1)*MY_N && my_s[i-rank*MY_N])) {
            for (int j=0; j<MY_N && rank*MY_N+j<N; j++) {
                if ( my_s[j] && dominates( &(P[i*D]), &(P[(rank*MY_N+j)*D]), D ) ) {
                    my_s[j] = 0;
                    r--;
                }
            }
        }
    }

    MPI_Gather(
	my_s,		//Send buffer
	MY_N,		//Send count
	MPI_INT,	//Send datatype
	s,		//Receive buffer
	MY_N,		//Receive count
	MPI_INT,	//Receive datatype
	0,		//Destination
	MPI_COMM_WORLD	//Communicator
    );

    MPI_Reduce(
	&r,		//Send buffer
	&result,	//Receive buffer
	1,		//Count
	MPI_INT,	//Datatype
	MPI_SUM,	//Operation
	0,		//Destination
	MPI_COMM_WORLD	//Communicator
    );

    if(rank != 0) {
        free(P);
    }
    free(my_s);

    return result + N;
}

/**
 * Print the coordinates of points belonging to the skyline `s` to
 * standard ouptut. `s[i] == 1` iff point `i` belongs to the skyline.
 * The output format is the same as the input format, so that this
 * program can process its own output.
 */
void print_skyline( const points_t* points, const int *s, int r ) {
    const int D = points->D;
    const int N = points->N;
    const float *P = points->P;

    printf("%d\n", D);
    printf("%d\n", r);
    for (int i=0; i<N; i++) {
        if ( s[i] ) {
            for (int k=0; k<D; k++) {
                printf("%f ", P[i*D + k]);
            }
            printf("\n");
        }
    }
}

int main( int argc, char* argv[] ) {
    points_t points;
    int *s = NULL;

    MPI_Init(&argc, &argv);
    if (argc != 1) {
        fprintf(stderr, "Usage: %s < input_file > output_file\n", argv[0]);
	MPI_Finalize();
        return EXIT_FAILURE;
    }

    int rank;
    int comm_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

    if(rank == 0) {
        read_input(&points);
        s = (int*)malloc((points.N + comm_size - 1) * sizeof(*s));
        assert(s);
    }

    const double tstart = hpc_gettime();
    const int r = skyline(&points, s);
    const double elapsed = hpc_gettime() - tstart;

    if (rank == 0) {
        print_skyline(&points, s, r);

        fprintf(stderr, "\n\t%d points\n", points.N);
        fprintf(stderr, "\t%d dimensions\n", points.D);
        fprintf(stderr, "\t%d points in skyline\n\n", r);
        fprintf(stderr, "Execution time (s) %f\n", elapsed);

        free_points(&points);
	free(s);
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
}
