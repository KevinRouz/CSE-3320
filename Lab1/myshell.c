/*  Some example code and prototype - 
    contains many, many problems: should check for return values 
    (especially system calls), handle errors, not use fixed paths,
    handle parameters, put comments, watch out for buffer overflows,
    security problems, use environment variables, etc.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <time.h>

int main(void) {
    pid_t child;
    DIR * d;
    struct dirent * de;
    int i, c, k;
    char s[256], cmd[256];
    time_t t;
    struct tm *tm_info;
    char buffer[160];

    while (1) {
      //clear terminal
      system("clear"); 

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
      printf("It is currently %s\n", buffer);
      printf("-----------------------------------------------\n" );

      d = opendir( "." );
      c = 0;
      while ((de = readdir(d))){
          if ((de->d_type) & DT_DIR) 
             printf( " ( %d Directory:  %s ) \n", c++, de->d_name);	  
      }
      closedir( d );
      printf( "-----------------------------------------\n" );
 
      d = opendir( "." );
      if (d == NULL) {
            perror("opendir");
            exit(EXIT_FAILURE);
        }
      c = 0;                    
      while ((de = readdir(d))){                    
          if (((de->d_type) & DT_REG))                              
             printf( " ( %d File:  %s ) \n", c++, de->d_name);
          if ( ( c % 5 ) == 0 ) {
             printf( "Hit N for Next\n" );
             k = getchar( );
             }
      }
      closedir( d );
      printf( "-----------------------------------------\n" );
  
      c = getchar( ); getchar( );
      switch (c) {
        case 'q': exit(0); /* quit */
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
      }
       
    }
}
