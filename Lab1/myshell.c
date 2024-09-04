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
#include <ctype.h>
#include <sys/wait.h>


int main(void) {
    pid_t child;
    DIR * d;
    struct dirent * de;
    int i, c, k, errorcode;
    char s[256], cmd[256];
    time_t t;
    struct tm *tm_info;
    char buffer[160];


    //clear terminal
    system("clear"); 
    while (1) {
      

      //print cwd
      if (getcwd(s, sizeof(s)) == NULL) {
            perror("getcwd");
            exit(EXIT_FAILURE);
      }
      printf("\n\n---\n");
      printf( "\nCurrent Directory: %s \n\n", s);


      //get current time
      t = time(NULL);
      if (t == -1) {
        perror("time");
        exit(EXIT_FAILURE);
      }

      // Convert time to struct tm
      tm_info = localtime(&t);
      if (tm_info == NULL) {
        perror("localtime");
        exit(EXIT_FAILURE);
      } 

      // Format the time
      if (strftime(buffer, sizeof(buffer), "%d %B %Y, %I:%M %p", tm_info) == 0) {
        fprintf(stderr, "strftime returned 0");
        exit(EXIT_FAILURE);
      }

      // Print the formatted time
      printf("It is currently %s\n\n", buffer);

      printf( "\n" );

      //open directory
      d = opendir( "." );
      if (d == NULL) {
            perror("opendir");
            exit(EXIT_FAILURE);
      }

      //print files
      //TODO: Implement the pages for viewing files and directories.
      c = 0;                    
      printf("Files:\n");
      while ((de = readdir(d))){                    
          if (((de->d_type) & DT_REG))                              
             printf( "\t\t%d. %s\n", c++, de->d_name);
          if ( ( c % 5 ) == 0 ) {
             printf( "Hit N for Next\n" );
             k = getchar( );
             }
      }
      closedir( d );
      printf( "\n" );


      d = opendir( "." );
      //print directories
      c = 0;
      printf("Directories:\n");
      while ((de = readdir(d))){
          if ((de->d_type) & DT_DIR){ 
            //skip "." (cwd)
            if (strcmp(de->d_name, ".") == 0)
              continue;
            printf( "\t\t%d. %s\n", c++, de->d_name);	  
          }
      }
      closedir( d );
      printf("\n");
      

      //print operations
      printf("Operation:          \n\t\t"
      "D  Display                 \n\t\t"
      "E  Edit                    \n\t\t"
      "R  Run                     \n\t\t"
      "C  Change Directory        \n\t\t"
      "S  Sort Directory Listing  \n\t\t"
      "M  Move to Directory       \n\t\t"
      "V  Remove File             \n\t\t"
      "Q  Quit                      \n\n"
      ); 
      printf("\n");
      
      

      c = getchar( ); 
      getchar();
      printf("\n");
      switch (tolower(c)) {
        case 'd':
                  printf("Which file would you like to display?\n\n");
                  scanf("%s", s);
                  getchar();
                  strcpy(cmd, "cat ");
                  strcat(cmd, s);
                  printf("\nFile contents:\n\n");
                  errorcode = system(cmd);
                  if(errorcode != 0)
                    printf("Ensure file %s exists. Typo?", s);
                  break;
        case 'e': printf("What file would you like to edit?\n\n");
                  scanf("%s", s);
                  getchar();
                  strcpy( cmd, "nano ");
                  strcat( cmd, s );
                  errorcode = system( cmd );
                  if(errorcode != 0)
                    printf("Something went wrong while editing.");
                  else
                    printf("Edited file %s successfully.", s);
                  
                  break;
        case 'r': printf("Which file would you like to run?\n\n");
                  scanf( "%s", s );
                  getchar();
                  strcpy(cmd, "./");
                  strcat(cmd, s);
                  errorcode = system(cmd);
                  if (WIFSIGNALED(errorcode) && WTERMSIG(errorcode) == SIGINT) {
                      printf("\n\nProgram %s was interrupted by Ctrl+C.\n", s);
                  } else if (errorcode != 0) {
                      printf("Could not run %s. Ensure the file is executable.\n", s);
                  } else {
                      printf("Ran file %s successfully.\n", s);
                  }
                  break;
        case 'c': printf( "Change To?:" );
                  scanf( "%s", cmd );
                  getchar();
                  chdir( cmd );   
                  break; 
        case 's':
                  //TODO: sort directory listing
                  break;
        case 'm':
                  //TODO: move to directory
                  break;
        case 'v':
                  //TODO: remove file
                  break;
        case 'q': 
                  printf("Exiting. Thank you for using this program.\n\n\nDeveloped by Kevin Farokhrouz and Ali Jifi-Bahlool\n\n");
                  exit(0); /* quit */
        default:
                  printf("Didn't recognize that command. Try again.\n");
                  break;
      }
       
    }
}
