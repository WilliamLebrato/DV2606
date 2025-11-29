/***************************************************************************
 *
 * Parallel version of Gauss-Jordan row reduction
 *
 ***************************************************************************/

#include <stdio.h>

#define MAX_SIZE 2048


int	N;		/* matrix size		*/
int	maxnum;		/* max number of element*/
const char* Init;		/* matrix init type	*/
int	PRINT;		/* print switch		*/

// Pointer-based matrix and vectors
double*    A;        /* matrix A		*/
double*    b;		/* vector b		*/
double*    y;		/* solution vector y	*/


/* forward declarations */
void work(void);
void Init_Matrix(void);
void Print_Matrix(void);
void Init_Default(void);
int Read_Options(int, char**);

int
main(int argc, char** argv)
{
    printf("Gauss Jordan\n");

    Init_Default();		/* Init default values	*/
    Read_Options(argc, argv);	/* Read arguments	*/
    Init_Matrix();		/* Init the matrix	*/
    work();
    if (PRINT == 1)
        Print_Matrix();
}

__global__ void normalize_kernel(double *A, double *b, double *y, int k, int n)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = blockDim.x * gridDim.x;

    double pivot_coeff = A[k * n + k];


    for (int j = idx + k + 1; j < n; j += stride) {
        A[k * n + j] = A[k * n + j] / pivot_coeff;
    }
    

    if (idx == 0) {
        y[k] = b[k] / pivot_coeff; 
        
        A[k * n + k] = 1.0; 
    }
}

__global__ void elimination_kernel(double *A, double *b, double *y, int k, int n)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = blockDim.x * gridDim.x;

    // eliminate rows below pivot
    for (int i = idx + k + 1; i < n; i += stride) {
        double pivot_val = A[i * n + k];

        for (int j = k + 1; j < n; j++) {
            A[i * n + j] -= pivot_val * A[k * n + j];
        }

        b[i] -= pivot_val * y[k]; 

        A[i * n + k] = 0.0;
    }

    // solve all rows above pivot
    for (int i = idx; i < k; i += stride) {
        double pivot_val = A[i * n + k];

        for (int j = k + 1; j < n; j++) {
            A[i * n + j] -= pivot_val * A[k * n + j];
        }

        y[i] -= pivot_val * y[k];

        A[i * n + k] = 0.0;
    }
}

void work(void)
{
    int  k; 

    int threads = 1024;
    int blocks = 2; 

    double* d_A;
    double* d_b;
    double* d_y;

    // Allocate memory and copy to GPU

    cudaMalloc((void**)&d_A, N * N * sizeof(double));
    cudaMalloc((void**)&d_b, N * sizeof(double));
    cudaMalloc((void**)&d_y, N * sizeof(double));

    cudaMemcpy(d_A, A, N * N * sizeof(double), cudaMemcpyHostToDevice);
    cudaMemcpy(d_b, b, N * sizeof(double), cudaMemcpyHostToDevice);
    cudaMemcpy(d_y, y, N * sizeof(double), cudaMemcpyHostToDevice);

    /* Gaussian elimination algorithm */
    for (k = 0; k < N; k++) { 

        normalize_kernel<<<blocks, threads>>>(d_A, d_b, d_y, k, N);

        elimination_kernel<<<blocks, threads>>>(d_A, d_b, d_y, k, N);
    }

    cudaMemcpy(A, d_A, N * N * sizeof(double), cudaMemcpyDeviceToHost);
    cudaMemcpy(b, d_b, N * sizeof(double), cudaMemcpyDeviceToHost);
    cudaMemcpy(y, d_y, N * sizeof(double), cudaMemcpyDeviceToHost);

    cudaFree(d_A);
    cudaFree(d_b);
    cudaFree(d_y);
}

void
Init_Matrix()
{
    int i, j;

    // Changed to pointers
    A = (double*)malloc(N * N * sizeof(double));
    b = (double*)malloc(N * sizeof(double));
    y = (double*)malloc(N * sizeof(double));

    printf("\nsize      = %dx%d ", N, N);
    printf("\nmaxnum    = %d \n", maxnum);
    printf("Init      = %s \n", Init);
    printf("Initializing matrix...");

    if (strcmp(Init, "rand") == 0) {
        for (i = 0; i < N; i++) {
            for (j = 0; j < N; j++) {
                if (i == j) /* diagonal dominance */
                    A[i * N + j] = (double)(rand() % maxnum) + 5.0; // Adjusted
                else
                    A[i * N + j] = (double)(rand() % maxnum) + 1.0; // Adjusted
            }
        }
    }
    if (strcmp(Init, "fast") == 0) {
        for (i = 0; i < N; i++) {
            for (j = 0; j < N; j++) {
                if (i == j) 
                    A[i * N + j] = 5.0; // Adjusted
                else
                    A[i * N + j] = 2.0; // Adjusted
            }
        }
    }

    /* Initialize vectors b and y */
    for (i = 0; i < N; i++) {
        b[i] = 2.0;
        y[i] = 1.0;
    }

    printf("done \n\n");
    if (PRINT == 1)
        Print_Matrix();
}

void
Print_Matrix()
{
    int i, j;

    printf("Matrix A:\n");
    for (i = 0; i < N; i++) {
        printf("[");
        for (j = 0; j < N; j++)
            printf(" %5.2f,", A[i * N + j]); 
        printf("]\n");
    }
    printf("Vector y:\n[");
    for (j = 0; j < N; j++)
        printf(" %5.2f,", y[j]);
    printf("]\n");
    printf("\n\n");
}

void
Init_Default()
{
    N = 2048;
    Init = "fast";
    maxnum = 15.0;
    PRINT = 0;
}

int
Read_Options(int argc, char** argv)
{
    char* prog;

    prog = *argv;
    while (++argv, --argc > 0)
        if (**argv == '-')
            switch (*++ * argv) {
            case 'n':
                --argc;
                N = atoi(*++argv);
                break;
            case 'h':
                printf("\nHELP: try sor -u \n\n");
                exit(0);
                break;
            case 'u':
                printf("\nUsage: gaussian [-n problemsize]\n");
                printf("           [-D] show default values \n");
                printf("           [-h] help \n");
                printf("           [-I init_type] fast/rand \n");
                printf("           [-m maxnum] max random no \n");
                printf("           [-P print_switch] 0/1 \n");
                exit(0);
                break;
            case 'D':
                printf("\nDefault:  n         = %d ", N);
                printf("\n          Init      = rand");
                printf("\n          maxnum    = 5 ");
                printf("\n          P         = 0 \n\n");
                exit(0);
                break;
            case 'I':
                --argc;
                Init = *++argv;
                break;
            case 'm':
                --argc;
                maxnum = atoi(*++argv);
                break;
            case 'P':
                --argc;
                PRINT = atoi(*++argv);
                break;
            default:
                printf("%s: ignored option: -%s\n", prog, *argv);
                printf("HELP: try %s -u \n\n", prog);
                break;
            }
    return (0);
}