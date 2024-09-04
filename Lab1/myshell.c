//Kevin Farokhrouz
//Ali Jifi-Bahlool
//CSE 3320-001
//Lab 1

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <sys/wait.h>

#define MAX_FILES 1024
#define BUFFER_SIZE 256

typedef struct 
{
    char name[BUFFER_SIZE];
    off_t size;
    time_t mtime;
    mode_t mode;
} FileEntry;

FileEntry file_list[MAX_FILES];
int file_count = 0;
int current_start = 0;

//display current time
void display_time() 
{
    time_t t = time(NULL);
    if (t == -1) 
    {
        perror("time");
        return;
    }

    struct tm *tm_info = localtime(&t);
    if (tm_info == NULL) 
    {
        perror("localtime");
        return;
    }

    char buffer[BUFFER_SIZE];
    if (strftime(buffer, sizeof(buffer), "%d %B %Y, %I:%M %p", tm_info) == 0) 
    {
        fprintf(stderr, "Error formatting time\n");
        return;
    }

    printf("It is currently %s\n", buffer);
}

//list directory contents
void list_directory_contents() 
{
    DIR *d = opendir(".");
    if (d == NULL) 
    {
        perror("opendir");
        return;
    }

    struct dirent *de;
    file_count = 0;

    while ((de = readdir(d)) && file_count < MAX_FILES) 
    {
        struct stat st;
        if (stat(de->d_name, &st) == 0) 
        {
            strncpy(file_list[file_count].name, de->d_name, BUFFER_SIZE);
            file_list[file_count].size = st.st_size;
            file_list[file_count].mtime = st.st_mtime;
            file_list[file_count].mode = st.st_mode;
            file_count++;
        }
    }
    closedir(d);
}

//sorting functions
int compare_by_name(const void *a, const void *b) 
{
    return strcmp(((FileEntry *)a)->name, ((FileEntry *)b)->name);
}

int compare_by_size(const void *a, const void *b) 
{
    return ((FileEntry *)a)->size - ((FileEntry *)b)->size;
}

int compare_by_date(const void *a, const void *b) 
{
    return ((FileEntry *)a)->mtime - ((FileEntry *)b)->mtime;
}

void sort_files(int sort_option) 
{
    switch (sort_option) 
    {
        case 1:
            qsort(file_list, file_count, sizeof(FileEntry), compare_by_name);
            break;
        case 2:
            qsort(file_list, file_count, sizeof(FileEntry), compare_by_size);
            break;
        case 3:
            qsort(file_list, file_count, sizeof(FileEntry), compare_by_date);
            break;
        default:
            printf("Invalid sorting option!\n");
            break;
    }
}

//display files with pagination
void display_files() 
{
    printf("Current Directory Contents:\n");
    for (int i = current_start; i < current_start + 5 && i < file_count; i++) 
    {
        char type = (S_ISDIR(file_list[i].mode)) ? 'D' : 'F';
        printf("%d. [%c] %s (Size: %ld bytes, Date: %s)\n", i, type, file_list[i].name, file_list[i].size, ctime(&file_list[i].mtime));
    }
    printf("\n(N)ext, (P)revious, (Q)uit, (S)ort [1-Name, 2-Size, 3-Date]: ");
}

//edit a file using the users preferred editor
void edit_file(char *filename) 
{
    char *editor = getenv("EDITOR");
    if (editor == NULL) 
    {
        editor = "nano"; //default to nano if EDITOR not set
    }

    char *args[] = {editor, filename, NULL};
    pid_t pid = fork();
    if (pid < 0) 
    {
        perror("fork");
        return;
    } 
    else if (pid == 0) 
    {
        execvp(editor, args);
        perror("execvp"); //exec failed case
        exit(EXIT_FAILURE);
    } 
    else 
    {
        wait(NULL);
    }
}

//run an executable
void run_file(char *filename) 
{
    char *args[] = {filename, NULL};
    pid_t pid = fork();
    if (pid < 0) 
    {
        perror("fork");
        return;
    } 
    else if (pid == 0) 
    {
        execvp(filename, args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } 
    else 
    {
        wait(NULL);
    }
}

//change working directory
void change_directory(char *path) 
{
    if (chdir(path) != 0) 
    {
        perror("chdir");
    }
}

int main(void) 
{
    char cmd[BUFFER_SIZE];
    int ch;

    while (1) 
    {
        //clear terminal
        printf("\033[H\033[J");

        //print current working directory
        if (getcwd(cmd, sizeof(cmd)) == NULL) 
        {
            perror("getcwd");
            continue;
        }
        printf("Current Directory: %s\n", cmd);

        //display current time
        display_time();
        printf("-----------------------------------------------\n");

        //list directory contents and display with pagination
        list_directory_contents();
        display_files();

        //get user input
        ch = getchar();
        while (getchar() != '\n'); //clear input buffer

        switch (ch) 
        {
            case 'q': //quit
                exit(0);
            case 'n': //next page
                if (current_start + 5 < file_count) current_start += 5;
                break;
            case 'p': //previous page
                if (current_start - 5 >= 0) current_start -= 5;
                break;
            case 's': //sort options
                printf("\nEnter sorting option (1-Name, 2-Size, 3-Date): ");
                int sort_option = getchar() - '0';
                while (getchar() != '\n'); //clear input buffer
                sort_files(sort_option);
                break;
            case 'e': //edit
                printf("Edit which file?: ");
                fgets(cmd, BUFFER_SIZE, stdin);
                cmd[strcspn(cmd, "\n")] = '\0'; //remove newline character
                edit_file(cmd);
                break;
            case 'r': //run
                printf("Run which file?: ");
                fgets(cmd, BUFFER_SIZE, stdin);
                cmd[strcspn(cmd, "\n")] = '\0'; //remove newline character
                run_file(cmd);
                break;
            case 'c': //change directory
                printf("Change to directory: ");
                fgets(cmd, BUFFER_SIZE, stdin);
                cmd[strcspn(cmd, "\n")] = '\0'; //remove newline character
                change_directory(cmd);
                break;
            default:
                printf("Invalid command!\n");
                break;
        }
    }

    return 0;
}
