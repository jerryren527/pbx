/*
 * "PBX" server module.
 * Manages interaction with a client telephone unit (TU).
 */
#include <stdlib.h>

#include "debug.h"
#include "pbx.h"
#include "server.h"
#include "csapp.h"

/*
 * Thread function for the thread that handles interaction with a client TU.
 * This is called after a network connection has been made via the main server
 * thread and a new thread has been created to handle the connection.
 */
#if 1
void *pbx_client_service(void *arg) {
    debug("Inside pbx_client_service().\n");
    TU *tu;                             // Declare a TU.
    size_t n;                           // Declare a variable to hold the number of bytes in the line that was ready by the read buffer, 'rio'.
    char buf[MAXLINE];                  // Initialize a char buffer, 'buf'.
    rio_t rio;                          // Initialize a read buffer, 'rio'.

    Pthread_detach(pthread_self());     // To avoid memory leaks in the thread routine, detach each thread so that its memory resources are reclaimed when it terminates.

    int connfd = *((int *) arg);        // Save the descriptor passed as argument to this function.
    Free(arg);                          // Free the storage occupied by the descriptor.
    tu = tu_init(connfd);               // Initialize a new TU with descriptor, connfd.
    pbx_register(pbx, tu, connfd);      // Register the new TU to pbx with a unique extension number. I made the extension number the value of 'connfd'.
    Rio_readinitb(&rio, connfd);        // Associate a descriptor, 'connfd', with a read buffer, 'rio', and reset its buffer.

    while(1)                            // Infinite service loop that reads client messages, parses, and calls functions.
    {
        while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0)        // Repeatedly read lines of text,
        {
            debug("buf: %s\n", buf);
            // char first_four[5];                                     // Declare a string, 'first_four', to contain the first 4 chars in char buffer, 'buf'.
            // memset(first_four, 0, sizeof(first_four));
            // strncpy(first_four, buf, 4);                            // Initialize 'first_four'
            char *token;    // Pointer to first token in 'buf'
            char *rest = buf;   // Pointer to first character in 'buf'

            
            while ((token = strtok_r(rest, " ", &rest)))
            {
                debug("token: %s\n", token);

                // Parse the contents of buf.
                if (strcmp(token, "pickup\r\n") == 0)                     // If client sends pickup mesage, call tu_pickup.
                {
                    debug("The client sent a pickup message.\n");
                    tu_pickup(tu);
                }
                else if ((strcmp(token, "hangup\r\n")) == 0)              // If client sends hangup message, call tu_hangup.
                {
                    debug("The client sent a hangup message.\n");
                    tu_hangup(tu);
                }
                else if (strcmp(token, "dial") == 0)               // If client sends dial message, call pbx_dial.
                {
                    debug("The client sent a dial message.\n");
                    // char *buf_p = buf;   
                    // buf_p = buf_p + 5;                                  // Points to where the number should be.
                    token = strtok_r(rest, " ", &rest);
                    int ext = atoi(token);
                    // (void) ext;
                    debug("%d\n", ext);
                    pbx_dial(pbx, tu, ext);
                }
                else if (strcmp(token, "chat") == 0)               // If client sends chat message, call tu_chat.
                {
                    debug("The client sent a chat message.\n");
                    char *buf_p = buf;   
                    buf_p = buf_p + 5;                                  // Points to where the number should be.
                    // token = strtok_r(rest, " ", &rest);
                    // int ext = atoi(token);
                    // (void) ext;
                    // debug("%d\n", ext);
                    tu_chat(tu, buf_p);
                }
                else
                {
                    debug("The client sent an unknown message.\n");     // Do nothing if client sends unknown message.
                }
            }

        }
        debug("Outside line reading loop.\n");                      // If client disconnects itself,
        pbx_unregister(pbx, tu);                                    // Unregister tu from pbx.
        Close(connfd);                                              // Close the connected descriptor because it is no longer needed.
        return NULL;
    }
    // abort();
}
#endif
