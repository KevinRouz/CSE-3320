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

int main(void) {
    pid_t child;
    DIR * d;
    struct dirent * de;
    int i, c, k;
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
      "R  Remove File             \n\t\t"
      "Q  Quit                      \n\n"
      ); 
 
      
      
  
      c = getchar( );
      switch (tolower(c)) {
        case 'd':
                  printf( "Which file would you like to display?");
                  scanf("%s", s);
                  strcpy(cmd, "cat ");
                  strcat(cmd, s);
                  system(cmd);


                  break;
        case 'e': printf( "Edit what?:" );
                  scanf( "%s", s );
                  strcpy( cmd, "pico ");
                  strcat( cmd, s );
                  system( cmd );
                  break;
        case 'r': printf( "Run what?:" );
                  scanf( "%s", cmd );
                  system( cmd );
                  break;
        case 'c': printf( "Change To?:" );
                  scanf( "%s", cmd );
                  chdir( cmd );   
                  break; 
        case 'q': 
                  printf("Exiting. Thank you for using this program.\n\n\nDeveloped by Kevin Farokhrouz and Ali Jifi-Bahlool\n\n");
                  exit(0); /* quit */
      }
       
    }
}
