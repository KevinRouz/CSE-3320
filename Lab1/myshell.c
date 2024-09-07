// Kevin Farokhrouz
// Ali Jifi-Bahlool
// CSE 3320-001
// Lab 1

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
#include <signal.h>

#define MAX_FILES 1024
#define BUFFER_SIZE 512

//declare global variable message, that can be used to display a message next time the screen is refreshed
//basically a global char array used to store messages after each operation
char message[BUFFER_SIZE] = "";

//struct used for file entries
typedef struct
{
  char name[BUFFER_SIZE];
  off_t size;
  time_t mtime; //last modified time of file
  mode_t mode;  //file type and permissions
} FileEntry;

//array of FileEntry structs that hold info about files and directories in the cwd
FileEntry file_list[MAX_FILES];
int file_count = 0;
int current_start = 0; //controls which set of files is displayed

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

  //clean up time display format
  char buffer[BUFFER_SIZE];
  if (strftime(buffer, sizeof(buffer), "%d %B %Y, %I:%M %p", tm_info) == 0)
  {
    fprintf(stderr, "Error formatting time\n");
    return;
  }
  printf("It is currently %s\n", buffer);
}

//list directory contents
void get_directory_contents()
{
  DIR *d = opendir(".");
  if (d == NULL)
  {
    perror("opendir");
    return;
  }

  struct dirent *de;
  file_count = 0;

  //set file entry info for each entry
  while ((de = readdir(d)) && file_count < MAX_FILES)
  {
    struct stat st;
    if (stat(de->d_name, &st) == 0)
    {
      if (strcmp(de->d_name, ".") == 0)
        continue;
      strncpy(file_list[file_count].name, de->d_name, BUFFER_SIZE);
      file_list[file_count].size = st.st_size;
      file_list[file_count].mtime = st.st_mtime;
      file_list[file_count].mode = st.st_mode;
      file_count++;
    }
  }
  closedir(d);
}

//sorting functions, to be used by qsort
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
void display_contents()
{
  printf("Current Directory Contents:\n\n");
  for (int i = current_start; i < current_start + 5 && i < file_count; i++)
  {
    char *type = (S_ISDIR(file_list[i].mode)) ? "Directory" : "File";
    printf("%d. [%s] %s \tSize: %ld bytes, Date: %s", i, type, file_list[i].name, file_list[i].size, ctime(&file_list[i].mtime));
  }
  printf("\n\nOperation:          \n\t\t"
         "N  Next page               \n\t\t"
         "B  Previous page           \n\t\t"
         "D  Display                 \n\t\t"
         "E  Edit                    \n\t\t"
         "R  Run                     \n\t\t"
         "C  Change Directory        \n\t\t"
         "S  Sort Directory Listing  \n\t\t"
         "M  Move to Directory       \n\t\t"
         "V  Remove File             \n\t\t"
         "Q  Quit                      \n\n");
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
    perror("execvp");  //exec failed case
    exit(EXIT_FAILURE);
  }
  else
  {
    wait(NULL);
  }
}

//used during run_file, to capture ctrl c interrupt properly instead of printing generic failure
void signal_handler(int sig)
{
  //empty signal handler to ignore SIGINT
}

//run an executable
void run_file(char *filename)
{
  char *args[] = {filename, NULL};

  //create fork
  pid_t pid = fork();

  //print error message if error
  if (pid < 0)
  {
    snprintf(message, BUFFER_SIZE, "FORK: Could not create process for %s.", filename);
    return;
  }
  else if (pid == 0)
  {
    //sets default action for SIGINT (terminate), in case it was set to signal_handler
    signal(SIGINT, SIG_DFL);

    //prepend "./" to the filename to indicate it is in the current directory
    char filepath[BUFFER_SIZE];
    snprintf(filepath, BUFFER_SIZE, "./%s", filename);

    execvp(filepath, args);

    //if execvp fails, format error message and exit
    snprintf(message, BUFFER_SIZE, "EXECVP: Could not execute %s.", filename);
    perror(message); // print error to stderr
    exit(EXIT_FAILURE);
  }
  else
  {
    signal(SIGINT, signal_handler); //ignore SIGINT in else

    int status;
    if (waitpid(pid, &status, 0) == -1)
    {
      snprintf(message, BUFFER_SIZE, "WAITPID: Error waiting for %s.", filename);
      return;
    }

    //capture interrupt properly
    if (WIFSIGNALED(status))
    {
      if (WTERMSIG(status) == SIGINT)
      {
        snprintf(message, BUFFER_SIZE, "INTERRUPTED: %s was terminated by Ctrl+C.", filename);
      }
      else
      {
        snprintf(message, BUFFER_SIZE, "FAILURE: %s was terminated by signal %d.", filename, WTERMSIG(status));
      }
    } //otherwise capture a normal success or failure
    else if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
    {
      snprintf(message, BUFFER_SIZE, "SUCCESS: %s executed successfully.", filename);
    }
    else
    {
      snprintf(message, BUFFER_SIZE, "FAILURE: %s did not execute successfully.", filename);
    }

    //restore default handling of SIGINT
    signal(SIGINT, SIG_DFL);
  }
}

//removes a file
void remove_file(const char *filename)
{

  FILE *file = fopen(filename, "w");
  if (remove(filename) == 0)
  {
    snprintf(message, BUFFER_SIZE, "File %s successfully deleted.\n", filename);
  }
  else
  {
    snprintf(message, BUFFER_SIZE, "Error deleting file %s.\n", filename);
  }
}

//displays a file
void display_file(const char *filename)
{
  FILE *file = fopen(filename, "r");
  if (file == NULL)
  {
    printf("Unable to open file %s\n", filename);
    return;
  }

  //read and print the file
  char ch;
  while ((ch = fgetc(file)) != EOF)
  {
    putchar(ch);
  }
  printf("\n\n");
  //close the file
  fclose(file);
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

  //create a list of the current directory's contents
  get_directory_contents();

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
    printf("---\n");

    //display the contents of the cwd with pagination
    display_contents();

    //print message from the previous command, then clears the message.
    if (strlen(message) > 0)
    {
      printf("%s\n", message);
      strcpy(message, "");
    }

    //get user input
    ch = getchar();
    while (getchar() != '\n'); //clear input buffer

    switch (ch)
    {
      case 'q': //quit
        printf("Exiting. Thank you for using this program.\n\n\nDeveloped by Kevin Farokhrouz and Ali Jifi-Bahlool\n\n");
        exit(0);
      case 'n': //next page
        //make sure not to go past the length of the file count.
        if (current_start + 5 < file_count)
        {
          current_start += 5;
        }
        else
        { //otherwise prevent user from going below zero, already at the last page
          strcpy(message, "No more pages.");
        } 
      break;
      case 'b': //previous page
        //make sure not to go below zero
        if (current_start - 5 >= 0)
        {
          current_start -= 5;
        }
        else
        { //otherwise prevent user from going past the last page
          strcpy(message, "Already on the first page.");
        }
        break;
      case 's': //sort options
        while (1)
        {
          //loops until valid input
          printf("\nEnter sorting option (1-Name, 2-Size, 3-Date): ");
          int sort_option = getchar() - '0';
          while (getchar() != '\n'); //clear input buffer
          if (sort_option >= 1 && sort_option <= 3)
          {
            sort_files(sort_option); //sort file list based on chosen option
            current_start = 0;       //reset the start of page list
            break;
          }
          else
          {
            printf("Invalid sorting option! Please choose 1, 2, or 3.\n");
          }
        }
        break;
      case 'e': //edit
        printf("Which file would you like to edit? ");
        fgets(cmd, BUFFER_SIZE, stdin);
        cmd[strcspn(cmd, "\n")] = '\0'; //remove newline character
        edit_file(cmd);
        //create a list of the directory contents- important if the user just created a new file.
        get_directory_contents();
        //reset the start of the pagination since we created a new list
        current_start = 0;
        break;
      case 'r': //run
        printf("Which file would you like to run? ");
        fgets(cmd, BUFFER_SIZE, stdin);
        cmd[strcspn(cmd, "\n")] = '\0'; //remove newline character
        run_file(cmd);
        //create a list of the directory contents- important if the user just created a new file.
        get_directory_contents();
        //reset the start of the pagination since we created a new list
        current_start = 0;
        printf("MESSAGE after run: %s", message);
        break;
      case 'm': //move to directory
      case 'c': //change directory
        printf("Which directory would you like to move to? ");
        fgets(cmd, BUFFER_SIZE, stdin);
        cmd[strcspn(cmd, "\n")] = '\0'; //remove newline character
        change_directory(cmd);
        //create a list of the new cwd's contents
        get_directory_contents();
        //reset the start of the pagination since the list is new
        current_start = 0;
        break;
      case 'd': //display file
        printf("Which file would you like to display? ");
        fgets(cmd, BUFFER_SIZE, stdin);
        cmd[strcspn(cmd, "\n")] = '\0';
        printf("File contents:\n\n");
        display_file(cmd);
        printf("Press any key to continue. ");
        ch = getchar();
        while (getchar() != '\n');
        break;
      case 'v': //remove file
        printf("Which file would you like to remove? ");
        fgets(cmd, BUFFER_SIZE, stdin);
        cmd[strcspn(cmd, "\n")] = '\0';
        remove_file(cmd);
        //create a list of the directory contents since the user removed a file
        get_directory_contents();
        //reset the start of the pagination since we created a new list
        current_start = 0;
        break;
      default:
        strcpy(message, "Invalid command!");
        break;
      }
  }
  return 0;
}
