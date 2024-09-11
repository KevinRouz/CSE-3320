//Kevin Farokhrouz
//Ali Jifi-Bahlool
//CSE 3320-001
//Lab 1
//Implemented by Kevin: message, display_file(), change_directory(), remove_file(), call with argument
//Implemented by Ali: FileEnry Struct, display_time(), get_directory_contents(), sort_files(), process forking
//Implemented by both: display_contents(), edit_file(), main(), main switch logic, run_file()

//Sources:
//https://brennan.io/2015/01/16/write-a-shell-in-c/
//https://dev.to/wahomethegeek/building-your-own-shell-in-c-a-journey-into-command-line-magic-2lon
//https://stackoverflow.com/questions/4788374/writing-a-basic-shell

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

#define MAX_FILES 1024 //max number of files in fileList array
#define BUFFER_SIZE 512 //max buffer size to temp store data

//declare global variable message, that can be used to display a message next time the screen is refreshed
//basically a global char array used to store messages after each operation
char message[BUFFER_SIZE] = "";

//struct used for file entries in cwd
typedef struct
{
  char name[BUFFER_SIZE];
  off_t size;
  time_t modTime; //last modified time of file
  mode_t mode;  //file type and permissions
} FileEntry;

//array of FileEntry structs that hold info about files and directories in the cwd
FileEntry fileList[MAX_FILES];
int fileCount = 0; //number of files stored in fileList array
int currentStart = 0; //controls which set of files is displayed

// retrieves and displays current time
void displayTime()
{
  time_t t = time(NULL);
  if (t == -1)
  {
    perror("time error, could not retrieve time");
    return;
  }
//use of perror instead of printf to print error message, directly uses errno to print more accurate error messages
  struct tm *timeInfo = localtime(&t); 
  if (timeInfo == NULL)
  {
    perror("localtime, could not convert to local time");
    return;
  }
  //error creating time struct, ex: month, day, year

  //clean up time display format
  char timeBuffer[BUFFER_SIZE];
  if (strftime(timeBuffer, sizeof(timeBuffer), "%d %B %Y, %I:%M %p", timeInfo) == 0) //
  {
    fprintf(stderr, "Error formatting time\n");
    return;
  }
  printf("It is currently %s\n", timeBuffer);
}

//list directory contents
void getDirectoryContents()
{
  DIR *d = opendir("."); //opens cwd, returns pointer to directory stream, a data structure used to represent a directory 
  if (d == NULL)
  {
    perror("opendir, could not open directory to read and display contents");
    return;
  }

  struct dirent *directoryEntry; //pointer to hold directory entries already read by readdir
  fileCount = 0; //number of files read, resets to zero every function call

  //set file info for each entry to a pointer containing the file info, continues until fileList reaches max size
  while ((directoryEntry = readdir(d)) && fileCount < MAX_FILES)
  {
    struct stat st; //struct variable to hold detailed info about each file
    if (stat(directoryEntry->d_name, &st) == 0) //fill st struct with file info, if not 0 (error) skip
    {
      if (strcmp(directoryEntry->d_name, ".") == 0) //skip cwd entry 
        continue;
      strncpy(fileList[fileCount].name, directoryEntry->d_name, BUFFER_SIZE); //copy cwd entry into name field, only use buffer_size to avoid buffer overflow
      fileList[fileCount].size = st.st_size; //copy file size field
      fileList[fileCount].modTime = st.st_mtime; //copy file modification time 
      fileList[fileCount].mode = st.st_mode; //copy file mode field (file type, permissions)
      fileCount++; //increment file count to move to next entry
    }
  }
  closedir(d); //close directory stream 
}

//sorting functions, to be used by qsort
//+ if first file is greater/larger/later
//= if files are equal in size, date, or name
//- if first file is lesser/smaller/earlier
int sortName(const void *a, const void *b) //sort by name, compare two FileEntry structs by name fields
{
  return strcmp(((FileEntry *)a)->name, ((FileEntry *)b)->name);
}

int sortSize(const void *a, const void *b) //sort by size, compare two FileEntry structs by size fields
{
  return ((FileEntry *)a)->size - ((FileEntry *)b)->size;
}

int sortDate(const void *a, const void *b) //sort by date, compare two FileEntry structs by date fields
{
  return ((FileEntry *)a)->modTime - ((FileEntry *)b)->modTime;
}

void sortFiles(int sort_option) //chooses comparsion function based on user input, "sort_option"
{
  switch (sort_option)
  {
    case 1:
      qsort(fileList, fileCount, sizeof(FileEntry), sortName); //sort alphabetically
      break;
    case 2:
      qsort(fileList, fileCount, sizeof(FileEntry), sortSize); //sort size
      break;
    case 3:
      qsort(fileList, fileCount, sizeof(FileEntry), sortDate); //sort modification time
      break;
    default:
      printf("Invalid sorting option!\n"); 
      break;
  }
}

//display files with pagination
void displayContents()
{
  printf("Current Directory Contents:\n\n");
  // //iterate through the fileList array and display file details, loop thru first 5 entries, increment or decrement currentStart to display next or past 5 entries 
  for (int i = currentStart; i < currentStart + 5 && i < fileCount; i++)
  {
    char *type = (S_ISDIR(fileList[i].mode)) ? "Directory" : "File"; //checks if directory or file, macro
    printf("%d. [%s] %s \tSize: %ld bytes, Date: %s", i, type, fileList[i].name, fileList[i].size, ctime(&fileList[i].modTime)); //displays index number, type, name, size, and date
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
void editFile(char *filename)
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
void signalHandler(int sig)
{
  //empty signal handler to ignore SIGINT
}

//run an executable
void runFile(char *filename)
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
    signal(SIGINT, signalHandler); //ignore SIGINT in parent function

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
void removeFile(const char *filename)
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
void displayFile(const char *filename)
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
void changeDirectory(char *path)
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
        changeDirectory(argv[1]);

    char cmd[BUFFER_SIZE];
    int ch;

  //create a list of the current directory's contents
  getDirectoryContents();

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
    displayTime();
    printf("---\n");

    //display the contents of the cwd with pagination
    displayContents();

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
        if (currentStart + 5 < fileCount)
        {
          currentStart += 5;
        }
        else
        { //otherwise prevent user from going below zero, already at the last page
          strcpy(message, "No more pages.");
        } 
      break;
      case 'b': //previous page
        //make sure not to go below zero
        if (currentStart - 5 >= 0)
        {
          currentStart -= 5;
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
            sortFiles(sort_option); //sort file list based on chosen option
            currentStart = 0;       //reset the start of page list
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
        editFile(cmd);
        //create a list of the directory contents- important if the user just created a new file.
        getDirectoryContents();
        //reset the start of the pagination since we created a new list
        currentStart = 0;
        break;
      case 'r': //run
        printf("Which file would you like to run? ");
        fgets(cmd, BUFFER_SIZE, stdin);
        cmd[strcspn(cmd, "\n")] = '\0'; //remove newline character
        runFile(cmd);
        //create a list of the directory contents- important if the user just created a new file.
        getDirectoryContents();
        //reset the start of the pagination since we created a new list
        currentStart = 0;
        printf("MESSAGE after run: %s", message);
        break;
      case 'm': //move to directory
      case 'c': //change directory
        printf("Which directory would you like to move to? ");
        fgets(cmd, BUFFER_SIZE, stdin);
        cmd[strcspn(cmd, "\n")] = '\0'; //remove newline character
        changeDirectory(cmd);
        //create a list of new cwd's contents
        getDirectoryContents();
        //reset the start of the pagination since list is new
        currentStart = 0;
        break;
      case 'd': //display file
        printf("Which file would you like to display? ");
        fgets(cmd, BUFFER_SIZE, stdin);
        cmd[strcspn(cmd, "\n")] = '\0';
        printf("File contents:\n\n");
        displayFile(cmd);
        printf("Press Enter to continue. ");
        ch = getchar();
        if(ch != '\n') //ensures enter doesn't need to be pressed twice
          while (getchar() != '\n'); //clears stdin
        break;
      case 'v': //remove file
        printf("Which file would you like to remove? ");
        fgets(cmd, BUFFER_SIZE, stdin);
        cmd[strcspn(cmd, "\n")] = '\0';
        removeFile(cmd);
        //create a list of the directory contents, user removed a file
        getDirectoryContents();
        //reset the start of the pagination because of created new list
        currentStart = 0;
        break;
      default:
        strcpy(message, "Invalid command!");
        break;
      }
  }
  return 0;
}
