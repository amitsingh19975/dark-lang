#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sys/types.h"

char HexChar [16] = { '0', '1', '2', '3', '4', '5', '6', '7',
                      '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

static int * initHexArray (char * pDecStr, int * pnElements);

static void addDecValue (int * pMyArray, int nElements, int value);
static void printHexArray (int * pHexArray, int nElements);

static void
addDecValue (int * pHexArray, int nElements, int value)
{
    int carryover = value;
    int tmp = 0;
    int i;

    /* start at the bottom of the array and work towards the top
     *
     * multiply the existing array value by 10, then add new value.
     * carry over remainder as you work back towards the top of the array
     */
    for (i = (nElements-1); (i >= 0); i--)
    {
        tmp = (pHexArray[i] * 10) + carryover;
        pHexArray[i] = tmp % 16;
        carryover = tmp / 16;
    }
}

static int *
initHexArray (char * pDecStr, int * pnElements)
{
    int * pArray = NULL;
    int lenDecStr = strlen (pDecStr);
    int i;

    /* allocate an array of integer values to store intermediate results
     * only need as many as the input string as going from base 10 to
     * base 16 will never result in a larger number of digits, but for values
     * less than "16" will use the same number
     */

    pArray = (int *) calloc (lenDecStr,  sizeof (int));

    for (i = 0; i < lenDecStr; i++)
    {
        addDecValue (pArray, lenDecStr, pDecStr[i] - '0');
    }

    *pnElements = lenDecStr;

    return (pArray);
}

static void
printHexArray (int * pHexArray, int nElements)
{
    int start = 0;
    int i;

    /* skip all the leading 0s */
    while ((pHexArray[start] == 0) && (start < (nElements-1)))
    {
        start++;
    }

    for (i = start; i < nElements; i++)
    {
        printf ("%c", HexChar[pHexArray[i]]);
    }

    printf ("\n");
}

int
main (int argc, char * argv[])
{
    int i;
    int * pMyArray = NULL;
    int nElements;

    if (argc < 2)
    {
        printf ("Usage: %s decimalString\n", argv[0]);
        return (-1);
    }

    pMyArray = initHexArray (argv[1], &nElements);

    printHexArray (pMyArray, nElements);

    if (pMyArray != NULL)
        free (pMyArray);

    return (0);
}