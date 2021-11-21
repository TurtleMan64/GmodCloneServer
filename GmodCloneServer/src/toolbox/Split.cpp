#include <stdlib.h> 

#include "split.hpp"

char** split(char* line, char delim, int* length)
{
    /* Scan through line to find the number of tokens */
    int numTokens = 0;
    int index = 0;
    int inToken = 0;

    while (line[index] != 0)
    {
        if (line[index] != delim && inToken == 0)
        {
            inToken = 1;
            numTokens += 1;
        }
        else if (line[index] == delim)
        {
            inToken = 0;
        }
        index += 1;
    }

    /* Get memory to store the data */
    char ** parsedData = (char**)malloc(sizeof(char*)*(numTokens + 1));

    /* Scan through line to fill parsedData
    and set 0 characters after tokens*/
    index = 0;
    inToken = 0;
    int tokenNum = 0;

    while (line[index] != 0)
    {
        if (line[index] != delim && inToken == 0)
        {
            parsedData[tokenNum] = &line[index];
            tokenNum += 1;
            inToken = 1;
        }
        else if (line[index] == delim)
        {
            if (inToken == 1)
            {
                line[index] = 0;
            }
            inToken = 0;
        }
        index += 1;
    }

    parsedData[numTokens] = NULL;

    *length = numTokens;

    return parsedData;
}

void split(char* line, char delim, int* numFound, char** tokenPointers, int maxNumTokensToFind)
{
    int numTokensFound = 0;
    int index = 0;
    bool inToken = false;

    while (line[index] != 0 && numTokensFound < maxNumTokensToFind)
    {
        if (line[index] != delim && !inToken)
        {
            inToken = true;
            line[index] = 0;
            tokenPointers[numTokensFound] = &line[index];
            numTokensFound += 1;
        }
        else if (line[index] == delim)
        {
            inToken = false;
        }
        index += 1;
    }

    *numFound = numTokensFound;
}
