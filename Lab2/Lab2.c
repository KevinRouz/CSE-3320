#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#define MAX_LINE_LENGTH 256

// Tuple struct to hold csv rows
typedef struct {
    char text[MAX_LINE_LENGTH];
    int number;
} Tuple;

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

// Forking and executing sorting
void parallelSort(Tuple *data, int start, int end, int pipe_fd[], int process_id) {
    close(pipe_fd[0]); // Close reading end

    // Sort the portion of the data
    int n = end - start + 1;
    bubbleSort(data + start, n);

    // Print sorted part for this child process
    printf("Process %d sorted data:\n", process_id);
    printSortedData(data + start, n, "Child Process");

    // Send sorted data to parent
    write(pipe_fd[1], &data[start], n * sizeof(Tuple));
    close(pipe_fd[1]); // Close writing end
    exit(0); // Child exits after sorting
}

// Merging function that handles the whole array progressively
void parallelMerge(Tuple *data, int n, int num_segments, int segment_size) {
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

    // Now do partitioning with 2, 4, and 10 processes
    int num_processes[] = {2, 4, 10};
    for (int p = 0; p < 3; p++) {
        int num = num_processes[p];
        printf("\nPartitioning data into %d parts:\n", num);

        // Calculate segment size and remainder
        int segment_size = n / num;  // Base segment size
        int remainder = n % num;      // Remainder to distribute

        // Array to hold the partitions
        Tuple partitions[num][segment_size + 1]; // +1 to accommodate remainder in the last partition
        int partition_sizes[num];

        // Start timer for sorting and merging
        clock_t start_time = clock();

        // Fill the partitions
        for (int i = 0; i < num; i++) {
            int start = i * segment_size + (i < remainder ? i : remainder);
            int end = (i + 1) * segment_size + (i + 1 < remainder ? (i + 1) : remainder) - 1;

            // Ensure end does not exceed n-1
            if (end >= n) {
                end = n - 1;
            }

            // Store the partition
            partition_sizes[i] = end - start + 1; // Number of elements in this partition
            for (int j = start; j <= end; j++) {
                partitions[i][j - start] = data[j]; // Copy the tuple
            }
        }

        // Sort each partition
        for (int i = 0; i < num; i++) {
            bubbleSort(partitions[i], partition_sizes[i]);
        }

        // Merging sorted partitions
        Tuple merged_data[n]; // Array to hold the final merged data
        int merged_index = 0; // Current index in the merged data

        // Merging the sorted partitions using a multi-way merge algorithm
        int current_indices[num]; // Current index for each partition
        memset(current_indices, 0, sizeof(current_indices)); // Initialize to zero

        while (merged_index < n) {
            int min_index = -1;
            int min_value = INT_MAX;

            // Find the minimum tuple among the heads of each partition
            for (int i = 0; i < num; i++) {
                if (current_indices[i] < partition_sizes[i]) {
                    if (partitions[i][current_indices[i]].number < min_value) {
                        min_value = partitions[i][current_indices[i]].number;
                        min_index = i; // Keep track of which partition has the minimum value
                    }
                }
            }

            if (min_index != -1) {
                // Add the minimum tuple to the merged data
                merged_data[merged_index++] = partitions[min_index][current_indices[min_index]];
                current_indices[min_index]++; // Move to the next element in that partition
            } else {
                break; // No more elements to merge
            }
        }

        // Stop timer after merging
        clock_t end_time = clock();
        printf("Total time taken (sort and merge with %d processes): %f seconds\n", num, (double)(end_time - start_time) / CLOCKS_PER_SEC);


        // UNCOMMENT if you would like to view the final merged data for this partitioning.
        // printf("\nFinal Merged and Sorted Data for %d Processes:\n", num);
        // for (int i = 0; i < n; i++) {
        //     printf("Index %d: %s, %d\n", i, merged_data[i].text, merged_data[i].number);
        // }
    }

    return 0;
}