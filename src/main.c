#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>

#include "pbx.h"
#include "server.h"
#include "debug.h"
#include "csapp.h"

static void terminate(int status);

volatile sig_atomic_t hang_up = 0;

// SIGHUP handler
void sighup_handler(int s)
{
    debug("Inside sighup_handler\n");
    hang_up = 1;
    terminate(EXIT_FAILURE);
}


/*
 * "PBX" telephone exchange simulation.
 *
 * Usage: pbx <port>
 */
int main(int argc, char* argv[]){
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.
    int option;
    char *port;
    while ((option = getopt(argc, argv, "p:")) != -1)
    {
        switch(option)
        {
            case 'p':
                port = strdup(optarg);   // Dynamically allocate memory to hold the value of optarg in 'port'.
                break;
            case '?':
                if (optopt == 'p')      // Print error messages.
                    fprintf(stderr, "Option -p requires a port.\n");
                else
                    fprintf(stderr, "Unknown option character %c.\n", optopt);
                exit(EXIT_SUCCESS);
            default:
                debug("Error.\n");
                abort();
        }
    }
    if (optind == 1)                    // If no command line args provided, print usage message
    {
        fprintf(stderr, "Usage: pbx <port>.\n");
        exit(EXIT_SUCCESS);
    }
    debug("port: %s\n", port);

    // Perform required initialization of the PBX module.
    debug("Initializing PBX...");
    pbx = pbx_init();

    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function pbx_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.

    Signal(SIGHUP, sighup_handler);                 // Install SIGHUP handler.
    
    int listenfd, *connfdp;                         // Declare the listening descriptor and a pointer to a connected descriptor.
    socklen_t clientlen;                            // Declare an int variable to get assigned the sizeof(sockadd_storage).
    struct sockaddr_storage clientaddr;             // Declare a sockaddr_storage structure variable.
    pthread_t tid;                                  // Declare a long variable that will hold the thread ID of the created thread.

    listenfd = Open_listenfd(port);                 // Open a listening descriptor, 'listenfd', ready to receive connection requests.

    while (!hang_up) {                                                              // Infinite loop,
        clientlen=sizeof(struct sockaddr_storage);                                  // Assign the sizeof a sockaddr_storage struct to 'clientlen'. Used in accept().
        connfdp = Malloc(sizeof(int));                                              // We must dynamically allocate space for the connected descriptor returned by accept(). This is done to avoid a race between the assignment statement in the peer thread and the accept statement in the main thread.
        *connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen);                // The accept() function waits for a connection request to arrive on the listening descriptor, 'listenfd'. When it arrives, 'clientaddr' is filled with the client's socket address.
        int rc;
        if ((rc = pthread_create(&tid, NULL, pbx_client_service, connfdp)) != 0)    // Finally, `pthread_create` is called to create a thread with the id, 'tid'. The new thread will run the thread routine, 'pbx_client_server()' with the input arguments 'connfdp'.
        {
            posix_error(rc, "pthread_create error");                                // Error checking. 
            exit(EXIT_FAILURE);                                                     // Exit failure for now.
        }
    }
    // fprintf(stderr, "You have to finish implementing main() "
	    // "before the PBX server will function.\n");

    // terminate(EXIT_FAILURE);
}

/*
 * Function called to cleanly shut down the server.
 */
static void terminate(int status) {
    debug("Shutting down PBX...");
    pbx_shutdown(pbx);
    debug("PBX server terminating");
    exit(status);
}
