// Kevin Farokhrouz
// 1002072886
// CSE 3320-001 Lab 2
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_LINE_LENGTH 256
#define MAX_THREADS 10

// Tuple struct to hold CSV rows
typedef struct {
    char text[MAX_LINE_LENGTH];
    int number;
} Tuple;

// Structure to pass multiple arguments to thread function
typedef struct {
    Tuple *data;
    int start;
    int end;
} SortArgs;

// Bubble sort implementation for sorting tuples by number
void bubbleSort(Tuple *arr, int n) {
    int i, j;
    for (i = 0; i < n - 1; i++) {
        for (j = 0; j < n - i - 1; j++) {
            if (arr[j].number > arr[j + 1].number) {
                // Swap tuples
                Tuple temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

// Function to load data from CSV
int loadData(Tuple *data, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    int count = 0;
    while (fscanf(file, "%[^,],%d\n", data[count].text, &data[count].number) == 2) {
        count++;
    }

    fclose(file);
    return count;
}

// Printing the final sorted data
void printSortedData(Tuple *data, int n, const char *identifier) {
    printf("Sorted Data from %s:\n", identifier);
    for (int i = 0; i < n; i++) {
        printf("%d: %s, %d\n", i, data[i].text, data[i].number);
    }
}

// Merge partitions after they're sorted
void merge(Tuple *arr, int low, int mid, int high) {
    int i = low, j = mid + 1, k = 0;
    int n = high - low + 1;
    Tuple *temp = (Tuple *)malloc(n * sizeof(Tuple));

    while (i <= mid && j <= high) {
        if (arr[i].number <= arr[j].number) {
            temp[k++] = arr[i++];
        } else {
            temp[k++] = arr[j++];
        }
    }

    while (i <= mid) {
        temp[k++] = arr[i++];
    }

    while (j <= high) {
        temp[k++] = arr[j++];
    }

    for (i = 0; i < n; i++) {
        arr[low + i] = temp[i];
    }

    free(temp);
}

// Thread function to sort a portion of data
void *threadSort(void *args) {
    SortArgs *sortArgs = (SortArgs *)args;
    int start = sortArgs->start;
    int end = sortArgs->end;
    Tuple *data = sortArgs->data;

    // Sort the portion of the data
    bubbleSort(data + start, end - start + 1);

    // Print sorted part for this thread

    // printf("Thread sorted data (start: %d, end: %d):\n", start, end);
    // printSortedData(data + start, end - start + 1, "Thread");

    return NULL;
}

// Merging function that handles the whole array progressively
void mergeSortedParts(Tuple *data, int n, int num_segments, int segment_size) {
    for (int step = 1; step < num_segments; step *= 2) {
        for (int i = 0; i + step < num_segments; i += 2 * step) {
            int mid = (i + step) * segment_size - 1;
            int end = ((i + 2 * step) * segment_size - 1 < n) ? ((i + 2 * step) * segment_size - 1) : n - 1;
            merge(data, i * segment_size, mid, end);
        }
    }
}

int main() {
    // Load data
    Tuple data[10001]; 
    int n = loadData(data, "data.csv");

    // Sort data in a single process
    printf("Sorting with a single process...\n");
    clock_t start_time_single = clock();
    bubbleSort(data, n);
    clock_t end_time_single = clock();

    // UNCOMMENT this if you would like to view the sorted results.
    // printSortedData(data, n, "Single Process");

    printf("Total time taken (single process sort): %f seconds\n", (double)(end_time_single - start_time_single) / CLOCKS_PER_SEC);

    // Now do partitioning with 2, 4, and 10 threads
    int num_threads_options[] = {2, 4, 10};
    for (int p = 0; p < 3; p++) {
        int num_threads = num_threads_options[p];
        printf("\nPartitioning data into %d parts:\n", num_threads);

        // Calculate segment size and remainder
        int segment_size = n / num_threads;  // Base segment size
        int remainder = n % num_threads;      // Remainder to distribute

        // Start timer for sorting and merging
        clock_t start_time = clock();

        pthread_t threads[MAX_THREADS];
        SortArgs args[MAX_THREADS];

        // Create threads to sort each partition
        for (int i = 0; i < num_threads; i++) {
            args[i].data = data;
            args[i].start = i * segment_size + (i < remainder ? i : remainder);
            args[i].end = (i + 1) * segment_size + (i + 1 < remainder ? (i + 1) : remainder) - 1;

            // Ensure end does not exceed n-1
            if (args[i].end >= n) {
                args[i].end = n - 1;
            }

            pthread_create(&threads[i], NULL, threadSort, (void *)&args[i]);
        }

        // Wait for all threads to finish
        for (int i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);
        }

        // Merging sorted partitions
        mergeSortedParts(data, n, num_threads, segment_size);

        // Stop timer after merging
        clock_t end_time = clock();
        printf("Total time taken (sort and merge with %d threads): %f seconds\n", num_threads, (double)(end_time - start_time) / CLOCKS_PER_SEC);

        // UNCOMMENT if you would like to view the final merged data for this partitioning.
        
        // printf("\nFinal Merged and Sorted Data for %d Threads:\n", num_threads);
        // for (int i = 0; i < n; i++) {
        //     printf("Index %d: %s, %d\n", i, data[i].text, data[i].number);
        // }
    }

    return 0;
}
