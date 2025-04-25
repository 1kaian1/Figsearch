#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    char *self;
    int rowNum;
    int colNum;
} Bitmap;

Bitmap bitmap;

// Mostly for better economy. Closes given file, prints out exitMessage, frees bitmap and exits the program.
void terminator(const int exitCode, FILE* outputStream, const char *exitMessage, FILE* fileStream) {
    if (fileStream != NULL) {
        fclose(fileStream);
    }
    fprintf(outputStream, "%s", exitMessage);
    free(bitmap.self);
    exit(exitCode);
}

int validateAndLoadImage(const char *fileName) {
    FILE *file = fopen(fileName, "r");
    if (file == NULL) {
        terminator(1, stderr, "Invalid\n", file);
    }

    // Scanning input row and column values, which can be minimum of 1.
    if (fscanf(file, " %d %d", &bitmap.rowNum, &bitmap.colNum) != 2 || bitmap.rowNum < 1 || bitmap.colNum < 1) {
        terminator(1, stderr, "Invalid\n", file);
    }

    int areaScale = bitmap.rowNum * bitmap.colNum;

    // malloc: Allocating 1D matrix of areaScale size. Allocating the most economic and efficient type - char.
    bitmap.self = malloc(areaScale);
    if (bitmap.self == NULL) {
        terminator(1, stderr, "Invalid\n", file);
    }

    bool bitmapIsEmpty = true;
    int index = 0;
    char element;
    // Loading every bit from bitmap one by one till it can't read the next one (likely EOF but can be something else).
    while (fscanf(file, " %c", &element) == 1) {
        if ((element == '1' || element == '0') && areaScale > 0) {
            if (element == '1' && bitmapIsEmpty) {
                bitmapIsEmpty = false;
            }
            bitmap.self[index] = element;
            areaScale--;
        } else {
            terminator(1, stderr, "Invalid\n", file);
        }
        index++;
    }

    // If the end of file isn't reached or there is a difference between number of loaded elements and areaScale,
    // input is considered invalid.
    if (!feof(file) || areaScale != 0) {
        terminator(1, stderr, "Invalid\n", file);
    }

    fclose(file);

    // The bitmap is empty (meaning there is no "1"s but only zeros), returns 1.
    if (bitmapIsEmpty) {
        return 1;
    }
    // If the bitmap is valid and not empty, returns 0.
    return 0;
}

// This method looks for longest line in bitmap. hLine and vLine methods were very similar, so I made them into one.
int lineLength(const int startIndex, const char action) {
    int indexMove = 0;
    int maxStep = 0;
    int len = 0;

    // With every iteration when processing hLine we need to move to the right by one bit. -> indexMove = 1;
    // MaxStep we can make till we reach the borders of the bitmap (hLine): maxStep = colNum-startIndex%colNum;
    if (action == 'h') {
        indexMove = 1;
        maxStep = bitmap.colNum-startIndex%bitmap.colNum;

    // With every iteration when processing vLine we need to move down by whole column. -> indexMove = colNum;
    // MaxStep we can make till we reach the borders of the bitmap (vLine): maxStep = rowNum-startIndex/colNum;
    } else if (action == 'v') {
        indexMove = bitmap.colNum;
        maxStep = bitmap.rowNum-startIndex/bitmap.colNum;
    }

    // Now that we know parameters for our for-loop, we can start looking for the longest line at startIndex.
    for (int index = startIndex; index < startIndex+indexMove*maxStep; index+=indexMove) {
        if (bitmap.self[index] == '0') {
            break;
        }
        len++;
    }
    return len;
}

// This method counts the longest squareLineLength on given startIndex.
int squareLineLength(const int startIndex) {
    // On each startIndex, we need to find 4 same-length lines to produce a square. These are the first two (primaries).
    const int primaryHLineLen = lineLength(startIndex, 'h');
    const int primaryVLineLen = lineLength(startIndex, 'v');

    const int maxPotentialSquareLineLength = (primaryHLineLen > primaryVLineLen) ? primaryVLineLen : primaryHLineLen;

    // Given maxPotentialSquareLineLength we can go through every bit and try to complement the square with secondaries.
    int index;
    for (index = maxPotentialSquareLineLength-1; index >= 0; index--) {

        // These are the secondaries. They will lead from where the primaries ended up, complementing the square.
        const int secondaryHLineLen = lineLength(startIndex + index * bitmap.colNum, 'h'); // hLine from primaryVLine end
        const int secondaryVLineLen = lineLength(startIndex + index, 'v'); // vLine from primaryHLine end

        // If the secondaries line length is at least the same as index+1, we have found the largest square.
        if (secondaryHLineLen >= index+1 && secondaryVLineLen >= index+1) {
            break;
        }
    }
    return index+1;
}

void mainNodeLoop(const char action) {
    int lineLen = 0;
    int maxLineLen = 0;
    int maxLineStartIndex = -1;
    int maxLineEndIndex; // -1;

    // The main node loop runs through every bit in bitmap and checks for longest hLine/vLine or largest square in them.
    for (int index = 0; index < bitmap.rowNum*bitmap.colNum; index++) {
        if (bitmap.self[index] == '1') {
            if (action == 's') {
                // squareLineLength: Finds the largest square in bitmap. Returns longest lineLen at given startIndex.
                lineLen = squareLineLength(index);
            } else {
                // lineLength: Finds the longest hLine/vLine in bitmap. Returns longest lineLen at given startIndex.
                lineLen = lineLength(index, action);
            }
            if (lineLen > maxLineLen) {
                maxLineLen = lineLen;
                maxLineStartIndex = index;
            }
        }
    }

    // maxLineEndIndex (hLine): The endIndex is equal to: startIndex + lineLen - 1. Both indexes are part of lineLen.
    if (action == 'h') {
        maxLineEndIndex = maxLineStartIndex + maxLineLen-1;
    // maxLineEndIndex (vLine): The endIndex is equal to: startIndex + (lineLen-1) * colNum. We move down by colNum.
    } else if (action == 'v') {
        maxLineEndIndex = maxLineStartIndex + (maxLineLen-1)*bitmap.colNum;
    // maxLineEndIndex (square): Both hLine and vLine are considered when counting the endIndex of a square.
    } else {
        maxLineEndIndex = maxLineStartIndex + maxLineLen-1 + (maxLineLen-1)*bitmap.colNum;
    }

    // We need to convert startIndex/endIndex to row/column they represent. Both using colNum. Confusing, right?
    printf("%d %d %d %d\n", maxLineStartIndex/bitmap.colNum, maxLineStartIndex%bitmap.colNum,
                                  maxLineEndIndex/bitmap.colNum  , maxLineEndIndex%bitmap.colNum  );
}

int main(const int argc, char *argv[]) {
    // The argc can be either 2 or 3. Everything else is invalid.
    if (argc < 2 || argc > 3) {
        terminator(1, stderr, "Error: Invalid number of arguments.\nUse --help for more information.\n", NULL);
    }
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        printf("Figsearch by Jan Kai Marek\n"
                   "./figsearch --help : Print this help.\n"
                   "./figsearch test FILE : Check for valid input.\n"
                   "./figsearch hline FILE : Search for longest horizontal line in bitmap.\n"
                   "./figsearch vline FILE : Search for longest vertical line in bitmap.\n"
                   "./figsearch square FILE : Search for largest square in bitmap.\n");
        terminator(0, stdout, "", NULL);
    }
    int status = validateAndLoadImage(argv[2]);
    if (strcmp(argv[1], "hline") == 0 || strcmp(argv[1], "vline") == 0 || strcmp(argv[1], "square") == 0) {

        // validateAndLoadImage returns 1 when there is zero 1s in bitmap, meaning the bitmap is empty.
        if (status == 1) {
            terminator(0, stdout, "Not found\n", NULL);

        // validateAndLoadImage returns 0 when the bitmap is valid and not empty.
        } else {
            mainNodeLoop(argv[1][0]);
            terminator(0, stdout, "", NULL);
        }

    } else if (strcmp(argv[1], "test") == 0) {
        terminator(0, stdout, "Valid\n", NULL);
    } else {
        terminator(1, stderr, "Error: Unrecognized argument.\nUse --help for more information.\n", NULL);
    }
}