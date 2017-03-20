#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <errno.h>
#include <string.h>
#include <semaphore.h>

// Maximum number of words the server can store (capacity)
#define LIST_CAP 6

// Maximum length of a word on the server's list
#define WORD_MAX 20
#define MESSAGE_LIMIT 1024

sem_t* mySemaphore;
/** Structure containing our ordered list of words. */
typedef struct {
  // List of words maintained by the server.
  char list[ LIST_CAP ][ WORD_MAX + 1 ];

  // Add more fieds as needed.
  int wcount;
} Ladder;

// Print out an error message and exit.
static void fail( char const *message ) {
  fprintf( stderr, "%s\n", message );
  exit( 1 );
}
/**
*method for handling the up command
*@param message string to be dealt with in the list
*/
int moveUp(Ladder* sladder, char* message) {
  #ifndef UNSAFE
  sem_wait( mySemaphore );
  #endif
  //adds to the list when the list is empty
  if (sladder->wcount == 0) {
    strncpy( sladder->list[0], message, strlen(message) + 1 );
    //printf("adds to the empty list: %s\n", message);
    
    sladder->wcount++;
    #ifndef UNSAFE
    sem_post(mySemaphore);
    #endif
    return sladder->wcount - 1;
  } else {
    for( int i = 0; i < sladder->wcount; i++) {
      if ( strcmp(message, sladder->list[i]) == 0) {
        if (i == 0) {
          #ifndef UNSAFE
          sem_post(mySemaphore);
          #endif
          return 0;
        }
        char temp[MESSAGE_LIMIT];
        strncpy(temp, sladder->list[i-1], strlen(sladder->list[i-1]) + 1 );
        strncpy(sladder->list[i-1], message, strlen(message) + 1 );
        strncpy(sladder->list[i], temp, strlen(temp) + 1 );
        //printf("moved up one position: %s\n", message);
        #ifndef UNSAFE
        sem_post(mySemaphore);
        #endif
        return i - 1;
      }
    }
    if (sladder->wcount == 5) {
      strncpy(sladder->list[sladder->wcount], message, strlen(message) + 1 );
      #ifndef UNSAFE
      sem_post(mySemaphore);
      #endif
      return sladder->wcount;
    } else {
      //printf("added to the list: %s\n", message);
      //add to the list
      strncpy(sladder->list[sladder->wcount], message, strlen(message) + 1 );
      sladder->wcount++;
      #ifndef UNSAFE
      sem_post(mySemaphore);
      #endif
      return sladder->wcount - 1;
    }
  }
}
/**
*method for helping with the down command
*@param message for moving down in the list
*/
int moveDown(Ladder* sladder, char* message) {
  #ifndef UNSAFE
  sem_wait(mySemaphore);
  #endif
  for (int i = 0; i < sladder->wcount - 1; i++) {
    if ( strcmp(message, sladder->list[i]) == 0) {
      //printf("moved down in the list %s\n", message);
      char temp[MESSAGE_LIMIT];
      strncpy(temp, sladder->list[i+1], strlen(sladder->list[i+1]) + 1);
      //printf("What is temp? %s\n", temp);
      strncpy(sladder->list[i+1], message, strlen(message) + 1);
      strncpy(sladder->list[i], temp, strlen(temp) + 1 );
      //printf("temp again: %s\n", temp);
      #ifndef UNSAFE
      sem_post(mySemaphore);
      #endif
      return i + 1;
    }
  }
  if (strcmp(sladder->list[sladder->wcount - 1], message) == 0) {
    #ifndef UNSAFE
    sem_post(mySemaphore);
    #endif
    return sladder->wcount - 1;
  }
  if (sladder->wcount == 6) {
    strncpy(sladder->list[sladder->wcount], message, strlen(message) + 1);
    #ifndef UNSAFE
    sem_post(mySemaphore);
    #endif
    return sladder->wcount;
  } else if ( strcmp(message, sladder->list[sladder->wcount]) == 0) {
    strncpy(sladder->list[sladder->wcount - 1], message, strlen(message) + 1);
    #ifndef UNSAFE
    sem_post(mySemaphore);
    #endif
    return sladder->wcount;
  } else {
    #ifndef UNSAFE
    sem_post(mySemaphore);
    #endif
    return -1;
  }
}
/**
*Query method for return the contents of the ladder
*@param sladder a pointer to the Ladder object
*/
void query(Ladder* sladder) {
  #ifndef UNSAFE
  sem_wait(mySemaphore);
  #endif
  //we need to read the argument into the buffer now
  //for up/down
  for (int i = 0; i < sladder->wcount; i++) {
    printf("%s\n", sladder->list[i]);
  }
  if (sladder->wcount == 5) {
    printf("%s\n", sladder->list[5]);
  }
  #ifndef UNSAFE
  sem_post(mySemaphore);
  #endif
  return;
}
/**
*test method
*/
void test( Ladder *sladder, char *word, int n ) {
  for ( int i = 0; i < n; i++ ) {
    moveUp( sladder, word );
    moveDown( sladder, word );
  }
}
/**
* main method for reading input, making and adding to the list, moving contents up and
* down the list and printing out the list
*@param argc number of arguments
*@param argv command for ladder to compute
*@return int success of program
*/
int main( int argc, char *argv[] ) {
  mySemaphore = sem_open("/maskelto-ladder-lock", O_CREAT, 0666, 1);
  char com[ MESSAGE_LIMIT ];
  char argum[ MESSAGE_LIMIT ];
  strncpy(com, argv[1], strlen(argv[1]) + 1);
  int shmid = shmget( ftok( "/afs/unity/ncsu.edu/users/m/maskelto", 0 ), sizeof( Ladder ), 0666 | IPC_CREAT );
  
  if ( shmid == -1 ) {
    fail( "Can't create shared memory" );
  }
  Ladder *sladder = (Ladder *)shmat( shmid, 0, 0 );
  if ( sladder == (Ladder *)-1 ) {
    fail( "Can't map shared memory segment into address space" );
  }
  if (strcmp(com, "query") == 0) {
    query(sladder);
  } else if (strcmp(com, "test") == 0) {
    int third;
    strncpy(argum, argv[2], strlen(argv[2]) + 1);
    third = atoi(argv[3]);
    test(sladder, argum, third);
  } else {
    
    strncpy(argum, argv[2], strlen( argv[2] ) + 1);
    
    if (strcmp(com, "up")  == 0) {
      
      //get access to the memory
      
      int random;
      random = moveUp(sladder, argum);
      printf("%d\n", random);
      //write to the memory
      
    } else if (strcmp(com, "down") == 0) {
      //gain access to the shared memory
      
      int random;
      random = moveDown(sladder, argum);
      printf("%d\n", random);
      
    } else {
      fail("invalid command");
    }
  }
  shmdt(sladder);
  sem_close(mySemaphore);
  return EXIT_SUCCESS;
}