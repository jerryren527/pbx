/*
 * TU: simulates a "telephone unit", which interfaces a client with the PBX.
 */
#include <stdlib.h>

#include "pbx.h"
#include "debug.h"
#include "csapp.h"

struct tu_node {
    int ref_cnt;
    int connfd;             // The connected descriptor associated with this TU structure.
    char *state;            // The state associated with this TU structure.
    TU *peer;               // The TU structure's peer.
};

typedef struct tu {
    struct tu_node *head;
    sem_t tu_lock;
    int tu_read_cnt;
    sem_t tu_read_cnt_mutex;
} TU;    

/* This function adds one reader into the critial section of the shared resource, a linked list of tu_node
structures. The first reader into the critical section will lock it, preventing any writer from entering while 
still allowing other readers to enter.
*/

void tu_reader_enters(TU *tu) {
    P(&(tu->tu_read_cnt_mutex));    // Reader wants enters the CS of tu->head.
    tu->tu_read_cnt += 1;           // Number of readers trying to enter CS of tu->head increased by 1.
    if (tu->tu_read_cnt == 1)
    {
        P(&(tu->tu_lock));          // If the first reader, lock CS of tu->head from any writer. Readers can still access it.
    }
    V(&(tu->tu_read_cnt_mutex));    // Readers can still access CS of tu->head.
}

/*
This function removed one reader from critical section of the shared resource, a linked list of tu_node
structures. The last reader that leaves the critical section will unlock it, allowing any writer to enter.
*/ 
void tu_reader_leaves(TU *tu) {
    P(&(tu->tu_read_cnt_mutex));        // Reader is trying to leave the CS of tu->head.
    tu->tu_read_cnt -= 1;               // One fewer reader.
    if (tu->tu_read_cnt == 0)
    {
        V(&(tu->tu_lock));              // If the last reader, unlock the CS of tu->head. Writers can now write to CS of tu->head.
    }
    V(&(tu->tu_read_cnt_mutex));        // Reader is trying to leave the CS of tu->head.
}

/*
 * Initialize a TU
 *
 * @param fd  The file descriptor of the underlying network connection.
 * @return  The TU, newly initialized and in the TU_ON_HOOK state, if initialization
 * was successful, otherwise NULL.
 */
#if 1
TU *tu_init(int fd) {

    debug("Inside tu_init(). Initializing tu with fd %d.\n", fd);
    TU *tu;                                             // Declare TU pointer.
    if ((tu = malloc(sizeof(struct tu))) == NULL)       // Dynamically allocate memory for the new TU. Check for Error and return NULL.
    {
        debug("Error calling malloc(). Returning NULL.\n"); 
        return NULL;
    }

    Sem_init(&(tu->tu_lock), 0 ,1);
    Sem_init(&(tu->tu_read_cnt_mutex), 0 ,1);

    P(&tu->tu_lock);    // Writer enters CS of tu->head.
    if ((tu->head = malloc(sizeof(struct tu_node))) == NULL)
    {
        debug("Error mallocing tu->head. Returning NULL.\n");
        return NULL;
    }
    // Write initial values of tu->head.
    tu->head->ref_cnt = 0;
    tu->head->connfd = fd;
    tu->head->state = tu_state_names[TU_ON_HOOK];
    tu->head->peer = NULL;

    // Write value of tu->tu_read_cnt to 0.
    P(&tu->tu_read_cnt_mutex);
    tu->tu_read_cnt = 0;
    V(&tu->tu_read_cnt_mutex);
    
    V(&tu->tu_lock);    // Writer releases the lock. Writers are allowed in if there are no readers requesting.
    
    return tu;                                          // Return the newly initialized TU structure.
}
#endif

/*
 * Increment the reference count on a TU.
 *
 * @param tu  The TU whose reference count is to be incremented
 * @param reason  A string describing the reason why the count is being incremented
 * (for debugging purposes).
 */
#if 1
void tu_ref(TU *tu, char *reason) {
    debug("Inside tu_ref().\n");
    if (reason)                                 // Print 'reason' if defined.
        debug("Reason: %s\n", reason);
    
    P(&(tu->tu_lock));                      // Writer entered CS of tu->head.
    tu->head->ref_cnt += 1;                              // Increment the reference count on 'tu'.
    debug("tu->ref now: %d\n", tu->head->ref_cnt);
    V(&(tu->tu_lock));                      // Writer leaves CS of tu->head.
    return;
}
#endif

/*
 * Decrement the reference count on a TU, freeing it if the count becomes 0.
 *
 * @param tu  The TU whose reference count is to be decremented
 * @param reason  A string describing the reason why the count is being decremented
 * (for debugging purposes).
 */
#if 1
void tu_unref(TU *tu, char *reason) {
    debug("Inside tu_unref().\n");
    if (reason)                                     // Print reason if defined.
        debug("Reason: %s\n", reason);
    
    P(&(tu->tu_lock));                      // Writer entered CS of tu->head.
    tu->head->ref_cnt -= 1;                              // Decrement the reference count on 'tu'.
    debug("tu->ref now: %d\n", tu->head->ref_cnt);
    V(&(tu->tu_lock));  
    return;
}
#endif

/*
 * Get the file descriptor for the network connection underlying a TU.
 * This file descriptor should only be used by a server to read input from
 * the connection.  Output to the connection must only be performed within
 * the PBX functions.
 *
 * @param tu
 * @return the underlying file descriptor, if any, otherwise -1.
 */
#if 1
int tu_fileno(TU *tu) {
    debug("Inside tu_fileno().\n");
    
    tu_reader_enters(tu);

    // Reader reads tu->head.
    int fileno = tu->head->connfd;                        // Save the value of tu->connfd in a variable.

    tu_reader_leaves(tu);
    return fileno;                                  // Return the value of the file descriptor. Will return -1 if tu->connfd == -1.
}
#endif

/*
 * Get the extension number for a TU.
 * This extension number is assigned by the PBX when a TU is registered
 * and it is used to identify a particular TU in calls to tu_dial().
 * The value returned might be the same as the value returned by tu_fileno(),
 * but is not necessarily so.
 *
 * @param tu
 * @return the extension number, if any, otherwise -1.
 */
#if 1
int tu_extension(TU *tu) {
    debug("Inside tu_extension().\n");
    tu_reader_enters(tu);

    // Reader reads tu->head.
    int extension = tu->head->connfd;                     // Save the value of tu->connfd in a variable.

    tu_reader_leaves(tu);
    return extension;                               // Return the value of the extension. Will return -1 if tu->connfd == -1.
}
#endif

/*
 * Set the extension number for a TU.
 * A notification is set to the client of the TU.
 * This function should be called at most once one any particular TU.
 *
 * @param tu  The TU whose extension is being set.
 */
#if 1
int tu_set_extension(TU *tu, int ext) {
    debug("Inside tu_set_extension().\n");

    P(&(tu->tu_lock));
    tu->head->connfd = ext;                 // Update 'tu->connfd' to 'ext'.
    V(&(tu->tu_lock));

    if (ext == -1)                                   // If ext is -1, then do nothing.
    {
        debug("tu->connfd is -1. Return 0 here.\n");
        return 0;
    }

    char *bp;                                               // Declare a char buffer pointer.
    size_t size;                                            // Declare size location.
    FILE *stream;                                           // Declare a FILE pointer.
    stream = open_memstream(&bp, &size);
    fprintf(stream, "%s", tu_state_names[TU_ON_HOOK]);      // Place characters into the stream.
    fflush(stream);
    fprintf(stream, " %d\n", ext);                          // Place more characters into the stream.
    fclose(stream);                                         // Close the stream. Closing the stream frees the dynamic buffer.
    Rio_writen(ext, bp, size);                       // Write those characters to tu's connected descriptor.
    free(bp);
    return 0;
}
#endif

/*
 * @param tu  The originating TU.
 * @param target  The target TU, or NULL if the caller of this function was unable to
 * identify a TU to be dialed.
 * @return 0 if successful, -1 if any error occurs that results in the originating
 * TU transitioning to the TU_ERROR state. 
 */
#if 1
int tu_dial(TU *tu, TU *target) {
    char *bp;                                       // Declare a char buffer pointer.
    size_t size;                                    // Declare size location.
    FILE *stream;                                   // Declare a FILE pointer.
    stream = open_memstream(&bp, &size);

    tu_reader_enters(tu);

    if (!target)                                                        // If NULL was passed in as argument for 'target',
    {
        if ((strcmp(tu->head->state, tu_state_names[TU_DIAL_TONE]) == 0))     // If target is NULL and the originating TU is in DIAL_TONE state,
        {
            debug("Transitioning to ERROR state. And returning -1.\n");
            printf("Transitioning to ERROR state. And returning -1.\n");
            
            int fileno_tu = tu->head->connfd;

            tu_reader_leaves(tu);

            P(&(tu->tu_lock));                      // Writer entered CS of tu->head.
            tu->head->state = tu_state_names[TU_ERROR];                       // Then the originating TU will transition to the ERROR state.
            V(&(tu->tu_lock));                      // Writer leaves CS of tu->head.

            fprintf(stream, "%s\n", tu_state_names[TU_ERROR]);                         // Place characters into the stream.
            fclose(stream);                                             // Close the stream. Closing the stream frees the dynamic buffer.
            Rio_writen(fileno_tu, bp, size);                        // Write characters in 'stream' to tu's connected descriptor.
            free(bp);
            return -1;
        }

        if(strcmp(tu->head->state, tu_state_names[TU_DIAL_TONE]) != 0)        // If target is NULL and 'tu' is not in DIAL_TONE state, then no effect.
        {
            debug("Target's state is not DIAL TONE, there will be no effect. Returning 0.\n");
            printf("Target's state is not DIAL TONE, there will be no effect. Returning 0.\n");
            fprintf(stream, "%s", tu->head->state);                           // Place characters into the stream.
            fflush(stream);

            int fileno_tu = tu->head->connfd;
            int fileno_peer_tu;
            char *peer_tu_state;
            (void) fileno_peer_tu;
            (void) peer_tu_state;
            if (tu->head->peer)
            {
                fileno_peer_tu = tu->head->peer->head->connfd;
                peer_tu_state = strdup(tu->head->peer->head->state);
            }
            char *tu_state = strdup(tu->head->state);
            
            tu_reader_leaves(tu);

            if (strcmp(tu_state, tu_state_names[TU_CONNECTED]) == 0)       // If tu is in CONNECTED state,
                fprintf(stream, " %d\n", fileno_peer_tu);               // Print notification.
            else if (strcmp(tu_state, tu_state_names[TU_ON_HOOK]) == 0)    // If tu in in ON HOOK state
                fprintf(stream, " %d\n", fileno_tu);                    // Print notification.
            else
                fprintf(stream, "\n");                                      // Print notification.
            fclose(stream);                                                 // Close the stream. Closing the stream frees the dynamic buffer.
            Rio_writen(fileno_tu, bp, size);                            // Write characters in 'stream' to tu's connected descriptor.
            free(bp);
            free(tu_state);
            if (tu->head->peer)
                free(peer_tu_state);

            return 0;
        }
        else
        {
            debug("Control should not reach here.\n");
            printf("Control should not reach here.\n");
            tu_reader_leaves(tu);
            return -1;
        }
    }
    if (target)                                                             // If 'target' is not NULL,
    {

        if (strcmp(tu->head->state, tu_state_names[TU_DIAL_TONE]) != 0)           // If 'target' is not NULL and if tu is not in DIAL TONE state, then no effect.
        {
            debug("Origination TU is not in DIAL TONE state. There is no effect. Returning 0.\n");
            printf("Origination TU is not in DIAL TONE state. There is no effect. Returning 0.\n");
            fprintf(stream, "%s", tu->head->state);                               // Place characters into the stream.
            fflush(stream);                                                 // Flush the stream.

            int fileno_tu = tu->head->connfd;
            int fileno_peer_tu;
            char *peer_tu_state;
            (void) fileno_peer_tu;
            (void) peer_tu_state;
            if (tu->head->peer)
            {
                fileno_peer_tu = tu->head->peer->head->connfd;
                peer_tu_state = strdup(tu->head->peer->head->state);
            }
            char *tu_state = strdup(tu->head->state);

            tu_reader_leaves(tu);
            
            if (strcmp(tu_state, tu_state_names[TU_CONNECTED]) == 0)       // If tu is in CONNECTED state, print notif.
                fprintf(stream, " %d\n", fileno_peer_tu);
            else if (strcmp(tu_state, tu_state_names[TU_ON_HOOK]) == 0)    // If tu in in ON HOOK state, print notif.
                fprintf(stream, " %d\n", fileno_tu);
            else
                fprintf(stream, "\n");                                      // Print notif.

            fclose(stream);                                                 // Close the stream. Closing the stream frees the dynamic buffer.
            Rio_writen(tu_fileno(tu), bp, size);                            // Write characters in 'stream' to tu's connected descriptor.
            free(bp);
            free(tu_state);
            if (tu->head->peer)
                free(peer_tu_state);

            return 0;
        }
        
        if (target == tu)                                                   // It target is defined, and if 'target' is the same as 'tu', then 'tu' transitions to BUSY_SIGNAL state.
        {
            debug("Target TU and origination TU are the same. Origination TU transitioning to BUSY SIGNAL.\n");
            printf("Target TU and origination TU are the same. Origination TU transitioning to BUSY SIGNAL.\n");

            int fileno_tu = tu->head->connfd;

            tu_reader_leaves(tu);
            
            P(&tu->tu_lock);                                    // Writer tries to enter CS of tu->head.
            tu->head->state = tu_state_names[TU_BUSY_SIGNAL];         // Writer enters CS, writes to tu->head.
            V(&tu->tu_lock);                                    // Writer leaves CS.
            
            fprintf(stream, "%s\n", tu_state_names[TU_BUSY_SIGNAL]);                 // Place characters into the stream.
            fclose(stream);                                     // Close the stream. Closing the stream frees the dynamic buffer.
            Rio_writen(fileno_tu, bp, size);                // Write characters in 'stream' to tu's connected descriptor.
            free(bp);

            return 0;
        }
        
        tu_reader_enters(target);
        
        if ((target->head->peer) || (strcmp(target->head->state, tu_state_names[TU_ON_HOOK]) != 0)) // If target TU already has a peer or if target TU  is not in the ON HOOK state, then originating TU transitions to BUSY SIGNAL state.
        {
            debug("Target TU already has a peer, or target TU is not on ON HOOK state. Originating TU transitioning to BUSY SIGNAL state.\n");
            printf("Target TU already has a peer, or target TU is not on ON HOOK state. Originating TU transitioning to BUSY SIGNAL state.\n");

            int fileno_tu = tu->head->connfd;

            tu_reader_leaves(target);
            
            tu_reader_leaves(tu);
            
            P(&tu->tu_lock);                            // Writer trying to enter CS of tu->head.
            tu->head->state = tu_state_names[TU_BUSY_SIGNAL];     // Change origin tu's state to BUSY SIGNAL.
            V(&tu->tu_lock);                            // Writer leaves CS of tu->head.
            
            fprintf(stream, "%s\n", tu_state_names[TU_BUSY_SIGNAL]);             // Place characters into the stream.
            fclose(stream);                                 // Close the stream. Closing the stream frees the dynamic buffer.
            Rio_writen(fileno_tu, bp, size);            // Write characters in 'stream' to tu's connected descriptor.
            free(bp);

            return 0;
        }

        if ((!target->head->peer) && (strcmp(target->head->state, tu_state_names[TU_ON_HOOK]) == 0))    // If target tu does not have a peer and it is in ON HOOK state, then connect origination tu and target tu and make them peers.
        {
            debug("Target TU does not have a peer and it is in ON HOOK state. Recording originating TU and target TU as peers.\n");
            printf("Target TU does not have a peer and it is in ON HOOK state. Recording originating TU and target TU as peers.\n");

            int fileno_tu = tu->head->connfd;
            int fileno_target = target->head->connfd;

            tu_reader_leaves(target);
            
            tu_reader_leaves(tu);
            
            // Write to tu->head and target->head simultaneously.
            if (tu < target)    // If tu is a lower address than target, lock in this particular order.
            {
                debug("tu is a lower address than target. Locking tu and target state mutexes in order 1.\n");
                P(&(tu->tu_lock));          // Writer is trying to enter CS of tu->head.
                P(&(target->tu_lock));      // Writer is trying to enter CS of target->head.
            }
            else
            {
                debug("tu is a greater address than target. Locking tu and target state mutexes in order 2.\n");
                P(&(target->tu_lock));  // Writer is trying to enter CS of target->head.
                P(&(tu->tu_lock));  // Writer is trying to enter CS of tu->head.
            }
            
            // P(&tu->state_mutex);                            // Protect the update of `tu->state` by surrounding it with P and V operations.
            tu->head->state = tu_state_names[TU_RING_BACK];       // Writer writes in CS of tu->head. Assign tu's state to RING BACK.
            target->head->state = tu_state_names[TU_RINGING];   // Writer writes in CS of target->head. Assign target's state to RINGING.
            tu->head->peer = target;        // Writer writes in CS of tu->head. Assign tu->head->peer.
            target->head->peer = tu;    // Writer writes in CS of target->head. Assign target->head->peer.
            
            V(&(target->tu_lock));      // Writer leaves CS of target->head.
            V(&(tu->tu_lock));          // Writer leaves CS of tu->head.

            fprintf(stream, "%s\n",  tu_state_names[TU_RING_BACK]);             // Place characters into the stream.
            fclose(stream);
            Rio_writen(fileno_tu, bp, size);            // Write characters in 'stream' to tu's connected descriptor.
            free(bp);
            
            char *bp2;                                              // Declare a char buffer pointer to be used in another stream.
            size_t size2;                                           // Declare size location.
            FILE *stream2;                                          // Declare a FILE pointer.
            stream2 = open_memstream(&bp2, &size2);                 // Create another stream.
            fprintf(stream2, "%s\n", tu_state_names[TU_RINGING]);                // Place characters into the stream.
            fclose(stream2);
            Rio_writen(fileno_target, bp2, size2);      // Write characters in 'stream2' to targets's connected descriptor.
            free(bp2);
            
            return 0;
        }

        debug("No conditions met in tu_dial. Returning -1.\n");
        printf("No conditions met in tu_dial. Returning -1.\n");

        tu_reader_leaves(tu);
        tu_reader_leaves(target);
        return -1;
    }
    
    debug("No conditions met in tu_dial. Returning -1.\n");
    tu_reader_leaves(tu);
    return -1;
}
#endif

/*
 * Take a TU receiver off-hook (i.e. pick up the handset).
 *   If the TU is in neither the TU_ON_HOOK state nor the TU_RINGING state,
 *     then there is no effect.
 *   If the TU is in the TU_ON_HOOK state, it goes to the TU_DIAL_TONE state.
 *   If the TU was in the TU_RINGING state, it goes to the TU_CONNECTED state,
 *     reflecting an answered call.  In this case, the calling TU simultaneously
 *     also transitions to the TU_CONNECTED state.
 *
 * In all cases, a notification of the resulting state of the specified TU is sent to
 * to the associated network client.  If a peer TU has changed state, then its client
 * is also notified of its new state.
 *
 * @param tu  The TU that is to be picked up.
 * @return 0 if successful, -1 if any error occurs that results in the originating
 * TU transitioning to the TU_ERROR state. 
 */
#if 1
int tu_pickup(TU *tu) {
    char *bp;                                       // Declare a char buffer pointer.
    size_t size;                                    // Declare size location.
    FILE *stream;                                   // Declare a FILE pointer.
    stream = open_memstream(&bp, &size);

    tu_reader_enters(tu);
    
    if ((strcmp(tu->head->state, tu_state_names[TU_ON_HOOK]) != 0) && (strcmp(tu->head->state, tu_state_names[TU_RINGING]) != 0))
    {
        debug("TU is not in ON HOOK or RINGING state. No effect. Returning 0.\n");
        fprintf(stream, "%s", tu->head->state);    // Place characters into the stream.
        fflush(stream);
        
        if (strcmp(tu->head->state, tu_state_names[TU_CONNECTED]) == 0)       // If tu is in CONNECTED state, print notif.
            fprintf(stream, " %d\n", tu_fileno(tu->head->peer));
        else if (strcmp(tu->head->state, tu_state_names[TU_ON_HOOK]) == 0)    // If tu in in ON HOOK state, print notif.
            fprintf(stream, " %d\n", tu_fileno(tu));
        else
            fprintf(stream, "\n");                                      // Print notif.

        fclose(stream);                                                 // Close the stream. Closing the stream frees the dynamic buffer.
        Rio_writen(tu_fileno(tu), bp, size);                            // Write characters in 'stream' to tu's connected descriptor.
        free(bp);

        tu_reader_leaves(tu);
        return 0;
    }
    
    if (strcmp(tu->head->state, tu_state_names[TU_ON_HOOK]) == 0)
    {
        debug("TU is in ON HOOK state. TU will transition to DIAL TONE state. Returning 0.\n");

        tu_reader_leaves(tu);
        
        P(&tu->tu_lock);        // Writer enteres CS of tu->head.
        tu->head->state = tu_state_names[TU_DIAL_TONE];       // Writer writes to CS of tu->head. tu will transition to DIAL TONE state.
        V(&tu->tu_lock);        // Writer leaves CS of tu->head.

        fprintf(stream, "%s\n", tu_state_names[TU_DIAL_TONE]);             // Place characters into the stream.
        fclose(stream);
        Rio_writen(tu_fileno(tu), bp, size);            // Write characters in 'stream' to tu's connected descriptor.
        free(bp);
        return 0;
    }
    
    if (strcmp(tu->head->state, tu_state_names[TU_RINGING]) == 0)
    {
        debug("Tu is in RINGING state. Tu will transition to CONNECTED state.");
        
        tu_reader_leaves(tu);
        
        if (tu < tu->head->peer)    // If tu is a lower address than its peer, lock in this particular order.
        {
            debug("tu is a lower address than tu->peer. Locking tu and target state mutexes in order 1.\n");
            P(&(tu->tu_lock));              // Writer tries to enter CS of tu->head.
            P(&(tu->head->peer->tu_lock));  // Writer tries to enter CS of tu_peer->head.
        }
        else
        {
            debug("tu is a greater address than tu->peer. Locking tu and target state mutexes in order 2.\n");
            P(&(tu->head->peer->tu_lock));  // Writer tries to enter CS of tu_peer->head.
            P(&(tu->tu_lock));  // Writer tries to enter CS of tu->head.
        }


        tu->head->state = tu_state_names[TU_CONNECTED];               // Change tu's state to CONNECTED.
        tu->head->peer->head->state = tu_state_names[TU_CONNECTED];          // Change peer_tu's state to CONNECTED.
        V(&tu->head->peer->tu_lock);                                     // Writer leaves Cs of tu->head.
        V(&tu->tu_lock);                                     // Writer leaves Cs of tu_peer->head.

        fprintf(stream, "%s", tu_state_names[TU_CONNECTED]);                       // Print tu's notification.
        fflush(stream);
        fprintf(stream, " %d\n", tu_fileno(tu->head->peer));
        fclose(stream);
        Rio_writen(tu_fileno(tu), bp, size);                    // Write characters in 'stream' to tu's connected descriptor.
        free(bp);

        char *bp2;                                              // Declare a char buffer pointer to be used in another stream.
        size_t size2;                                           // Declare size location.
        FILE *stream2;                                          // Declare a FILE pointer.
        stream2 = open_memstream(&bp2, &size2);                 // Create another stream.
        fprintf(stream2, "%s", tu_state_names[TU_CONNECTED]);                      // Print peer_tu's notification.
        fflush(stream2);
        fprintf(stream2, " %d\n", tu_fileno(tu));                  
        fclose(stream2);
        Rio_writen(tu_fileno(tu->head->peer), bp2, size2);             // Write characters in 'stream2' to peer_tu's connected descriptor.
        free(bp2);

        tu_ref(tu, "tu_pickup");                                // Increment tu's reference count.
        tu_ref(tu->head->peer, "tu_pickup");                           // Increment peer_tu's reference count.
        
        return 0;
    }
    tu_reader_leaves(tu);
    
    debug("No conditions met in tu_pickup(). Returning -1.\n");
    return -1;
    // abort();
}
#endif

/*
 * @param tu  The tu that is to be hung up.
 * @return 0 if successful, -1 if any error occurs that results in the originating
 * TU transitioning to the TU_ERROR state. 
 */
#if 1
int tu_hangup(TU *tu) {
    debug("Inside tu_hangup().\n");

    tu_reader_enters(tu);

    if (tu->head->connfd == -1)
    {
        debug("Tu's connfd is -1. Going to unref tu's peer if it has one and set 'peer' to NULL.\n");

        if (tu->head->peer)
        {
            debug("Tu has a peer. Going to transition the peer tu's state to DIAL TONE, unreference peer_tu, and set 'peer' to NULL for both.\n");

            int tu_peer_fileno = tu->head->peer->head->connfd;
            
            tu_reader_leaves(tu);
            
            tu_unref(tu->head->peer, "Peer tu disconnected.");     // Decrement peer_tu's reference count.
            tu_unref(tu, "Tu has disconnected.");           // Decrement tu's reference count.

            if (tu < tu->head->peer)    // If tu is a lower address than its peer, lock in this particular order.
            {
                debug("tu is a lower address than tu->peer. Locking tu and target peer mutexes in order 1.\n");
                P(&(tu->tu_lock));              // Writer tries to enter CS of tu->head.
                P(&(tu->head->peer->tu_lock));  // Writer tries to enter CS of tu_PEER->head.
            }
            else
            {
                debug("tu is a greater address than tu->peer. Locking tu and target peer mutexes in order 2.\n");
                P(&(tu->head->peer->tu_lock));  // Writer tries to enter CS of tu_peer->head.
                P(&(tu->tu_lock));  // Writer tries to enter CS of tu->head.
            }

            tu->head->peer->head->state = tu_state_names[TU_DIAL_TONE];  // Writer writes in CS of tu_peer->head. Changing peer tu's state to DIAL TONE state.
            

            tu->head->peer->head->peer = NULL;      // Writer writes in Cs of tu_peer->head. Assigns its peer value to NULL.
            V(&(tu->head->peer->tu_lock));  // Writer leaves CS of tu_PEER->head.
            tu->head->peer = NULL;                // Writer writes in Cs of tu->head. Assigns its peer value to NULL.
            V(&(tu->tu_lock));              // Writer leaves CS of tu->head.
            
            char *bp;                                       // Declare a char buffer pointer.
            size_t size;                                    // Declare size location.
            FILE *stream;                                   // Declare a FILE pointer.
            stream = open_memstream(&bp, &size);

            fprintf(stream, "%s\n", tu_state_names[TU_DIAL_TONE]);        // Print the notification.
            fclose(stream);
            Rio_writen(tu_peer_fileno, bp, size);            // Write the notification to peer_tu's connfd.
            free(bp);
            free(tu->head);

            return 0;
        }
        else
        {
            tu_reader_leaves(tu);

            debug("Tu did not have a peer. Going to unref tu.");
            tu_unref(tu, "Tu has disconnected");
            free(tu->head);
            return 0;
        }
    }

    char *bp;                                       // Declare a char buffer pointer.
    size_t size;                                    // Declare size location.
    FILE *stream;                                   // Declare a FILE pointer.
    stream = open_memstream(&bp, &size);

    if ((strcmp(tu->head->state, tu_state_names[TU_CONNECTED]) == 0) ||
        (strcmp(tu->head->state, tu_state_names[TU_RINGING]) == 0))
    {
        debug("Tu is in the CONNECTED or RINGING state. Tu is transitioning to ON HOOK state. The peer tu simulatenously transition to DIAL TONE. Returning 0.\n");
        
        int fileno_tu = tu->head->connfd;
        int fileno_peer_tu = tu->head->peer->head->connfd;

        tu_reader_leaves(tu);
        
        if (strcmp(tu->head->state, tu_state_names[TU_CONNECTED]) == 0)   // If tu is in CONNECTED state,
        {
            tu_unref(tu, "tu_hangup");                          // Decrement tu reference count.
            tu_unref(tu->head->peer, "tu_hangup");                     // Decrement peer_tu reference count.
        }

        if (tu < tu->head->peer)    // If tu is a lower address than its peer, lock in this particular order.
        {
            debug("tu is a lower address than tu->peer. Locking tu and target peer mutexes in order 1.\n");
            P(&(tu->tu_lock));              // Writer tries to enter CS of tu->head.
            P(&(tu->head->peer->tu_lock));  // Writer tries to enter CS of tu_PEER->head.
        }
        else
        {
            debug("tu is a greater address than tu->peer. Locking tu and target peer mutexes in order 2.\n");
            P(&(tu->head->peer->tu_lock));  // Writer tries to enter CS of tu_peer->head.
            P(&(tu->tu_lock));  // Writer tries to enter CS of tu->head.
        }

        tu->head->state = tu_state_names[TU_ON_HOOK];                 // Change tu's state to ON HOOK.
        tu->head->peer->head->state = tu_state_names[TU_DIAL_TONE];          // Change peer_tu's state to DIAL TONE.
        tu->head->peer->head->peer = NULL;
        V(&(tu->head->peer->tu_lock));  // Writer tries to leave CS of tu_PEER->head.
        tu->head->peer = NULL;
        V(&(tu->tu_lock));              // Writer tries to leave CS of tu->head.

        fprintf(stream, "%s", tu_state_names[TU_ON_HOOK]);                       // Place characters into the stream.
        fflush(stream);
        fprintf(stream, " %d\n", fileno_tu);            // Add descriptor number to the notification.
        fclose(stream);
        Rio_writen(fileno_tu, bp, size);                    // Write characters in 'stream' to tu's connected descriptor.
        free(bp);

        char *bp2;                                              // Declare a char buffer pointer to be used in another stream.
        size_t size2;                                           // Declare size location.
        FILE *stream2;                                          // Declare a FILE pointer.
        stream2 = open_memstream(&bp2, &size2);                 // Create another stream.
        fprintf(stream2, "%s\n", tu_state_names[TU_DIAL_TONE]);                 // Place characters into the stream.
        fclose(stream2);
        Rio_writen(fileno_peer_tu, bp2, size2);             // Write characters in 'stream2' to targets's connected descriptor.
        free(bp2);

        return 0;
    }

    if (strcmp(tu->head->state, tu_state_names[TU_RING_BACK]) == 0)
    {
        debug("Tu is in RING BACK state. Tu is transitioning to ON HOOK state. The peer tu simultaneously transitions from RINGING to ON HOOK state. Returning 0.\n");
        
        int fileno_tu = tu->head->connfd;
        int fileno_peer_tu = tu->head->peer->head->connfd;

        tu_reader_leaves(tu);
        
        if (tu < tu->head->peer)    // If tu is a lower address than its peer, lock in this particular order.
        {
            debug("tu is a lower address than tu->peer. Locking tu and target peer mutexes in order 1.\n");
            P(&(tu->tu_lock));              // Writer tries to enter CS of tu->head.
            P(&(tu->head->peer->tu_lock));  // Writer tries to enter CS of tu_PEER->head.
        }
        else
        {
            debug("tu is a greater address than tu->peer. Locking tu and target peer mutexes in order 2.\n");
            P(&(tu->head->peer->tu_lock));  // Writer tries to enter CS of tu_peer->head.
            P(&(tu->tu_lock));  // Writer tries to enter CS of tu->head.
        }

        tu->head->state = tu_state_names[TU_ON_HOOK];         // Change the tu->state to ON HOOK.
        tu->head->peer->head->state = tu_state_names[TU_ON_HOOK];    // Change the peer_tu->state to ON HOOK.

        V(&(tu->head->peer->tu_lock));  // Writer tries to enter CS of tu_PEER->head.
        V(&(tu->tu_lock));              // Writer tries to enter CS of tu->head.

        fprintf(stream, "%s", tu_state_names[TU_ON_HOOK]);              // Place characters into the stream.
        fflush(stream);
        fprintf(stream, " %d\n", fileno_tu);       // Place more characters into the stream.
        fclose(stream);
        Rio_writen(fileno_tu, bp, size);            // Write characters in 'stream' to tu's connected descriptor.
        free(bp);

        char *bp2;                                              // Declare a char buffer pointer to be used in another stream.
        size_t size2;                                           // Declare size location.
        FILE *stream2;                                          // Declare a FILE pointer.
        stream2 = open_memstream(&bp2, &size2);                 // Create another stream.
        fprintf(stream2, "%s", tu_state_names[TU_ON_HOOK]);                 // Place characters into the stream.
        fflush(stream2);
        fprintf(stream2, " %d\n", fileno_peer_tu);          // Place more characters into the stream.
        fclose(stream2);
        Rio_writen(fileno_peer_tu, bp2, size2);             // Write characters in 'stream2' to targets's connected descriptor.
        free(bp2);

        return 0;
    }

    if ((strcmp(tu->head->state, tu_state_names[TU_DIAL_TONE]) == 0) ||
        (strcmp(tu->head->state, tu_state_names[TU_BUSY_SIGNAL]) == 0) || 
        (strcmp(tu->head->state, tu_state_names[TU_ERROR]) == 0))
    {
        debug("Tu is in DIAL TONE, BUSY SIGNAL, or ERROR state. Tu is transitioning to ON HOOK state. Returning 0.\n");
        
        int fileno_tu = tu->head->connfd;

        tu_reader_leaves(tu);

        P(&tu->tu_lock);                    // Protect the update of `tu->state` by surrounding it with P and V operations.
        tu->head->state = tu_state_names[TU_ON_HOOK];
        V(&tu->tu_lock);                    // Protect the update of `tu->state` by surrounding it with P and V operations.

        fprintf(stream, "%s", tu_state_names[TU_ON_HOOK]);                  // Place characters into the stream.
        fflush(stream);
        fprintf(stream, " %d\n", fileno_tu);           // Place more characters into the stream.
        fclose(stream);
        Rio_writen(tu_fileno(tu), bp, size);                // Write characters in 'stream' to tu's connected descriptor.
        free(bp);

        return 0;
    }

    debug("No conditions met in tu_hangup(). Returning -1.\n");
    
    int fileno_tu = tu->head->connfd;
    char *tu_state = strdup(tu->head->state);
    
    tu_reader_leaves(tu);
    
    fprintf(stream, "%s", tu_state);                  // Place characters into the stream.
    fflush(stream);
    fprintf(stream, " %d\n", fileno_tu);           // Place more characters into the stream.
    fclose(stream);
    Rio_writen(fileno_tu, bp, size);                // Write characters in 'stream' to tu's connected descriptor.
    free(bp);
    free(tu_state);
    return -1;
    // abort();
}
#endif

/*
 * "Chat" over a connection.
 *
 * If the state of the TU is not TU_CONNECTED, then nothing is sent and -1 is returned.
 * Otherwise, the specified message is sent via the network connection to the peer TU.
 * In all cases, the states of the TUs are left unchanged and a notification containing
 * the current state is sent to the TU sending the chat.
 *
 * @param tu  The tu sending the chat.
 * @param msg  The message to be sent.
 * @return 0  If the chat was successfully sent, -1 if there is no call in progress
 * or some other error occurs.
 */
#if 1
int tu_chat(TU *tu, char *msg) {
    debug("Inside tu_chat().\n");
    char *bp;                                       // Declare a char buffer pointer.
    size_t size;                                    // Declare size location.
    FILE *stream;                                   // Declare a FILE pointer.
    stream = open_memstream(&bp, &size);

    tu_reader_enters(tu);

    int fileno_tu = tu->head->connfd;

    if (strcmp(tu->head->state, tu_state_names[TU_CONNECTED]) != 0)
    {
        debug("Tu is not in CONNECTED state. Nothing will happen. Return -1.\n");
        fprintf(stream, "%s", tu->head->state);    // Place characters into the stream.
        fflush(stream);
        
        if (strcmp(tu->head->state, tu_state_names[TU_ON_HOOK]) == 0)    // If tu in in ON HOOK state
            fprintf(stream, " %d\n", fileno_tu);
        else
            fprintf(stream, "\n");
        

        tu_reader_leaves(tu);

        fclose(stream);                                 // Close the stream. Closing the stream frees the dynamic buffer.
        Rio_writen(fileno_tu, bp, size); // Write characters in 'stream' to tu's connected descriptor.
        free(bp);
        return -1;
    }
    else
    {
        debug("Tu is in CONNECTED state. msg will be sent to peer tu. Returning 0.\n");

        int fileno_tu = tu->head->connfd;
        int fileno_peer_tu = tu->head->peer->head->connfd;
        char *tu_state = strdup(tu->head->state);
        
        tu_reader_leaves(tu);

        fprintf(stream, "%s", "CHAT");
        fflush(stream);
        fprintf(stream, " %s", msg);
        fclose(stream);
        Rio_writen(fileno_peer_tu, bp, size);
        free(bp);

        // Write notification to sending Tu.
        char *bp2;                                              // Declare a char buffer pointer to be used in another stream.
        size_t size2;                                           // Declare size location.
        FILE *stream2;                                          // Declare a FILE pointer.
        stream2 = open_memstream(&bp2, &size2);                 // Create another stream.

        fprintf(stream2, "%s", tu_state);                      // Place characters into the stream.
        fflush(stream2);
        fprintf(stream2, " %d\n", fileno_peer_tu);          // Place more characters into the stream.
        fclose(stream2);
        Rio_writen(fileno_tu, bp2, size2);                  // Write characters in 'stream' to tu's connected descriptor.
        free(bp2);
        
        free(tu_state);
        return 0;
    }
}
#endif
