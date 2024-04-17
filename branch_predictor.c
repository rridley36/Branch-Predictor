#include <stdio.h>                                      // Used for printing and scanning (printf/scanf)
#include <stdlib.h>                                     // Used for dynamic memory allocation (malloc)
#include <math.h>                                       // Used for the power function (2^M)

// Global variables
int reads = 0;                                          // Stores total reads
int misses = 0;                                         // Stores total misses
int GBH = 0;                                            // Global Branch History Register

int verifyMiss(char actualPrediction, int tablePrediction){
    if((actualPrediction == 't' && (tablePrediction == 2 || tablePrediction == 3)) || (actualPrediction == 'n' && (tablePrediction == 0 || tablePrediction == 1))){      // A value of 2 or 3 counts as taken and value of 0 or 1 counts as not taken 
        return 1;                                       // It's a hit
    }
    else {
        return 2;                                       // It's a miss
    }
}

int *allocateTable(int size){
    int *ptr = malloc(size * sizeof(int));              // Allocate space
    for(int i = 0; i < size; i++){
        ptr[i] = 2;                                     // Initializes to weakly taken
    }  
    return ptr;                                         // Returns the address of the array 
}

void updateGsharetable(int * arrayTable, char actualPrediction, int i){
    if(actualPrediction == 't'){
        arrayTable[i]++;                                // If it's taken, then increment by 1
        if(arrayTable[i] > 3){                          // Makes sure it doesn't surpass 3
            arrayTable[i] = 3;
        }
    }
    else if (actualPrediction == 'n'){
         arrayTable[i]--;                               // If it's not taken, decrement by 1
         if(arrayTable[i] < 0){                         // Makes sure it doesn't go below
            arrayTable[i] = 0;
         }
    } 
}

void bimodalAlgorithm(int *array, unsigned long long address, char actualPrediction, int M) {
    unsigned long long i = (((1 << M)-1) & (address));  // M least significant bits from address
    if (verifyMiss(actualPrediction, array[i]) == 2) {  // Verify it's a miss
        misses++;
    }
    updateGsharetable(array, actualPrediction, i); // Update table
}

void predictionAlgorithm(int * array, unsigned long long address, char actualPrediction, int M, int N){
    unsigned long long programCounter;                  // Stores program counter
    unsigned long long excess;                          // Stores excess bits 
    programCounter = (((1 << M) - 1) & (address));      // Computes the program counter 
    excess = (((1 << (M-N))-1) & (programCounter));     // Obtains excess bits from shifting
    programCounter >>= (M - N);                         // Shift PC by m-n bits
    unsigned long long i = programCounter ^ GBH;        // XOR PC and GBH to obtain the index
    i = (i << (M - N)) | excess;                        // Putting excess bits back by ORing
   
    if(verifyMiss(actualPrediction, array[i]) == 2){    // If the prediction is not correct, increment misses by 1
        misses++;
    }
    if(actualPrediction == 't')                         // Prediction is taken
    {
        GBH >>= 1;                                      // Shift a one when it's taken
        GBH |= (1 << (N-1));   
    }
    else if (actualPrediction == 'n'){
        GBH >>= 1;                                      // Not taken, shift to the right by one
    }
    updateGsharetable(array, actualPrediction, i);      // Update the Gshare table 
}

int main(int argc, char * argv[]){
    int GPB = atoi(argv[1]);                            // Reads in the number of PC bits used to index the Gshare table
    int RB = atoi(argv[2]);                             // Reads the gloabl history register used to index the Ghare table
    FILE *inputFile = fopen(argv[3], "r");              // Opens the input file, such as gobmk_trace.txt or mcf_trace.txt

    int sizeOfTable = pow(2, GPB);                      // Size is denoted by 2^(GPB) or 2^m
    int *GhareTable = allocateTable(sizeOfTable);       // Generates Ghsare table
    unsigned long long address;                         // Stores address
    char actualPredicition;                             // Stores either "t" for taken or "n" for not taken

    if (RB > GPB){                                      // Verify if N is greather than M, if not 
        fprintf(stderr, "N cannot be greatger than M. Please enter an N value less than M.");    // Prints error message
        fclose(inputFile);                              // Close file to avoid any fututure issues
        free(GhareTable);                               // Frees the memory used to avoid any possible memory leaks
        exit(0);                                        // Exits program
    }

    if(inputFile == NULL){                              // If the file failed to be read, it will result in an error
        fprintf(stderr, "Error: Unable to open trace file.\n");       // Prints error message
        fclose(inputFile);                              // Close file to avoid any fututure issues
        free(GhareTable);                               // Frees the memory used to avoid any possible memory leaks
        exit(0);                                        // Exits program
    }

    while (fscanf(inputFile, "%llx %c", &address, &actualPredicition) != EOF) {     // Scan the file until the end of file, which reads the address and actual prediction
        address >>= 2;                                                              // Remove address lower 2 bits
        if (RB >= 1){
            predictionAlgorithm(GhareTable, address, actualPredicition, GPB, RB);   // Perform the Gshare Prediction
        }
        else if (RB >= 0){
            bimodalAlgorithm(GhareTable, address, actualPredicition, GPB);          // Peform the bimodal algorithm
        }
        else {
            fclose(inputFile);                          // Close file to avoid any fututure issues
            free(GhareTable);                           // Frees the memory used to avoid any possible memory leaks
            exit(0);                                    // Exit the program because of an error, a negative was entered
        }
        reads++;                                        // Increment reads since it's the total amount of addresses from the trace file
    }

    fclose(inputFile);                                  // Close file to avoid any fututure issues
    free(GhareTable);                                   // Frees the memory used to avoid any possible memory leaks

    // Statistics
    printf("M: %d, N: %d\n", GPB, RB);                              // Prints values of M and N
    double mispredictionRate = ((double)misses / reads) * 100.0;    // Calculates misprediction rate
    printf("Misprediction Rate = %.2f%%\n", mispredictionRate);     // Prints misprediction
}


