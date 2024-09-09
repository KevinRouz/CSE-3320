//Kevin Farokhrouz
//Ali Jifi-Bahlool
//CSE 3320-001
//Lab 1
//Implemented by Kevin: message, display_file(), change_directory(), remove_file(), call with argument
//Implemented by Ali: FileEnry Struct, display_time(), get_directory_contents(), sort_files(), process forking
//Implemented by both: display_contents(), edit_file(), main(), main switch logic, run_file()

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

#define MAX_FILES 1024 //max number of files in file_list array
#define BUFFER_SIZE 512 //max buffer size to temp store data

//declare global variable message, that can be used to display a message next time the screen is refreshed
//basically a global char array used to store messages after each operation
char message[BUFFER_SIZE] = "";

//struct used for file entries in cwd
typedef struct
{
  char name[BUFFER_SIZE];
  off_t size;
  time_t mtime; //last modified time of file
  mode_t mode;  //file type and permissions
} FileEntry;

//array of FileEntry structs that hold info about files and directories in the cwd
FileEntry file_list[MAX_FILES];
int file_count = 0; //number of files stored in file_list array
int current_start = 0; //controls which set of files is displayed

// retrieves and displays current time
void display_time()
{
  time_t t = time(NULL);
  if (t == -1)
  {
    perror("time error, could not retrieve time");
    return;
  }
//use of perror instead of printf to print error message, directly uses errno to print more accurate error messages
  struct tm *tm_info = localtime(&t); 
  if (tm_info == NULL)
  {
    perror("localtime, could not convert to local time");
    return;
  }
  //error creating time struct, ex: month, day, year

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
  DIR *d = opendir("."); //opens cwd, returns pointer to directory stream, a data structure used to represent a directory 
  if (d == NULL)
  {
    perror("opendir, could not open directory to read and display contents");
    return;
  }

  struct dirent *de; //pointer to hold directory entries already read by readdir
  file_count = 0; //number of files read, resets to zero every function call

  //set file info for each entry to a pointer containing the file info, continues until file_list reaches max size
  while ((de = readdir(d)) && file_count < MAX_FILES)
  {
    struct stat st; //struct variable to hold detailed info about each file
    if (stat(de->d_name, &st) == 0) //fill st struct with file info, if not 0 (error) skip
    {
      if (strcmp(de->d_name, ".") == 0) //skip cwd entry 
        continue;
      strncpy(file_list[file_count].name, de->d_name, BUFFER_SIZE); //copy cwd entry into name field, only use buffer_size to avoid buffer overflow
      file_list[file_count].size = st.st_size; //copy file size field
      file_list[file_count].mtime = st.st_mtime; //copy file modification time 
      file_list[file_count].mode = st.st_mode; //copy file mode field (file type, permissions)
      file_count++; //increment file count to move to next entry
    }
  }
  closedir(d); //close directory stream 
}

//sorting functions, to be used by qsort
//+ if first file is greater/larger/later
//= if files are equal in size, date, or name
//- if first file is lesser/smaller/earlier
int compare_by_name(const void *a, const void *b) //sort by name, compare two FileEntry structs by name fields
{
  return strcmp(((FileEntry *)a)->name, ((FileEntry *)b)->name);
}

int compare_by_size(const void *a, const void *b) //sort by size, compare two FileEntry structs by size fields
{
  return ((FileEntry *)a)->size - ((FileEntry *)b)->size;
}

int compare_by_date(const void *a, const void *b) //sort by date, compare two FileEntry structs by date fields
{
  return ((FileEntry *)a)->mtime - ((FileEntry *)b)->mtime;
}

void sort_files(int sort_option) //chooses comparsion function based on user input, "sort_option"
{
  switch (sort_option)
  {
    case 1:
      qsort(file_list, file_count, sizeof(FileEntry), compare_by_name); //compare alphabetically
      break;
    case 2:
      qsort(file_list, file_count, sizeof(FileEntry), compare_by_size); //compare size
      break;
    case 3:
      qsort(file_list, file_count, sizeof(FileEntry), compare_by_date); //compare modification time
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
  // //iterate through the file_list array and display file details, loop thru first 5 entries, increment or decrement current_start to display next or past 5 entries 
  for (int i = current_start; i < current_start + 5 && i < file_count; i++)
  {
    char *type = (S_ISDIR(file_list[i].mode)) ? "Directory" : "File"; //checks if directory or file, marco
    printf("%d. [%s] %s \tSize: %ld bytes, Date: %s", i, type, file_list[i].name, file_list[i].size, ctime(&file_list[i].mtime)); //displays index number, type, name, size, and date
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
         "Q  Quit                      \n\n"); //displays available commands to the user
}

//edit a file using the users preferred editor
void edit_file(char *filename)
{
  char *editor = getenv("EDITOR");
  if (editor == NULL)
  {
    editor = "nano"; //default to nano if EDITOR not set
  }

    char *args[] = {editor, filename, NULL}; //
    pid_t pid = fork(); //fork creates new process, for editing
    if (pid < 0) //if negative, error creating new process
    {
        perror("fork");
        snprintf(message, BUFFER_SIZE, "Something went wrong, could not edit %s.", filename);
        return; //return to parent
    } 
    else if (pid == 0) //if zero, error with execvp
    {
        execvp(editor, args);
        snprintf(message, BUFFER_SIZE, "Something went wrong, could not edit %s.", filename);
        perror("execvp"); //exec failed case
        exit(EXIT_FAILURE);
    } 
    else 
    {
        wait(NULL); //shell (parent) doesnt execute other commands while editing 
        snprintf(message, BUFFER_SIZE, "Successfully edited %s.", filename);
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

  //create fork to run executable
  pid_t pid = fork();

  //print error message if error, could not run executable
  if (pid < 0)
  {
    snprintf(message, BUFFER_SIZE, "FORK: Could not create process for %s.", filename);
    return;
  }
  else if (pid == 0)
  {
    //sets default action for SIGINT (terminate), in case it was set to signal_handler
    signal(SIGINT, SIG_DFL);

    //prepend "./" to the filename to indicate it is in the cwd
    char filepath[BUFFER_SIZE];
    snprintf(filepath, BUFFER_SIZE, "./%s", filename);

    execvp(filepath, args); //attempt to execute file

    //if execvp fails, format error message and exit
    snprintf(message, BUFFER_SIZE, "EXECVP: Could not execute %s.", filename);
    perror(message); //print error to stderr
    exit(EXIT_FAILURE);
  }
  else
  {
    signal(SIGINT, signal_handler); //ignore SIGINT in parent function

    int status;
    if (waitpid(pid, &status, 0) == -1) //wait for fork (child)
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

//removes file
void remove_file(const char *filename)
{

  FILE *file = fopen(filename, "w"); //open file
  if (remove(filename) == 0)
  {
    snprintf(message, BUFFER_SIZE, "File %s successfully deleted.\n", filename); //remove file
  }
  else
  {
    snprintf(message, BUFFER_SIZE, "Error deleting file %s.\n", filename); //error removing
  }
}

//displays file
void display_file(const char *filename)
{
  FILE *file = fopen(filename, "r"); //open file
  if (file == NULL)
  {
    printf("Unable to open file %s\n", filename); //error reading file
    return;
  }

  //read and print the file
  char ch;
  while ((ch = fgetc(file)) != EOF)
  {
    putchar(ch); //putchar instead of printf to print with standard output
  }
  printf("\n\n");
  //close the file
  fclose(file);
}

//change working directory
void change_directory(char *path)
{
  if (chdir(path) != 0) //change cwd to speficied directory
  {
    perror("chdir"); //print error message 
  }
}

int main(int argc, char **argv) 
{
    //if called with arguments, set the starting directory to argument
    if(argc == 2)
        change_directory(argv[1]);

    char cmd[BUFFER_SIZE];
    int ch;

  //create a list of the current directory's contents
  get_directory_contents();

  while (1)
  {
    //clear terminal
    printf("\033[H\033[J");

    //print cwd
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

        //print message from the previous command, then clear the message
        if(strlen(message) > 0){
            printf("%s\n\n", message);
            strcpy(message, "");
        }

        printf("What would you like to do? ");

        //get user input
        ch = getchar();
        while (getchar() != '\n'); //clear input buffer

    switch (ch)
    {
      case 'q': //quit
        printf("Exiting. Thank you for using this program.\n\n\nDeveloped by Kevin Farokhrouz and Ali Jifi-Bahlool\n\n");
        exit(0);
      case 'n': //next page
        //make sure not to go past the length of the file count
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
        //create a list of new cwd's contents
        get_directory_contents();
        //reset the start of the pagination since list is new
        current_start = 0;
        break;
      case 'd': //display file
        printf("Which file would you like to display? ");
        fgets(cmd, BUFFER_SIZE, stdin);
        cmd[strcspn(cmd, "\n")] = '\0';
        printf("File contents:\n\n");
        display_file(cmd);
        printf("Press Enter to continue. ");
        ch = getchar();
        if(ch != '\n') //ensures enter doesn't need to be pressed twice
          while (getchar() != '\n'); //clears stdin
        break;
      case 'v': //remove file
        printf("Which file would you like to remove? ");
        fgets(cmd, BUFFER_SIZE, stdin);
        cmd[strcspn(cmd, "\n")] = '\0';
        remove_file(cmd);
        //create a list of the directory contents, user removed a file
        get_directory_contents();
        //reset the start of the pagination because of created new list
        current_start = 0;
        break;
      default:
        strcpy(message, "Invalid command!");
        break;
      }
  }
  return 0;
}
