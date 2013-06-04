/**
 * Author: Markus Kusano
 *
 * Tool to generate all possibile combinations from passed in input. The
 * interface is designed to hopefully be like UNIX command seq, which will
 * allow it to be used in shell script for loops.
 *
 * The input is a single line of white space separated numbers. The tool will
 * read the first line and generate all the n pick k combinations where n is
 * the input number and k is specified by the user. Each output combination is
 * written on a new line separated by tabs.
 *
 * The input can have multiple numbers, use -c to specify which number to pick
 * from. For example:
 *
 *  combos -c 1 -k 2 "0     6"
 *
 *  will produce all the pick 2 combinations from 6.
 *
 *  The members of each set are separated by commas.
 *
 *  Use -p to specify a prefix number to go infront of each number in the
 *  combination set. For example, if the set is {1,8,2} then -p 0 will produce
 *  {0,1,0,8,0,2}
 */
#include <cstdio>
#include <cstdlib>

// Command line parsing headers
#include <algorithm> // for std::find (to parse command line)
#include <string>
#include <sstream>
#include <climits>
#include <cstring>
#include <cerrno>

// Enable debugging output
#define DEBUG

long n;  // The size of the input set (spans from 0 to n)
long k;  // The size of the pick set ie {0,1}, {0,2} k=2
long prefix;    // Prefix value (see header comment)
bool usePrefix;

// Find the next combination based on the array comb[]
int next_comb(int comb[], int k, int n);

// Sets up n and k from command line options and does error checking
void parseCommandLine(int argc, char *argv[]);

// Retrieves the value associated with the given option. ie given if -c 2 is
// specified on the command line, passing with with the option "c" will return
// "2"
char *getCmdValue(char **begin, char **end, const std::string &option) {
    char ** itr = std::find(begin, end, option);
    if (itr != end && (itr + 1) != end) {
        return *(itr + 1);
    }
    return NULL;
}

// Returns true if a command line option exists
bool cmdOptionExists(char** begin, char** end, const std::string& option)
{
    return std::find(begin, end, option) != end;
}

void parseCommandLine(int argc, char *argv[]) {
    long nValIndex;  // The number from nVals that should be selected as n.
                    // Specified with -c, defaults to 0
 
    nValIndex = 0;

    // The string of potential n values should be the last argument
    std::string nVals;
    nVals = argv[argc - 1];

#ifdef DEBUG
    fprintf(stderr, "[DEBUG] command line n value string == %s\n", nVals.c_str());
#endif

    //// Parse -c
    char *nValIndexStr; // The extracted string after -c 
    bool nValOptionExists; // Was -c passed or not?
    nValOptionExists = cmdOptionExists(argv, argv + argc, (std::string) "-c");

    nValIndexStr = getCmdValue(argv, argv + argc, (std::string) "-c");

#ifdef DEBUG
    if (nValOptionExists) {
        fprintf(stderr, "[DEBUG] -c exists\n");
    }
    else {
        fprintf(stderr, "[DEBUG] -c does not exist\n");
    }

    if (nValIndexStr == NULL)
        fprintf(stderr, "[DEBUG] no value for -c specified\n");
    else 
        fprintf(stderr, "[DEBUG] value for -c == %s\n", nValIndexStr);
#endif

    if (nValIndexStr == NULL && nValOptionExists) {
        fprintf(stderr, "Error: -c requires an argument\n");
        exit(EXIT_FAILURE);
    }

    if (nValIndexStr != NULL) {
        nValIndex = strtol(nValIndexStr, NULL, 10);
        if (nValIndex == LONG_MAX || nValIndex == LONG_MIN) {
            if (errno != 0) {
                perror("Error: invalid value found for -c");
                exit(EXIT_FAILURE);
            }
        }
        else if (nValIndex == 0L) {
            fprintf(stderr, "Warning: potential invalid conversion for -c argument, "
                            "defaulting to 0 (argument = %s)\n", nValIndexStr);
        }
        else if (nValIndex < 0L) {
            fprintf(stderr, "Error: value for -c cannot be less than 0\n");
            exit(EXIT_FAILURE);
        }
    }

    //// Extract n value
    std::stringstream ss(nVals);
    for (long i = 0; i < nValIndex + 1L; i++) {
        ss >> n;
#ifdef DEBUG
        fprintf(stderr, "[DEBUG] extracted value %ld from input string\n", n);
#endif
        if (ss.fail()) {
            fprintf(stderr, "Error: Malformed input string: %s\n", argv[argc-1]);
            exit(EXIT_FAILURE);
        }
        else if (ss.eof() && i != nValIndex) { // ie this is not the last iteration
            fprintf(stderr, "Error: Malformed input string: %s\n", argv[argc-1]);
            fprintf(stderr, "expected atleast %ld values (from -c = %ld)\n", nValIndex + 1, nValIndex);
            exit(EXIT_FAILURE);
        }
    }

    //// Extract k value
    bool kOptionExists;
    char *kOptionValue;

    kOptionExists = cmdOptionExists(argv, argv + argc, "-k");
    if (!kOptionExists) {
        fprintf(stderr, "Error: -k must be specified\n");
        exit(EXIT_FAILURE);
    }

    kOptionValue = getCmdValue(argv, argc + argv, "-k");

    if (kOptionValue == NULL && kOptionExists) {
        fprintf(stderr, "Error: -k requires a value\n");
        exit(EXIT_FAILURE);
    }

    k = strtol(kOptionValue, NULL, 10);
    if (k == 0L) {
        fprintf(stderr, "Error: error converting -k value (%s) to integer\n", kOptionValue);
        exit(EXIT_FAILURE);
    }

    if (k == LONG_MIN || k == LONG_MAX) {
        if (errno != 0) {
            perror("Error: error converting -k value to integer");
            exit(EXIT_FAILURE);
        }
    }

    // Extract p value
    bool pOptionExists;
    char *pOptionValue;

    pOptionExists = cmdOptionExists(argv, argv + argc, "-p");
    if (!pOptionExists) {
        usePrefix = false;
    }
    else {
        usePrefix = true;
        pOptionValue = getCmdValue(argv, argc + argv, "-p");

        if (kOptionValue == NULL && kOptionExists) {
            fprintf(stderr, "Error: -p requires a value\n");
            exit(EXIT_FAILURE);
        }

        prefix = strtol(pOptionValue, NULL, 10);
        if (prefix == LONG_MIN || prefix == LONG_MAX) {
            if (errno != 0) {
                perror("Error: error converting -p value to integer");
                exit(EXIT_FAILURE);
            }
        }
    }
    
#ifdef DEBUG
    fprintf(stderr, "[DEBUG] c == %ld\n", nValIndex);
    fprintf(stderr, "[DEBUG] n == %ld\n", n);
    fprintf(stderr, "[DEBUG] k == %ld\n", k);
    fprintf(stderr, "[DEBUG] usePrefix == %s\n", (usePrefix) ? "true" : "false");
    fprintf(stderr, "[DEBUG] prefix == %ld\n", prefix);
#endif
}

int main(int argc, char *argv[]) {
    parseCommandLine(argc, argv);

    if (n < 0 || k < 0) {
        fprintf(stderr, "Error: neither n nor k can be negative\n");
        exit(EXIT_FAILURE);
    }

    if (k > n + 1) {
        fprintf(stderr, "Error: k > (n+1), cannot pick %ld items from %ld\n", k, n + 1);
        exit(EXIT_FAILURE);
    }

    // array of integers that is the set for the current combinations
    int *combo = (int *) malloc(sizeof(int) * k);

    // The first combination is from 0 to k
    for (int i = 0; i < k; i++) {
        combo[i] = i;   // load the array with the combination so the next one
                        //can be calculated
        if (i == k - 1) { // Don't print out a comma on last iteration
            if (usePrefix)
                printf("%ld,%d", prefix, i);
            else
                printf("%d", i);
        }
        else {
            if (usePrefix)
                printf("%ld,%d,", prefix, i);
            else
                printf("%d,", i);
        }
    }
    printf("\n");

    while (next_comb(combo, k, n + 1)) {
        for (int i = 0; i < k; i++) {
            if (i == k - 1) { // Don't print out a comma on last iteration
                if (usePrefix)
                    printf("%ld,%d", prefix, combo[i]);
                else
                    printf("%d", combo[i]);
            }
            else {
                if (usePrefix)
                    printf("%ld,%d,", prefix, combo[i]);
                else
                    printf("%d,", combo[i]);
            }
        }
        printf("\n");
    }

    return 0;
}

// Source: https://compprog.wordpress.com/2007/10/17/generating-combinations-1/
int next_comb(int comb[], int k, int n) {
        int i = k - 1;
        ++comb[i]; // increment the last value in the combination
        while ((i >= 0) && (comb[i] >= n - k + 1 + i)) {
                --i;
                ++comb[i];
        }

        if (comb[0] > n - k) /* Combination (n-k, n-k+1, ..., n) reached */
                return 0; /* No more combinations can be generated */

        /* comb now looks like (..., x, n, n, n, ..., n).
        Turn it into (..., x, x + 1, x + 2, ...) */
        for (i = i + 1; i < k; ++i)
                comb[i] = comb[i - 1] + 1;

        return 1;
}
