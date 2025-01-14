#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "MinPopVote.h"

// Function to set settings based on command line arguments
bool setSettings(int argc, char** argv, int* year, bool* fastMode, bool* quietMode) {
    *year = 0; 
    *fastMode = false; 
    *quietMode = false; 

    // Iterate through command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-y") == 0) { // Year argument
            int temp = atoi(argv[++i]);
            if (temp >= 1828 && temp <= 2020 && temp % 4 == 0) {
                *year = temp;
            }
        }
        else if (strcmp(argv[i], "-q") == 0) { // Quiet mode argument
            *quietMode = true;
        }
        else if (strcmp(argv[i], "-f") == 0) { // Fast mode argument
            *fastMode = true;
        }
        else {
            return false; // Invalid argument
        }
    }
    return true; // Settings successfully set
}

// Function to generate input filename based on year
void inFilename(char* filename, int year) {
    sprintf(filename, "data/%d.csv", year);
    return;
}

// Function to generate output filename based on year
void outFilename(char* filename, int year) {
    sprintf(filename, "toWin/%d_win.csv", year);
    return;
}

// Function to parse a line from CSV file into State struct
bool parseLine(char* line, State* myState) {
    int count = 0;
    for (int i = 0; i < strlen(line); i++) {
        if (line[i] == ',') {
            count++;
        }
    }

    if (count != 3) { // Each line should have 4 fields separated by commas
        return false;
    } 
    else {
        int instance = 0;
        int start = 0;
        
        // Parse and store each field into State struct
        for (int i = 0; i <= strlen(line); i++) {
            if (line[i] == ',' || line[i] == '\0' || line[i] == '\n') {
                int newlen = i - start;
                char temp[50];
                strncpy(temp, &line[start], newlen);
                temp[newlen] = '\0';

                if (instance == 0) {
                    strcpy(myState->name, temp);
                } 
                else if (instance == 1) {
                    strcpy(myState->postalCode, temp);
                } 
                else if (instance == 2) {
                    myState->electoralVotes = atoi(temp);
                } 
                else if (instance == 3) {
                    myState->popularVotes = atoi(temp);
                }
                start = i + 1;
                instance++;
            }
        }
    }
    return true; // Line successfully parsed
}

// Function to read election data from CSV file into an array of State structs
bool readElectionData(char* filename, State* allStates, int* nStates) {
    *nStates = 0; // Initialize state count

    FILE* data = fopen(filename,"r");
    if (data == NULL) {
        printf("File %s not found...\n", filename);
        return false;
    }
    else {
        // Read each line from file and parse into State struct
        char line[500];
        while(fgets(line, 500, data) != NULL) {
            bool parse = parseLine(line, &allStates[*nStates]);
            if (parse) (*nStates)++;
        }
    }
    fclose(data);
 
    return true; // Data read successfully
}

// Function to calculate total electoral votes from an array of states
int totalEVs(State* states, int szStates) {
    int total = 0;
    for (int i = 0; i < szStates; i++) {
        total += states[i].electoralVotes;
    }
    return total;
}

// Function to calculate total popular votes from an array of states
int totalPVs(State* states, int szStates) {
    int total = 0;
    for (int i = 0; i < szStates; i++) {
        total += states[i].popularVotes;
    }
    return total;
}

/* Recursive helper function 
   to find the minimum popular vote needed to win 
   with at least a given number of electoral votes  */
MinInfo minPopVoteAtLeast(State* states, int szStates, int start, int EVs) {

    if (start == szStates) {
        MinInfo res;
        res.szSomeStates = 0;
        res.subsetPVs = 0;

        if (EVs <= 0) {
            res.sufficientEVs = true;
        }
        else {
            res.sufficientEVs = false;
        }

        return res;
    }

    else { 
        MinInfo notIncluding = minPopVoteAtLeast(states, szStates, start+1, EVs);

        MinInfo including = minPopVoteAtLeast(states, szStates, start+1, EVs - states[start].electoralVotes);
        including.subsetPVs += ((states[start].popularVotes/2) + 1);
        including.someStates[including.szSomeStates++] = states[start];

        if (including.sufficientEVs && !notIncluding.sufficientEVs) {
            return including;
        }
        else if (!including.sufficientEVs && notIncluding.sufficientEVs) {
            return notIncluding;
        }
        else {
            if (including.subsetPVs < notIncluding.subsetPVs) {
                return including;
            }
            else {
                return notIncluding;
            }
        }
    }
}

// Function to find the minimum popular vote needed to win the election
MinInfo minPopVoteToWin(State* states, int szStates) {
    int totEVs = totalEVs(states,szStates);
    int reqEVs = totEVs/2 + 1; // required EVs to win election
    return minPopVoteAtLeast(states, szStates, 0, reqEVs);
}

// Function to find the minimum popular vote needed to win with memoization for optimization
MinInfo minPopVoteAtLeastFast(State* states, int szStates, int start, int EVs, MinInfo** memo) {

    if (EVs < 0) {
        EVs = 0;
    }

    if (start == szStates) {
        MinInfo res;
        res.szSomeStates = 0;
        res.subsetPVs = 0;

        if (EVs <= 0) {
            res.sufficientEVs = true;
        }
        else {
            res.sufficientEVs = false;
        }
        return res;
    }

    else if (memo[start][EVs].subsetPVs != -1) {
        return memo[start][EVs];
    }

    else {
        MinInfo notIncluding = minPopVoteAtLeastFast(states, szStates, start+1, EVs, memo);

        MinInfo including = minPopVoteAtLeastFast(states, szStates, start+1, EVs - states[start].electoralVotes, memo);
        including.subsetPVs += ((states[start].popularVotes/2) + 1);
        including.someStates[including.szSomeStates++] = states[start];

        if (including.sufficientEVs && !notIncluding.sufficientEVs) {
            memo[start][EVs] = including;
        }
        else if (!including.sufficientEVs && notIncluding.sufficientEVs) {
            memo[start][EVs] = notIncluding;
        }
        else {
            if (including.subsetPVs < notIncluding.subsetPVs) {
                memo[start][EVs] = including;
            }
            else {
                memo[start][EVs] = notIncluding;
            }
        }
        return memo[start][EVs];
    }
}

// Function to find the minimum popular vote needed to win the election with memoization
MinInfo minPopVoteToWinFast(State* states, int szStates) {
    int totEVs = totalEVs(states,szStates);
    int reqEVs = totEVs/2 + 1; // required EVs to win election

    MinInfo** memo = (MinInfo**)malloc((szStates+1)*sizeof(MinInfo*));
    for (int i = 0; i < szStates+1; ++i) {
        memo[i] = (MinInfo*)malloc((reqEVs+1)*sizeof(MinInfo));
        for (int j = 0; j < reqEVs+1; ++j) {
            memo[i][j].subsetPVs = -1;
        }
    }
    MinInfo res = minPopVoteAtLeastFast(states, szStates, 0, reqEVs, memo);

    for (int i = 0; i < szStates+1; ++i) {
        free(memo[i]);
    }
    free(memo);

    return res;
}

// Function to write subset data to a file
bool writeSubsetData(char* filenameW, int totEVs, int totPVs, int wonEVs, MinInfo toWin) {

    FILE* data = fopen(filenameW,"w");
    if (data == NULL) {
        printf("File %s not found...\n", filenameW);
        return false;
    }
    else {
        fprintf(data, "%d,%d,%d,%d\n", totEVs, totPVs, wonEVs, toWin.subsetPVs);

        for (int i = toWin.szSomeStates-1; i >= 0; i--) {
            fprintf(data, "%s,%s,%d,%d\n", toWin.someStates[i].name, 
                                           toWin.someStates[i].postalCode, 
                                           toWin.someStates[i].electoralVotes, 
                                          (toWin.someStates[i].popularVotes/2) +1);
        }
    }
    fclose(data);
    return true;
}
