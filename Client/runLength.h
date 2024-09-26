#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function to encode the input file using Run-Length Encoding
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
    while ((ch = fgetc(in)) != EOF) {
        if (ch == prev) {
            count++;
        } else {
            // Write the encoded character and count to the output file
            fprintf(out, "%d%c", count, prev);
            count = 1; // Reset count for the new character
        }
        prev = ch;
    }

    // Write the last run to the output file
    if (count > 0) {
        fprintf(out, "%d%c", count, prev);
    }

    fclose(in);
    fclose(out);
}

// Function to decode the encoded file back to original text
void decodeFile(const char *inputFile, const char *outputFile) {
    FILE *in = fopen(inputFile, "r");
    FILE *out = fopen(outputFile, "w");
    
    if (in == NULL || out == NULL) {
        printf("Error opening file.\n");
        exit(1);
    }

    int count;
    char ch;

    // Read count and character alternately from the input file
    while (fscanf(in, "%d%c", &count, &ch) == 2) {
        for (int i = 0; i < count; i++) {
            fputc(ch, out); // Write character 'count' times
        }
    }

    fclose(in);
    fclose(out);
}

// int main() {
//     const char *inputFile = "text.txt";
//     const char *encodedFile = "encoding.txt";
//     const char *decodedFile = "decoding.txt";

//     // Encode the input file
//     encodeFile(inputFile, encodedFile);
//     printf("Encoding complete! Check encoding.txt\n");

//     // Decode the encoded file back to original text
//     decodeFile(encodedFile, decodedFile);
//     printf("Decoding complete! Check decoding.txt\n");

//     return 0;
// }