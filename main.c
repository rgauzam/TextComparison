#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PRIME 101
#define MIN_WORDS 4
#define CHUNK_SIZE 1024
#define BASE 256

// Function to read the contents of a file and return it as a string
char* readFileAsString(const char* filePath) {
    FILE* file = fopen(filePath, "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    char* content = NULL;
    char buffer[CHUNK_SIZE];
    size_t contentSize = 0;
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        char* newContent = realloc(content, contentSize + bytesRead + 1);
        if (!newContent) {
            perror("Error reallocating memory");
            exit(EXIT_FAILURE);
        }
        content = newContent;
        memcpy(content + contentSize, buffer, bytesRead);
        contentSize += bytesRead;
    }
    content = realloc(content, contentSize + 1);
    if (!content) {
        perror("Error reallocating memory");
        exit(EXIT_FAILURE);
    }
    content[contentSize] = '\0';
    fclose(file);
    return content;
}

// Function to tokenize a string into words
char** tokenize(const char* str, int* count) {
    char* copy = strdup(str);
    const char* delimiters = " \n\t\r.,;:!?'\"()[]{}*#_—-’”“™"; // signs in texts as "***" are not identified as word
    char* token = strtok(copy, delimiters);
    int capacity = 10;
    int size = 0;
    char** tokens = (char**)malloc(capacity * sizeof(char*));

    while (token != NULL) {
        if (size == capacity) {
            capacity *= 2;
            tokens = (char**)realloc(tokens, capacity * sizeof(char*));
        }
        tokens[size++] = strdup(token);
        token = strtok(NULL, delimiters);
    }
    free(copy);
    *count = size;
    return tokens;
}

char* createWordGroup(char** words, int start, int size) {
    int length = 0;
    for (int i = start; i < start + size; i++) {
        length += strlen(words[i]) + 1; // +1 for space
    }
    char* group = (char*)malloc(length + 1); // +1 for null terminator
    if (!group) {
        perror("Error allocating memory");
        exit(EXIT_FAILURE);
    }
    group[0] = '\0';
    for (int i = start; i < start + size; i++) {
        strcat(group, words[i]);
        if (i < start + size - 1) {
            strcat(group, " ");
        }
    }
    return group;
}

unsigned long rollingHash(const char* str, int len) {
    unsigned long hash = 0;
    for (int i = 0; i < len; ++i) {
        hash = (BASE * hash + str[i]) % PRIME;
    }
    return hash;
}

double calculatePlagiarismPercentage(int plagiarismsCount, int totalWordCount) {
    return (double)plagiarismsCount / totalWordCount * 100;
}


void findPlagiarism(const char* text1, const char* text2, int words) {
    int count1, count2;
    char** words1 = tokenize(text1, &count1);
    char** words2 = tokenize(text2, &count2);

    int plagiarismsCount = 0, totalWordCount = count1;
    int* plagiarizedIndices = (int*)calloc(count1, sizeof(int));
    int* sentenceLengths = (int*)calloc(count1, sizeof(int)); // Array to track sentence lengths

    if (count1 < words || count2 < words) return;

    // Create hash table for all word groups in text2
    unsigned long* hashesText2 = (unsigned long*)malloc((count2 - words + 1) * sizeof(unsigned long));
    for (int i = 0; i <= count2 - words; ++i) {
        char* group = createWordGroup(words2, i, words);
        hashesText2[i] = rollingHash(group, strlen(group));
        free(group);
    }

    // Check each word group in text1
    for (int i = 0; i <= count1 - words; ++i) {
        char* group1 = createWordGroup(words1, i, words);
        unsigned long hash1 = rollingHash(group1, strlen(group1));

        for (int j = 0; j <= count2 - words; ++j) {
            if (hash1 == hashesText2[j]) {
                char* group2 = createWordGroup(words2, j, words);
                if (strcmp(group1, group2) == 0) {
                    if (!plagiarizedIndices[i]) {
                        int additionalWords = 0;
                        int nextIndex = i + words;
                        while (nextIndex < count1 && strcmp(words1[nextIndex], words2[j + words + additionalWords]) == 0) {
                            additionalWords++;
                            nextIndex++;
                        }

                        char* extendedGroup = createWordGroup(words1, i, words + additionalWords);
                        printf(" · %s\n", extendedGroup);
                        free(extendedGroup);

                        plagiarismsCount++;
                        sentenceLengths[words + additionalWords]++; // Increment count for the detected sentence length

                        for (int k = i; k < nextIndex; ++k) {
                            plagiarizedIndices[k] = 1;
                        }
                        i = nextIndex - 1; // Skip already checked words
                        break;
                    }
                }
                free(group2);
            }
        }
        free(group1);
    }
    printf("\n--------------  STATISTICS:  ---------------------------------------------------------- \n\n");
    // Display sentence length statistics
    for (int i = words; i < count1; i++) {
        if (sentenceLengths[i] > 0) {
            printf("\t%d-words sentences detected: %d times\n", i, sentenceLengths[i]);
        }
    }

    // Count plagiarized words
    int plagiarizedWords = 0;
    for (int i = 0; i < count1; ++i) {
        if (plagiarizedIndices[i]) plagiarizedWords++;
    }

    printf("\n\tTotal words: %d\n", totalWordCount);
    printf("\tPlagiarized words: %d\n", plagiarizedWords);
    printf("\tPlagiarism score: %.4f%%\n\n", calculatePlagiarismPercentage(plagiarizedWords, totalWordCount));


    free(sentenceLengths);
    free(hashesText2);
    free(plagiarizedIndices);
    for (int i = 0; i < count1; i++) free(words1[i]);
    for (int i = 0; i < count2; i++) free(words2[i]);
    free(words1);
    free(words2);
}



int main() {
    const char* filePath1 = "/Users/oliwiaw/Desktop/TXT_Files/dracula.txt";
    const char* filePath2 = "/Users/oliwiaw/Desktop/TXT_Files/frankenstein.txt";
    char* text1 = readFileAsString(filePath1);
    char* text2 = readFileAsString(filePath2);
    printf("\n--------------------  PLAGIARIZED FRAGMENTS:  ---------------------------------------------------------- \n\n");
    clock_t startTime = clock();
    findPlagiarism(text1, text2, MIN_WORDS);
    clock_t endTime = clock();

    double executionTime = ((double)(endTime - startTime)) / CLOCKS_PER_SEC;
    printf("\tExecution time of finding plagiarism and counting statistics: %.4f seconds\n", executionTime);

    free(text1);
    free(text2);
    return 0;
}
