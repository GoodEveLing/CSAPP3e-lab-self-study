/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include "cachelab.h"
#include <stdio.h>

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

void trans_32x32(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, ii, jj;

    for (i = 0; i < N; i += 8) {
        for (j = 0; j < M; j += 8) {
            for (ii = 0; ii < 8; ii++) {
                for (jj = 0; jj < 8; jj++) {
                    int row     = ii + i;
                    int col     = jj + j;
                    B[col][row] = A[row][col];
                }
            }
        }
    }
}

void trans_32x32_v2(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k;

    for (i = 0; i < N; i += 8) {
        for (j = 0; j < M; j += 8) {
            for (k = 0; k < 8; k++) {
                int row = i + k;

                int a_0 = A[row][j + 0];
                int a_1 = A[row][j + 1];
                int a_2 = A[row][j + 2];
                int a_3 = A[row][j + 3];
                int a_4 = A[row][j + 4];
                int a_5 = A[row][j + 5];
                int a_6 = A[row][j + 6];
                int a_7 = A[row][j + 7];

                B[j + 0][row] = a_0;
                B[j + 1][row] = a_1;
                B[j + 2][row] = a_2;
                B[j + 3][row] = a_3;
                B[j + 4][row] = a_4;
                B[j + 5][row] = a_5;
                B[j + 6][row] = a_6;
                B[j + 7][row] = a_7;
            }
        }
    }
}

void trans_64x64_v1(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k;
    for (i = 0; i < 64; i += 8) {
        for (j = 0; j < 64; j += 8) {
            for (k = 0; k < 4; k++) {
                int row = i + k;

                int a_0 = A[row][j + 0];
                int a_1 = A[row][j + 1];
                int a_2 = A[row][j + 2];
                int a_3 = A[row][j + 3];
                int a_4 = A[row][j + 4];
                int a_5 = A[row][j + 5];
                int a_6 = A[row][j + 6];
                int a_7 = A[row][j + 7];

                // B 1 = A1_t
                B[j + 0][row] = a_0;
                B[j + 1][row] = a_1;
                B[j + 2][row] = a_2;
                B[j + 3][row] = a_3;
                // B2 = A2_t
                B[j + 0][row + 4] = a_4;
                B[j + 1][row + 4] = a_5;
                B[j + 2][row + 4] = a_6;
                B[j + 3][row + 4] = a_7;
            }

            for (k = 0; k < 4; k++) {
                int row = i + k;

                int a_0 = A[row + 4][j + 0];
                int a_1 = A[row + 4][j + 1];
                int a_2 = A[row + 4][j + 2];
                int a_3 = A[row + 4][j + 3];
                int a_4 = A[row + 4][j + 4];
                int a_5 = A[row + 4][j + 5];
                int a_6 = A[row + 4][j + 6];
                int a_7 = A[row + 4][j + 7];

                // B3 = A3_t
                B[j + 4][row] = a_0;
                B[j + 5][row] = a_1;
                B[j + 6][row] = a_2;
                B[j + 7][row] = a_3;
                // B4 = A4_t
                B[j + 4][row + 4] = a_4;
                B[j + 5][row + 4] = a_5;
                B[j + 6][row + 4] = a_6;
                B[j + 7][row + 4] = a_7;
            }

            for (k = 0; k < 4; k++) {
                int row = j + k;

                // B2 = A2_t
                int a_0 = B[row][i + 4];
                int a_1 = B[row][i + 5];
                int a_2 = B[row][i + 6];
                int a_3 = B[row][i + 7];

                // B3 = A3_t
                int a_4 = B[row + 4][i + 0];
                int a_5 = B[row + 4][i + 1];
                int a_6 = B[row + 4][i + 2];
                int a_7 = B[row + 4][i + 3];

                // B2 = A3_t
                B[row][i + 4] = a_4;
                B[row][i + 5] = a_5;
                B[row][i + 6] = a_6;
                B[row][i + 7] = a_7;

                // B2 = A2_t
                B[row + 4][i + 0] = a_0;
                B[row + 4][i + 1] = a_1;
                B[row + 4][i + 2] = a_2;
                B[row + 4][i + 3] = a_3;
            }
        }
    }
}

void trans_64x64_v2(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k;
    for (i = 0; i < 64; i += 8) {
        for (j = 0; j < 64; j += 8) {
            for (k = 0; k < 4; k++) {
                int row = i + k;

                int a_0 = A[row][j + 0];
                int a_1 = A[row][j + 1];
                int a_2 = A[row][j + 2];
                int a_3 = A[row][j + 3];
                int a_4 = A[row][j + 4];
                int a_5 = A[row][j + 5];
                int a_6 = A[row][j + 6];
                int a_7 = A[row][j + 7];

                // B1 = A1_t
                B[j + 0][row] = a_0;
                B[j + 1][row] = a_1;
                B[j + 2][row] = a_2;
                B[j + 3][row] = a_3;

                // B2 = A2_t
                B[j + 0][row + 4] = a_4;
                B[j + 1][row + 4] = a_5;
                B[j + 2][row + 4] = a_6;
                B[j + 3][row + 4] = a_7;
            }

            for (k = 0; k < 4; k++) {
                int row = j + k;

                // B2 = A2_t
                int a_0 = B[row][i + 4];
                int a_1 = B[row][i + 5];
                int a_2 = B[row][i + 6];
                int a_3 = B[row][i + 7];

                // get A3_t
                int a_4 = A[i + 4][row];
                int a_5 = A[i + 5][row];
                int a_6 = A[i + 6][row];
                int a_7 = A[i + 7][row];

                // B2 = A3_t
                B[row][i + 4] = a_4;
                B[row][i + 5] = a_5;
                B[row][i + 6] = a_6;
                B[row][i + 7] = a_7;

                // B3 = A2_t
                B[row + 4][i + 0] = a_0;
                B[row + 4][i + 1] = a_1;
                B[row + 4][i + 2] = a_2;
                B[row + 4][i + 3] = a_3;
            }

            for (k = 0; k < 4; k++) {
                int row = i + 4 + k;
                int a_4 = A[row][j + 4];
                int a_5 = A[row][j + 5];
                int a_6 = A[row][j + 6];
                int a_7 = A[row][j + 7];

                B[j + 4][row] = a_4;
                B[j + 5][row] = a_5;
                B[j + 6][row] = a_6;
                B[j + 7][row] = a_7;
            }
        }
    }
}

void trans_61x67(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k, n;
    int block = 20;   // 17,18,19 20 are OK
    for (i = 0; i < 61; i += block) {
        for (j = 0; j < 67; j += block) {
            for (k = i; k < i + block && k < 61; k++) {
                for (n = j; n < j + block && n < 67; n++) {
                    B[n][k] = A[k][n];
                }
            }
        }
    }
}

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (M == 32 && N == 32) {
        trans_32x32_v2(M, N, A, B);
    }
    else if (M == 64 && N == 64) {
        trans_64x64_v2(M, N, A, B);
    }
    else if (M == 67 && N == 61) {
        trans_61x67(M, N, A, B);
    }
}



/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp     = A[i][j];
            B[j][i] = tmp;
        }
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}
