/*
 * PBX: simulates a Private Branch Exchange.
 */
#include <stdlib.h>
#include <sys/socket.h> // For shutdown(2)

#include "pbx.h"
#include "debug.h"
#include "csapp.h"

struct pbx_node {               // A pbx_node structure contains:
    TU *tu;                     // The TU structure associated with 'ext',
    int ext;                    // The 'ext' associated with the TU structure,
    struct pbx_node *next;      // Pointer to the next pbx structure in the linked list of pbx structures.
};

typedef struct pbx{             // A pbx structure contains:
    struct pbx_node *head;      // A head.
    int node_count;             // A counter for number of nodes.
    sem_t pbx_lock;             // Protect updates to pbx..
    sem_t node_count_mutex;     // Protects updates to node_count.
} PBX;

PBX *pbx;
int pbx_read_cnt = 0;       // Read count for pbx->head.
sem_t pbx_read_cnt_mutex;   // Mutex for pbx_read_cnt.


/*
 * Initialize a new PBX.
 *
 * @return the newly initialized PBX, or NULL if initialization fails.
 */
#if 1
PBX *pbx_init() {
    
    debug("Inside pbx_init().\n");
    debug("Initializing pbx instance.\n");
    
    if ((pbx = calloc(1, sizeof(struct pbx))) == NULL)         // Dynamically allocate space for pbx instance, initializing it to 0.
    {
        debug("Error calling calloc(). Returning NULL.\n");
        return NULL;
    }

    Sem_init(&(pbx->pbx_lock), 0, 1);     
    Sem_init(&(pbx->node_count_mutex), 0, 1);     
    Sem_init(&(pbx_read_cnt_mutex), 0, 1); 
    
    P(&(pbx->pbx_lock));            // Write to pbx->head.
    pbx->head = NULL;               // Initialize 'tu' member to NULL.
    V(&(pbx->pbx_lock));

    P(&(pbx->node_count_mutex));    // Write to pbx->node_count.
    pbx->node_count = 0;            // Initialize 'node_count' member to 0.
    V(&(pbx->node_count_mutex));

    return pbx;
}
#endif

/*
 * Shut down a pbx, shutting down all network connections, waiting for all server
 * threads to terminate, and freeing all associated resources.
 * If there are any registered extensions, the associated network connections are
 * shut down, which will cause the server threads to terminate.
 * Once all the server threads have terminated, any remaining resources associated
 * with the PBX are freed.  The PBX object itself is freed, and should not be used again.
 *
 * @param pbx  The PBX to be shut down.
 */
#if 1
void pbx_shutdown(PBX *pbx) {
    debug("Inside pbx_shutdown(). Shutting down all clients.\n");
    // P(&(shutdown_mutex));
    

    P(&(pbx->pbx_lock));                            // Writer tries to enter CS of pbx->head.
    struct pbx_node *curr_node = pbx->head;         // Declare and assign a pbx_node pointer to point to the first pbx_node structure in the linked list.

    // Free all tu's
    while (curr_node != NULL)
    {
        if (curr_node->ext != -1)                       // If the TU is registered,
        {
            debug("curr_node->ext is not -1.\n");
            tu_unref(curr_node->tu, "pbx_shutdown");    // Unregister a TU.

            int ret;
            if ((ret = shutdown(curr_node->ext, SHUT_RD)) == -1)    // Shutdown the connection of the TU's connected descriptor, which happens to be the same value as its 'ext' memnber. SHUT_RD means to stop receiving data for this stocket, and reject any further arriving data.
            {
                V(&(pbx->pbx_lock));
                debug("shutdown() error. Returning nothing.\n");    // Return -1 if shutdown() error.
                return;
            }

            tu_hangup(curr_node->tu);                               // Hangup the TU and free its resources.
            free(curr_node->tu);          // Free the 'tu' member of pbx[i].
            curr_node = curr_node->next;                            // Iterate to the next node in linked list.
        }
        else
        {
            debug("curr_node->ext IS -1. curr_node->tu already freed.\n");
            curr_node = curr_node->next;            // Iterate to the next node in linked list.
        }
    }
    // Free all pbx nodes.
    curr_node = pbx->head;
    while (curr_node != NULL)
    {
        struct pbx_node *tmp_node = curr_node;
        free(tmp_node);
        curr_node = curr_node->next;
    }
    V(&(pbx->pbx_lock));  // Protect the update to pbx->head.
    free(pbx);  // Free pbx itself.
    debug("Finished shutting down all clients.\n");
    return;
}
#endif

/*
 * Register a telephone unit with a PBX at a specified extension number.
 * This amounts to "plugging a telephone unit into the PBX".
 * The TU is initialized to the TU_ON_HOOK state.
 * The reference count of the TU is increased and the PBX retains this reference
 *for as long as the TU remains registered.
 * A notification of the assigned extension number is sent to the underlying network
 * client.
 *
 * @param pbx  The PBX registry.
 * @param tu  The TU to be registered.
 * @param ext  The extension number on which the TU is to be registered.
 * @return 0 if registration succeeds, otherwise -1.
 */

void reader_enters(sem_t *mutex, PBX *pbx) {

    P(mutex);
    pbx_read_cnt += 1;              // The number of readers has increased by 1.
    if (pbx_read_cnt == 1)
    {
        P(&(pbx->pbx_lock));        // Ensure no writer can enter critial section of pbx->head if there is even one reader.
    }
    V(mutex);       // Other readers can enter critical section of pbx_head.
}

void reader_leaves(sem_t *mutex, PBX *pbx) {
    P(mutex);   // Reader wants to leave critical section.
    pbx_read_cnt -= 1;          // A reader wants to leave.
    if (pbx_read_cnt == 0)      // If no more readers left in critical section of pbx->head.
    {
        V(&(pbx->pbx_lock));    // Writers can enter the critial section of pbx->head.
    }
    V(mutex);   // Reader leaves the critial section of pbx->head.
}


#if 1
int pbx_register(PBX *pbx, TU *tu, int ext) {
    debug("Inside pbx_register().\n");

    struct pbx_node *new_node;  // Declare a new pbx_node.
    if ((new_node = calloc(1, sizeof(struct pbx_node))) == NULL)  // Dynamically allocate 1 pbx_node structure.
    {
        debug("Error calling calloc(). Returning -1.\n");
        return -1;
    }

    tu_ref(tu, "pbx_register");     // Increment tu->ref_cnt.
    tu_set_extension(tu, ext);      // Assign 'ext' value to tu->connfd.
    new_node->tu = tu;
    new_node->ext = ext;
    new_node->next = NULL;

    reader_enters(&pbx_read_cnt_mutex, pbx);
    
    // Reading pbx->head;
    struct pbx_node *curr_node = pbx->head; // Assign pbx->head value to a local pbx_node pointer variable.

    if (pbx->head == NULL)  // If pbx is empty.
    {
        reader_leaves(&pbx_read_cnt_mutex, pbx);

        // P(&(pbx_read_cnt_mutex));   // Reader wants to leave critical section.
        // pbx_read_cnt -= 1;          // A reader wants to leave.
        // if (pbx_read_cnt == 0)      // If no more readers left in critical section of pbx->head.
        // {
        //     V(&(pbx->pbx_lock));    // Writers can enter the critial section of pbx->head.
        // }
        // V(&(pbx_read_cnt_mutex));   // Reader leaves the critial section of pbx->head.

        // Writing to pbx->head.
        P(&(pbx->pbx_lock));    // Enters critical section of pbx->head.
        pbx->head = new_node;   // Writer performs the write.
        V(&(pbx->pbx_lock));    // Leaves critical section of pbx->head.

        P(&(pbx->node_count_mutex));
        pbx->node_count += 1;       // Increment node count.
        V(&(pbx->node_count_mutex));

        return 0;
    }

    while (curr_node->next != NULL)
    {
        curr_node = curr_node->next;
    }

    reader_leaves(&pbx_read_cnt_mutex, pbx);
    
    P(&(pbx->pbx_lock));    // Writer can enter CS of pbx->head.
    curr_node->next = new_node;     // Writer writes.
    V(&(pbx->pbx_lock));    // Writer can enter CS of pbx->head.
    
    P(&(pbx->node_count_mutex));
    pbx->node_count += 1;       // Increment node count.
    V(&(pbx->node_count_mutex));

    debug("Registered new client.\n");
    return 0;
}
#endif

/*
 * Unregister a TU from a PBX.
 * This amounts to "unplugging a telephone unit from the PBX".
 * The TU is disassociated from its extension number.
 * Then a hangup operation is performed on the TU to cancel any
 * call that might be in progress.
 * Finally, the reference held by the PBX to the TU is released.
 *
 * @param pbx  The PBX.
 * @param tu  The TU to be unregistered.
 * @return 0 if unregistration succeeds, otherwise -1.
 */
#if 1
int pbx_unregister(PBX *pbx, TU *tu) {
    debug("Inside pbx_unregister().\n");

    reader_enters(&pbx_read_cnt_mutex, pbx);
    
    // Reading CS of pbx->head.
    struct pbx_node *curr_node = pbx->head;                        // Declare and assign a pbx_node pointer to point to pbx->head.

    while (curr_node != NULL)
    {   
        if (curr_node->tu == tu)                  // If 'tu' is found via address.
        {
            debug("Tu has been found. Unregistering it now.\n");

            reader_leaves(&pbx_read_cnt_mutex, pbx);

            tu_set_extension(tu, -1);       // Set the extension number of the now unregistered tu to -1.
            tu_hangup(tu);               // A hangup operation is performed on the tu to cancel any call that might be in progress.
            free(curr_node->tu);              // Free the tu structure that is getting unregistered from pbx.
            
            // Writing to pbx->head.
            P(&(pbx->pbx_lock));    // Writer enters critical section of pbx->head.
            curr_node->ext = -1;            // Disassociate 'tu' from its extension number. Set it to -1.
            V(&(pbx->pbx_lock));    // Writer leaves the CS of pbx->head.

            P(&(pbx->node_count_mutex));    // Writer enters critical section of pbx->node_count.
            pbx->node_count -= 1;       // Decrement node_count.
            V(&(pbx->node_count_mutex));    // Writer enters critical section of pbx->node_count.

            return 0;
        }
        curr_node = curr_node->next;
    }
    reader_leaves(&pbx_read_cnt_mutex, pbx);
    debug("tu was not found in pbx. Returning -1.\n");
    return -1;
    // abort();
}
#endif

/*
 * Use the PBX to initiate a call from a specified TU to a specified extension.
 *
 * @param pbx  The PBX registry.
 * @param tu  The TU that is initiating the call.
 * @param ext  The extension number to be called.
 * @return 0 if dialing succeeds, otherwise -1.
 */
#if 1
int pbx_dial(PBX *pbx, TU *tu, int ext) {
    debug("Inside pbx_dial().\n");

    reader_enters(&pbx_read_cnt_mutex, pbx);
    
    // Reader reads CS of pbx->head.
    struct pbx_node *curr_node = pbx->head;                        // Declare and assign a PBX pointer to point to the first PBX structure in the linked list.

    while (curr_node != NULL)
    {
        if (curr_node->ext == ext)
        {
            debug("Ext found. Dialing it now. Returning 0.\n");

            reader_leaves(&pbx_read_cnt_mutex, pbx);
            tu_dial(tu, curr_node->tu);     // Dial the TU associated with 'ext'.
            return 0;
        }

        curr_node = curr_node->next;
    }
    // V(&(pbx->pbx_lock));
    reader_leaves(&pbx_read_cnt_mutex, pbx);

    debug("ext was not found in pbx. Returning -1.\n");
    tu_dial(tu, NULL);                          // If ext was not found in pbx, dial with NULL target.         
    return -1;                                  // Return -1.
    // abort();
}
#endif
