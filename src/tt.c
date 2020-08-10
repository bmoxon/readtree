// tt.c

// threadtest to explore valgrind unclaimed resources

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

// unmark pthread_detach(), memory leak will be gone

void* process( void* data ) {
  printf( "hello world\n" );
  pthread_exit (NULL);
}

int main (int argc, char *argv[]) {
  pthread_t th;
  pthread_create( &th, NULL, process, NULL );
  sleep( 1 );
  // pthread_detach( th );
  pthread_join( th, NULL );
  return 0;
}

