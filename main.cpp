//
//  main.cpp
//  arbitrage
//
//  Created by Michael Huang on 10/5/15.
//  Copyright (c) 2015 Michael Huang. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>

char does_it_exist(char *filename);

int main(int argc, char* argv[])
{
    //Initialize variables, pointers, etc.
    int retcode = 0;
    FILE *in = NULL, *out = NULL;
    char mybuffer[100];
    int numsec, numscen, j, k, l, numnonz;
    double r;
    double *p, optimalvalue, xvalue;
    double *rates;
    FILE *results = NULL;
    
    //Check all arguments are present
    if(argc != 3){
        printf("Usage:  arb1.exe datafilename lpfilename\n"); retcode = 100; goto BACK;
    }
    
    //Open data file
    in = fopen(argv[1], "r");
    if (in == NULL){
        printf("could not open %s for reading\n", argv[1]);
        retcode = 200; goto BACK;
    }
    
    fscanf(in, "%s", mybuffer);
    fscanf(in, "%s", mybuffer);
    numsec = atoi(mybuffer);
    fscanf(in, "%s", mybuffer);
    fscanf(in, "%s", mybuffer);
    numscen = atoi(mybuffer);
    
    //Create array for rates
    //I was lazy so I made the first index 0: [0, scen1 rate, scen2 rate, etc.]
    fscanf(in, "%s", mybuffer); //Reads "r"
    rates = (double *)calloc((numscen+1), sizeof(double));
    rates[0] = 0;
    for (l = 1; l <= numscen; l++){
        fscanf(in, "%s", mybuffer);
        rates[l] = atof(mybuffer);
    }

//    fscanf(in, "%s", mybuffer);
//    r = atof(mybuffer);
    
    printf("securities: %d, scenarios: %d;;\n",
           numsec, numscen);
    
    p = (double *)calloc((1 + numscen)*(1 + numsec), sizeof(double));
    if (p == NULL){
        printf("no memory\n"); retcode = 400; goto BACK;
    }
    
    //Reading in the matrix of rates and prices for each scenario
    //first column is rates
    //subsequent columns are prices [1 + r_1, p1_1, p2_1, p3_1, etc.]
    for (k = 0; k <= numscen; k++){
        fscanf(in, "%s", mybuffer);
        p[k*(1 + numsec)] = 1 + rates[k]*(k != 0);
        for (j = 1; j <= numsec; j++){
            fscanf(in, "%s", mybuffer);
            p[k*(1 + numsec) + j] = atof(mybuffer);
        }
    }
    
    //Reads END and we close the file
    fscanf(in, "%s", mybuffer);
    fclose(in);
    
    //We open the lp file we want to write
    out = fopen(argv[2], "w");
    if (out == NULL){
        printf("can't open %s\n", argv[2]); retcode = 500; goto BACK;
    }
    printf("printing LP to file %s\n", argv[2]);
    
    //Object function
    fprintf(out, "Maximize D_min");
    
    
    fprintf(out, "\n");
    
    //NOTE: j is column index, k is row index
    
    //Constraints
    fprintf(out, "Subject to\n");
    
    //scen0: 1 x0 + p1_0 x1 + ... - V_0 = 0
    fprintf(out, "scen%d: ", 0);
    for (j = 0; j <= numsec; j++){
        if (p[j] > 0) fprintf(out, "+ "); fprintf(out, "%g x%d ", p[j], j);
    }
    fprintf(out, "- 1 V_0 = 0\n");
    
    //scenk:
    for (k = 1; k <= numscen; k++){
        fprintf(out, "scen%d: ", k);
        fprintf(out, "- %g V_0 ", p[k*(1+numsec)]);
        for (j = 0; j <= numsec; j++){
            if (p[k*(1 + numsec) + j] > 0) fprintf(out, "+ ");
            fprintf(out, "%g x%d ", p[k*(1 + numsec) + j], j);
        }
        fprintf(out, "- D_%d = 0\n", k);
        fprintf(out, "delta%d: 1 D_%d - 1 D_min >= 0\n", k, k);
    }
    
    //fprintf(out, "val0: V_0 >= 1\n");
    
    
    //Bounds
    fprintf(out, "Bounds\n");
    for (j = 0; j <= numsec; j++){
        fprintf(out, "-1 <= x%d <= 1\n", j);
    }
    for (k = 1; k <= numscen; k++){
        fprintf(out, "D_%d free\n", k);
    }
    fprintf(out, "D_min free\n");
    fprintf(out, "V_0 >= 1\n");
    fprintf(out, "End\n");
    
    fclose(out);
    
    free(p);
    free(rates);
    // system("mygurobi.py arb.lp");
    //  exit(3);
    
    out = fopen("hidden.dat", "w");
    fclose(out);
    
    sprintf(mybuffer, "python script.py %s hidden.dat nothidden.dat", argv[2]);
    
    printf("mybuffer: %s\n", mybuffer);
    
    if (does_it_exist("nothidden.dat")){
        remove("nothidden.dat");
    }
    
    system(mybuffer);
    
    /** sleep-wake cycle **/
    
    for (;;){
        if (does_it_exist("nothidden.dat")){
            printf("\ngurobi done!\n");
            usleep(1000);
            break;
        }
        else{
            usleep(100);
        }
    }
    /** next, read mygurobi.log **/
    
    results = fopen("mygurobi.log", "r");
    if (!results){
        printf("cannot open mygurobi.log\n"); retcode = 300;
        goto BACK;
    }
    /* read until finding Optimal */
    
    for (;;){
        fscanf(results, "%s", mybuffer);
        /* compare mybuffer to 'Optimal'*/
        if (strcmp(mybuffer, "Optimal") == 0){
            /* now read three more*/
            fscanf(results, "%s", mybuffer);
            fscanf(results, "%s", mybuffer);
            fscanf(results, "%s", mybuffer);
            optimalvalue = atof(mybuffer);
            printf(" value = %g\n", optimalvalue);
            if (optimalvalue > .0001){
                printf("type A arbitrage exists!\n");
                /* read again to get the number of nonzeros*/
                fscanf(results, "%s", mybuffer);
                numnonz = atoi(mybuffer);
                numnonz = numnonz - numscen - 2;
                fscanf(results, "%s", mybuffer); fscanf(results, "%s", mybuffer); fscanf(results, "%s", mybuffer);
                fscanf(results, "%s", mybuffer); fscanf(results, "%s", mybuffer); fscanf(results, "%s", mybuffer);
                fscanf(results, "%s", mybuffer);
                for (k = 0; k < numnonz; k++){
                    fscanf(results, "%s", mybuffer);
                    j = atoi(mybuffer + 1);
                    fscanf(results, "%s", mybuffer); fscanf(results, "%s", mybuffer);
                    xvalue = atof(mybuffer);
                    printf("%d -> %g\n", j, xvalue);
                }
            }
            else{
                printf("no type A\n"); break;
            }
        }
        else if (strcmp(mybuffer, "bye.") == 0){
            break;
        }
    }
    
BACK:
    return retcode;
}


char does_it_exist(char *filename)
{
    struct stat buf;
    
    // the function stat returns 0 if the file exists
    
    if (0 == stat(filename, &buf)){
        return 1;
    }
    else return 0;
}