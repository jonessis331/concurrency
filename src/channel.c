#include "channel.h"

// begins listeing on the channel and connects a linked list of them 
void channel_start_listening(channel_t* ch,sem_t* select_sem) {
    llsem* newhead = malloc (sizeof(llsem));
    //randomly segfualting sometimes when same_channel_buffered is running 
    if (!newhead) {
        printf ("OUT OF MEMORY\n");
        exit   (1);
    }
    // sets the sem associated with a single select call 
    newhead->this = select_sem;//associated with a single select() call
    pthread_mutex_lock(&ch->selects_lock);

    //updates linked list
    newhead->next = ch->selects;
    ch->selects = newhead;
    pthread_mutex_unlock(&ch->selects_lock);
}
void channel_stop_listening(channel_t* ch,sem_t* select_sem) {
    llsem decoy;
    llsem* pre_it = & decoy;

    pthread_mutex_lock(&ch->selects_lock);
    // search through the linked list of sems connected to the provide sem
    // iterator it
    llsem* it = ch->selects;
    while (it){
        if (it->this == select_sem){
            // removes sem if found 
            pre_it->next = it->next;
            if(it == ch->selects)
                ch->selects = it->next;
            free (it);
            break;
        }
        pre_it = it;
        it = it->next;
    }
    pthread_mutex_unlock(&ch->selects_lock);
}
// tells all select calls that a change happedn 
void channel_alert_change(channel_t* ch) {
    llsem* sel;
    pthread_mutex_lock(&ch->selects_lock);
    sel = ch->selects;//up-to-date as possible
    //post for every single select() that has been hooked in for this channel (like a broadcast(multicast))
    while (sel) { 
        sem_post (sel->this);
        sel = sel->next;
    }
    pthread_mutex_unlock(&ch->selects_lock);
}

// Creates a new channel with the provided size and returns it to the caller
// A 0 size indicates an unbuffered channel, whereas a positive size indicates a buffered channel
channel_t* channel_create(size_t size)
{
    /* IMPLEMENT THIS */
    channel_t* channel = (channel_t*)malloc(sizeof(channel_t));
    if (channel == NULL)
        return NULL;
    
    channel->buffer = buffer_create(size);
    channel->selects = 0; 

    pthread_mutex_init(&channel->buffer_lock, NULL);
    pthread_mutex_init(&channel->selects_lock, NULL);

    pthread_cond_init(&channel->hasStuff, NULL);
    pthread_cond_init(&channel->notFull, NULL);

    channel->isClosed = false;

    return channel; 
}

// Writes data to the given channel
// This is a blocking call i.e., the function only returns on a successful completion of send
// In case the channel is full, the function waits till the channel has space to write the new data
// Returns SUCCESS for successfully writing data to the channel,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort
enum channel_status channel_send(channel_t *channel, void* data)
{
    /* IMPLEMENT THIS */
    // size_t buffSize, buffCap;
    pthread_mutex_lock(&(channel->buffer_lock));
    // buffSize = channel->buffer->size;
    // buffCap = channel->buffer->capacity;

    while(buffer_current_size(channel->buffer) == buffer_capacity(channel->buffer) && !channel->isClosed){
        pthread_cond_wait(&channel->notFull, &channel->buffer_lock);
    }
    if(channel->isClosed){
        pthread_mutex_unlock(&(channel->buffer_lock));
        return CLOSED_ERROR;
    }

    buffer_add(channel->buffer,data);
    channel_alert_change(channel); //alert all select() calls that the state of channel "ch" has changed
    pthread_cond_signal(&channel->hasStuff); // wakes up any threads waiting on hasStuff 

    pthread_mutex_unlock(&(channel->buffer_lock));

    return SUCCESS;
}

// Reads data from the given channel and stores it in the function's input parameter, data (Note that it is a double pointer)
// This is a blocking call i.e., the function only returns on a successful completion of receive
// In case the channel is empty, the function waits till the channel has some data to read
// Returns SUCCESS for successful retrieval of data,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort
enum channel_status channel_receive(channel_t* channel, void** data)
{
    /* IMPLEMENT THIS */
    // size_t buffSize;
    pthread_mutex_lock(&(channel->buffer_lock));
    // buffSize = channel->buffer->size;

    while(buffer_current_size(channel->buffer) < 1 && !channel->isClosed){
        pthread_cond_wait(&channel->hasStuff, &channel->buffer_lock);
    }

    if(channel->isClosed){
        pthread_mutex_unlock(&(channel->buffer_lock));
        return CLOSED_ERROR;
    }

    buffer_remove(channel->buffer,data); //pops into data
    channel_alert_change(channel); // alerts select calls that channel was changed 
    pthread_cond_signal(&channel->notFull);
  

    pthread_mutex_unlock(&(channel->buffer_lock));

    return SUCCESS;
}

// Writes data to the given channel
// This is a non-blocking call i.e., the function simply returns if the channel is full
// Returns SUCCESS for successfully writing data to the channel,
// CHANNEL_FULL if the channel is full and the data was not added to the buffer,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort
enum channel_status channel_non_blocking_send(channel_t* channel, void* data)
{
    /* IMPLEMENT THIS */
    size_t buffSize, buffCap;
    pthread_mutex_lock(& (channel->buffer_lock));
    //gonna do this no matter what
    buffSize  = channel->buffer->size;
    buffCap = channel->buffer->capacity;
    if(channel->isClosed){
        pthread_mutex_unlock(&(channel->buffer_lock));
        return CLOSED_ERROR;
    }

    if (buffSize == buffCap) { //either we can do it, or we can't
        //just give up
        pthread_mutex_unlock(&(channel->buffer_lock)); //let others do stuff
        return CHANNEL_FULL;//quit now
    }
    else {
        // we've got space
        buffer_add(channel->buffer,data);
        channel_alert_change(channel); 
       // pthread_cond_signal(&channel->hasStuff);
        pthread_mutex_unlock(&(channel->buffer_lock));
    }
    return SUCCESS;
}

// Reads data from the given channel and stores it in the function's input parameter data (Note that it is a double pointer)
// This is a non-blocking call i.e., the function simply returns if the channel is empty
// Returns SUCCESS for successful retrieval of data,
// CHANNEL_EMPTY if the channel is empty and nothing was stored in data,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort
enum channel_status channel_non_blocking_receive(channel_t* channel, void** data)
{
    /* IMPLEMENT THIS */
    size_t buffSize;
    pthread_mutex_lock(&(channel->buffer_lock));
    buffSize = channel->buffer->size;
    if(channel->isClosed){
        pthread_mutex_unlock(&(channel->buffer_lock));
        return CLOSED_ERROR;
    }
    if(buffSize < 1) {
        pthread_mutex_unlock(&(channel->buffer_lock));
        return CHANNEL_EMPTY;
    }
    else {
        buffer_remove(channel->buffer,data); //pops into data
        channel_alert_change(channel); 
        pthread_cond_signal(&channel->notFull);
        pthread_mutex_unlock(&(channel->buffer_lock));
        
    }
    //pthread_cond_wait(&channel->hasStuff, &channel->buffer_lock);
    //pthread_cond_signal(&channel->notFull);

    return SUCCESS;
}

// Closes the channel and informs all the blocking send/receive/select calls to return with CLOSED_ERROR
// Once the channel is closed, send/receive/select operations will cease to function and just return CLOSED_ERROR
// Returns SUCCESS if close is successful,
// CLOSED_ERROR if the channel is already closed, and
// GEN_ERROR in any other error case
enum channel_status channel_close(channel_t* channel)
{
    int err ;
    /* IMPLEMENT THIS */
    pthread_mutex_lock(& channel->buffer_lock);
    if (channel->isClosed) 
        err = CLOSED_ERROR;
    else{
        channel->isClosed = true;
        channel_alert_change(channel); 
        pthread_cond_broadcast(&channel->notFull); //wakes up every thread waiting on notfull 
        pthread_cond_broadcast(&channel->hasStuff); //wakes up every thread waiting on hasStuff 
        err = SUCCESS;
    }
    pthread_mutex_unlock(& channel->buffer_lock);
    return err;
}

// Frees all the memory allocated to the channel
// The caller is responsible for calling channel_close and waiting for all threads to finish their tasks before calling channel_destroy
// Returns SUCCESS if destroy is successful,
// DESTROY_ERROR if channel_destroy is called on an open channel, and
// GEN_ERROR in any other error case
enum channel_status channel_destroy(channel_t* channel)
{
    /* IMPLEMENT THIS */
    pthread_mutex_lock(&channel->buffer_lock);
    if(channel->isClosed == false){
        pthread_mutex_unlock(&channel->buffer_lock);
        return DESTROY_ERROR;
    }
    else {
        pthread_mutex_unlock(&channel->buffer_lock);
    }

    pthread_cond_destroy(&channel->hasStuff);
    pthread_cond_destroy(&channel->notFull);
    //pthread_mutex_unlock(&channel->buffer_lock);

    pthread_mutex_destroy(&channel->buffer_lock);
    pthread_mutex_destroy(&channel->selects_lock);
    buffer_free(channel->buffer);

    // traverses the sem linked list, and freees all the mem 
    while(channel->selects){
        llsem* next = channel->selects->next;
        free (channel->selects);
        channel->selects = next; // set current to next 
    }
    free(channel);

    return SUCCESS;
}

// should handle test_select_duplicate_channel_buffered 
size_t first_occurrence (channel_t* ch,int OP,select_t* list,size_t count) {
    //only applies when it's the same OP (send/recv)
    //prioritizing the first send() or first recv() for a channel in a select-list
    for (size_t i = 0;i < count;i ++) {
        if (list[i].channel == ch && list[i].dir == OP) return i ;
    }
    
    //should be impossible
    return (size_t)-1 ;
}
// Takes an array of channels (channel_list) of type select_t and the array length (channel_count) as inputs
// This API iterates over the provided list and finds the set of possible channels which can be used to invoke the required operation (send or receive) specified in select_t
// If multiple options are available, it selects the first option and performs its corresponding action
// If no channel is available, the call is blocked and waits till it finds a channel which supports its required operation
// Once an operation has been successfully performed, select should set selected_index to the index of the channel that performed the operation and then return SUCCESS
// In the event that a channel is closed or encounters any error, the error should be propagated and returned through select
// Additionally, selected_index is set to the index of the channel that generated the error
enum channel_status channel_select(select_t* channel_list, size_t channel_count, size_t* selected_index)
{
    /* IMPLEMENT THIS */
    if(!channel_list || selected_index == NULL)
    {
        return GEN_ERROR;
    }
    sem_t select_sem         ;//the one sem associated to this function call instance
    sem_init (&select_sem,0,0) ;

    void* data;
    enum channel_status cs = SUCCESS;

    for (size_t s = 0;s < channel_count;s ++){
        channel_t* chan = channel_list[s].channel;
        channel_start_listening(chan,& select_sem);//put our chips in every channel we are dealing with 
                                                   //for this specific select() instance
    }
    while (1){
        for (size_t s = 0;s < channel_count;s ++){
            channel_t* chan = channel_list[s].channel;
            if (chan->isClosed) {
                (*selected_index) = s;
                for (size_t s = 0;s < channel_count;s ++) {
                    channel_t* chan = channel_list[s].channel ;
                    channel_stop_listening(chan,& select_sem); //pull out from the listen-queue
                }
                sem_destroy(&select_sem); 
                return CLOSED_ERROR;
            }
            else {
                if (channel_list[s].dir == SEND) {
                    size_t first = first_occurrence (chan,SEND,channel_list,channel_count);
                    cs = channel_non_blocking_send (chan,channel_list[s].data);
                    if(cs == SUCCESS){
                        *selected_index = first;
                        for (size_t s = 0;s < channel_count;s ++) {
                            channel_t* chan = channel_list[s].channel;
                            channel_stop_listening(chan,& select_sem); //pull out from the listen-queue
                        }
                        sem_destroy(&select_sem);
                        return cs;
                    }
                }
                else {
                    cs = channel_non_blocking_receive(chan,&data);
                    //VISUAL FOR what is happening in first_occurence 
                    //[0].data [1].data
                    //[1].data = something
                    //return first_occurrence() => 0  ([0].data unset)
                    if(cs == SUCCESS){
                        size_t first = first_occurrence(chan,RECV,channel_list,channel_count);
                        channel_list[first].data = data;
                        *selected_index = first;
                        for (size_t s = 0;s < channel_count;s ++) {
                            channel_t* chan = channel_list[s].channel;
                            channel_stop_listening(chan,& select_sem);//pull out from the listen-queue
                        }
                        sem_destroy (&select_sem);
                        return cs;
                    }
                }
            }
        
        }
        //could not find a channel to satisfy select()
        //block until we do
        printf("sleep");
        sem_wait(&select_sem);

    }
    // really should never get here
    sem_destroy(&select_sem);

    return -1 ;

}


