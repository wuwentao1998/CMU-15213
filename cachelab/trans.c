/* 
 * trans.c - matrix transpose b = a^t
 *
 * each transpose function must have a prototype of the form:
 * void trans(int m, int n, int a[n][m], int b[m][n]);
 *
 * a transpose function is evaluated by counting the number of misses
 * on a 1kb direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int m, int n, int a[n][m], int b[m][n]);

/* 
 * transpose_submit - this is the solution transpose function that you
 *     will be graded on for part b of the assignment. do not change
 *     the description string "transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "transpose submission";
void transpose_submit(int m, int n, int a[n][m], int b[m][n])
{
	int i, j, k, t1, t2, t3, t4, t5, t6, t7, t8;

	if (m == n && m == 32)
		for ( i = 0; i < n; i += 8)
			for ( j = 0; j < n; j += 8)
				for ( k = i; k < i + 8; k++)
				{
					 t1 = a[k][j];
					 t2 = a[k][j + 1];
					 t3 = a[k][j + 2];
					 t4 = a[k][j + 3];
					 t5 = a[k][j + 4];
					 t6 = a[k][j + 5];
					 t7 = a[k][j + 6];
					 t8 = a[k][j + 7];

					b[j][k] = t1;
					b[j + 1][k] = t2;
					b[j + 2][k] = t3;
					b[j + 3][k] = t4;
					b[j + 4][k] = t5;
					b[j + 5][k] = t6;
					b[j + 6][k] = t7;
					b[j + 7][k] = t8;
				}
								
	if (m == 61 && n == 67)
		for (i = 0; i < n; i += 16)
			for (j = 0; j < m; j += 16)
				for (k = i; k < i + 16 && k < n; k++)
					for (int h = j; h < j + 16 && h < m; h++)
						b[h][k] = a[k][h];

	if (m == n && m == 64)
		for (i = 0; i < n; i += 4)
			for (j = 0; j < n; j += 4)
				for (k = i; k < i + 4; k++)
				{
					t1 = a[k][j];
					t2 = a[k][j + 1];
					t3 = a[k][j + 2];
					t4 = a[k][j + 3];
					
					b[j][k] = t1;
					b[j + 1][k] = t2;
					b[j + 2][k] = t3;
					b[j + 3][k] = t4;
				}

}	

/* 
 * you can define additional transpose functions below. we've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - a simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "simple row-wise scan transpose";
void trans(int m, int n, int a[n][m], int b[m][n])
{
    int i, j, tmp;

    for (i = 0; i < n; i++) {
        for (j = 0; j < m; j++) {
            tmp = a[i][j];
            b[j][i] = tmp;
        }
    }    

}

/*
 * registerfunctions - this function registers your transpose
 *     functions with the driver.  at runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. this is a handy way to experiment with different
 *     transpose strategies.
 */
void registerfunctions()
{
    /* register your solution function */
    registertransfunction(transpose_submit, transpose_submit_desc); 

    /* register any additional transpose functions */
    registertransfunction(trans, trans_desc); 

}

/* 
 * is_transpose - this helper function checks if b is the transpose of
 *     a. you can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int m, int n, int a[n][m], int b[m][n])
{
    int i, j;

    for (i = 0; i < n; i++) {
        for (j = 0; j < m; ++j) {
            if (a[i][j] != b[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

