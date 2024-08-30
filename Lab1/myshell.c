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

    while (1) {

      t = time( NULL );
      printf( "Time: %s\n", ctime( &t ));
      printf("-----------------------------------------------\n" );

      getcwd(s, 200);
      printf( "\nCurrent Directory: %s \n", s);

      d = opendir( "." );
      c = 0;
      while ((de = readdir(d))){
          if ((de->d_type) & DT_DIR) 
             printf( " ( %d Directory:  %s ) \n", c++, de->d_name);	  
      }
      closedir( d );
      printf( "-----------------------------------------\n" );
 
      d = opendir( "." );
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
