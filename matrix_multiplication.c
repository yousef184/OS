#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>

// matrix data
typedef struct{
    int **m1,  **m2,  **result;
    int R1, R2,  C2;
}Matrix;

// row data
typedef struct{
    Matrix *a;
    int row;
}row;

// element data
typedef struct{
    Matrix *a;
    int row;
    int col;
}element;

// get row and column dimensions from file
void readDimensions(const char *filename, int *rows, int *cols) {
    FILE *file = fopen(filename, "r");
    fscanf(file, "row=%d col=%d", rows, cols);
    fclose(file);
}

// read matrix data from file
int** readArray(const char *filename, int rows, int cols) {
    FILE *file = fopen(filename, "r");

    //allocate memory for the matrix
    int **array = (int**)malloc(rows * sizeof(int*));
    for (int i = 0; i < rows; i++) {
        array[i] = (int*)malloc(cols * sizeof(int));
    }    
    fscanf(file, "row=%*d col=%*d"); // Skip the first line
    

    // get the matrix data
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            fscanf(file, "%d", &array[i][j]);
        }
    }
    
    fclose(file);
    return array;
}


// save matrix data to file
void saveMatrixToFile(int **array, int rows, int cols, const char *filename) {
    FILE *file = fopen(filename, "w");

    // Write the dimensions as in the input files
    fprintf(file, "row=%d col=%d\n", rows, cols);

    // Write matrix data
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            fprintf(file, "%d ", array[i][j]);
        }
        fprintf(file, "\n");
    }

    fclose(file);
}

// free the allocated memory for the matrix
void freeArray(int **array, int rows) {
    for (int i = 0; i < rows; i++) {
        free(array[i]);
    }
    free(array);
}

// matrix multiplication using 1 thread
void* multiplyMatrix(void* arg)
{
    Matrix* a = (Matrix*) arg;
 
    for (int i = 0; i < a->R1; i++) {
        for (int j = 0; j < a->C2; j++) {
            a->result[i][j] = 0;
 
            for (int k = 0; k < a->R2; k++) {
                a->result[i][j] += a->m1[i][k] * a->m2[k][j];
            }
 
        }
 
    }
    pthread_exit(0);
}

// thread per row
void* multiplyRow(void* arg)
{
    row* a = (row*) arg;
    Matrix *b = a->a;
    int i = a->row;
        for (int j = 0; j < b->C2; j++) {
            b->result[i][j] = 0;

            for (int k = 0; k < b->R2; k++) {
                b->result[i][j] += b->m1[i][k] * b->m2[k][j];
            }

        }
        free(a);
        pthread_exit(0);
}

// thread per element
void* multiplyElement(void* arg)
{
    element* a = (element*) arg;
    Matrix *b = a->a;
    int i = a->row;
    int j = a->col;  
    b->result[i][j] = 0;
    for (int k = 0; k < b->R2; k++) {
        b->result[i][j] += b->m1[i][k] * b->m2[k][j];
    } 
        free(a);
        pthread_exit(0);
}


int main(int argc, char *argv[]) {
    
    // variables for calculating time
    struct timeval stop, start;

    // filenames
    char filename1[100], filename2[100], outfilename1[100], outfilename2[100], outfilename3[100];

    // get the filenames from the command line arguments
    if(argc == 4)
    {
        snprintf(filename1, sizeof(filename1), "%s.txt", argv[1]);
        snprintf(filename2, sizeof(filename2), "%s.txt", argv[2]);
        snprintf(outfilename1, sizeof(outfilename1), "%s_per_matrix.txt", argv[3]);
        snprintf(outfilename2, sizeof(outfilename2), "%s_per_row.txt", argv[3]);
        snprintf(outfilename3, sizeof(outfilename3), "%s_per_element.txt", argv[3]);
    }
    // if no arguments are provided
    else
    {
        strcpy(filename1, "a.txt");
        strcpy(filename2, "b.txt");
        strcpy(outfilename1, "c_per_matrix.txt");
        strcpy(outfilename2, "c_per_row.txt");
        strcpy(outfilename3, "c_per_element.txt");
    }

    // variables for matrix data
    int rowsA, colsA,rowsB,colsB;
    int **arrayA, **arrayB, **arrayM1, **arrayM2, **arrayM3; 

    // get the dimensions of the matrices
    readDimensions(filename1, &rowsA, &colsA);
    readDimensions(filename2, &rowsB, &colsB);  
    // read the matrices from the files
    arrayA = readArray(filename1, rowsA, colsA);
    arrayB = readArray(filename2, rowsB, colsB);
    // threads used for each method
    pthread_t thread;
    pthread_t threads[rowsA];
    pthread_t threadsE[rowsA][colsB];

    // check if the matrices can be multiplied
    if (colsA != rowsB) {
        printf("Matrix multiplication not possible: Incompatible dimensions.\n");
        return 1;
    }

   // allocate memory for the result matrices 
   arrayM1 = (int**)malloc(rowsA * sizeof(int*));
   arrayM2 = (int**)malloc(rowsA * sizeof(int*));
   arrayM3 = (int**)malloc(rowsA * sizeof(int*));
    for (int i = 0; i < rowsA; i++) {
        arrayM1[i] = (int*)malloc(colsB * sizeof(int));
        arrayM2[i] = (int*)malloc(colsB * sizeof(int));
        arrayM3[i] = (int*)malloc(colsB * sizeof(int));
    }
   
    // create matrix objects to store data
    Matrix M1 = {arrayA, arrayB, arrayM1, rowsA, rowsB, colsB};
    Matrix M2 = {arrayA, arrayB, arrayM2, rowsA, rowsB, colsB};
    Matrix M3 = {arrayA, arrayB, arrayM3, rowsA, rowsB, colsB};
    
    // Single Threaded Matrix Multiplication
    gettimeofday(&start, NULL); //start checking time
    pthread_create(&thread, NULL, multiplyMatrix, &M1);
    pthread_join(thread, NULL);
    gettimeofday(&stop, NULL); //end checking time

    printf("method 1: %lds %luus\n", stop.tv_sec - start.tv_sec ,stop.tv_usec - start.tv_usec);


    // Row wise Parallel Matrix Multiplication
    gettimeofday(&start, NULL); //start checking time

    for(int i=0; i<rowsA; i++)
    {
        row *r = (row *)malloc(sizeof(row));  // Dynamically allocate for each thread
        r->a = &M2;
        r->row = i;
        pthread_create(&threads[i], NULL, multiplyRow, r);
    }

    for(int i=0; i<rowsA; i++)
    {
        pthread_join(threads[i], NULL);
    }

    gettimeofday(&stop, NULL); //end checking time

    printf("method 2: %lds %luus\n", stop.tv_sec - start.tv_sec ,stop.tv_usec - start.tv_usec);


    // Element wise Parallel Matrix Multiplication
    gettimeofday(&start, NULL); //start checking time
    
    for(int i=0; i<rowsA; i++)
    {
        for(int j=0; j<colsB; j++)
        {
            element *r = (element *)malloc(sizeof(element));  // Dynamically allocate for each thread
            r->a = &M3;
            r->row = i;
            r->col = j;
            pthread_create(&threadsE[i][j], NULL, multiplyElement, r);
        }
    }

    for(int i=0; i<rowsA; i++)
    {
        for(int j=0; j<colsB; j++)
        {
            pthread_join(threadsE[i][j], NULL);
        }
    }

    gettimeofday(&stop, NULL); //end checking time

    printf("method 2: %lds %luus\n", stop.tv_sec - start.tv_sec ,stop.tv_usec - start.tv_usec);

    saveMatrixToFile(arrayM1, rowsA, colsB, outfilename1);
    saveMatrixToFile(arrayM2, rowsA, colsB, outfilename2);
    saveMatrixToFile(arrayM3, rowsA, colsB, outfilename3);

    freeArray(arrayA, rowsA);
    freeArray(arrayB, rowsB);
    freeArray(arrayM1, rowsA);
    freeArray(arrayM2, rowsA);
    freeArray(arrayM3, rowsA);
    return 0;
}
