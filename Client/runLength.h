#include <stdio.h>
#include <stdlib.h>

// Function to encode the input file using Run-Length Encoding with ASCII-based count

void encodeFile(const char *inputFile, const char *outputFile) {
    FILE *in = fopen(inputFile, "r");
    FILE *out = fopen(outputFile, "w");
    
    if (in == NULL || out == NULL) {
        printf("Error opening file.\n");
        exit(1);
    }

    char ch, prev;
    int count = 1;

    prev = fgetc(in); // Read the first character
    if (prev == EOF) {
        fclose(in);
        fclose(out);
        return; // If the file is empty, nothing to encode
    }
    
    while ((ch = fgetc(in)) != EOF) {
        if (ch == prev) {
            count++;
        } else {
            // Convert count to a char and write it with the character
            char val = count;
            fputc(val, out);  // Write the ASCII-encoded count
            fputc(prev, out); // Write the character
            count = 1; // Reset count for the new character
        }
        prev = ch;
    }

    // Write the last run to the output file
    char val = count;
    fputc(val, out);  // Write the ASCII-encoded count
    fputc(prev, out); // Write the last character

    fclose(in);
    fclose(out);
}

// Function to decode the file encoded with Run-Length Encoding
void decodeFile(const char *inputFile, const char *outputFile) {
    FILE *in = fopen(inputFile, "r");
    FILE *out = fopen(outputFile, "w");
    
    if (in == NULL || out == NULL) {
        printf("Error opening file.\n");
        exit(1);
    }

    char countChar, ch;
    int count;

    // Read count (as ASCII) and character alternately from the input file
    while ((countChar = fgetc(in)) != EOF && (ch = fgetc(in)) != EOF) {
        count = (unsigned char) countChar; // Convert ASCII back to int (unsigned for correct range)
        
        for (int i = 0; i < count; i++) {
            fputc(ch, out); // Write character 'count' times
        }
    }

    fclose(in);
    fclose(out);
}

