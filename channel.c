#include "channel.h"
#include "buffer.h"
#include "linked_list.h"

// Creates a new channel with the provided size and returns it to the caller
// A 0 size indicates an unbuffered channel, whereas a positive size indicates a buffered channel
chan_t* channel_create(size_t size)
{
     chan_t *channel = malloc(sizeof(chan_t));
	//intilize the input (send) and output(recieve) semaphore
    	sem_init(&channel->sem_in, 0, (unsigned int)size); //cast the size 
        sem_init(&channel->sem_out, 0, 0);
        
	
	if (size == 0) {
        channel->buffered = false; //unbaffered
        channel->buffer = NULL;//no buffer
    } else { //size >0
        channel->buffer = buffer_create(size); //create buffer with the specified size 
        channel->buffered = true;
    }

    channel-> closed = false;

    pthread_mutex_init(&channel->mutex, NULL);//initialize the mutex to synchronize
	pthread_mutex_init(&channel->s_mutex, NULL);//send_mutex
    pthread_mutex_init(&channel->r_mutex, NULL);//recieve mutex
    // Create send and receive lists
    channel->send_list = list_create(); //create send and recieve list for select 
    channel->receive_list = list_create();

    return channel;
}

// Writes data to the given channel
// This can be both a blocking call i.e., the function only returns on a successful completion of send (blocking = true), and
// a non-blocking call i.e., the function simply returns if the channel is full (blocking = false)
// In case of the blocking call when the channel is full, the function waits till the channel has space to write the new data
// Returns SUCCESS for successfully writing data to the channel,
// WOULDBLOCK if the channel is full and the data was not added to the buffer (non-blocking calls only),
// CLOSED_ERROR if the channel is closed, and
// OTHER_ERROR on encountering any other generic error of any sort
enum chan_status channel_send(chan_t* channel, void* data, bool blocking)
{
    /* IMPLEMENT THIS */

	//here in bloking case, first wait in th channel, then lock and check if the channel is closed, then add data and send signal data, and unlock.
    if (blocking == true) {
            		sem_wait(&channel->sem_in);
            		pthread_mutex_lock(&channel->mutex);
            		if (channel->closed){//if the channel is closed, send signal data for the send semaphore and unlock, and return closed_error
            			sem_post(&channel->sem_in);
            			pthread_mutex_unlock(&channel->mutex); 
            			return CLOSED_ERROR;
            		}
            		buffer_add(data, channel->buffer); 
					pthread_mutex_unlock(&channel->mutex);
            		sem_post(&channel->sem_out);//send data signal for recieve semaphore

                    pthread_mutex_lock(&channel->r_mutex);
                    list_node_t *current = channel->receive_list->head; //to loop over each semaphore in list and [post it
                    while (current != NULL) {
                        sem_post((sem_t*)current-> data);
                        current = current->next;
                    }
                    pthread_mutex_unlock(&channel->r_mutex);
            	    
            		
        	} 
        	else {//here in non bloking case if there available space within the channel without waiting, lock and check if the channel is closed then add data and sed the data signal and then unlock.
            		if (sem_trywait(&channel->sem_in) == 0) {
                 		pthread_mutex_lock(&channel->mutex);
            			if (channel->closed){
            				sem_post(&channel->sem_in);
            				pthread_mutex_unlock(&channel->mutex); 
            				return CLOSED_ERROR;
            			}
            			buffer_add(data, channel->buffer); 
            			pthread_mutex_unlock(&channel->mutex);
            			sem_post(&channel->sem_out);//send data signal for recieve semaphore
                        pthread_mutex_lock(&channel->r_mutex);
                        list_node_t *current = channel->receive_list->head;
                        while (current != NULL) {   //to loop over each semaphore in list and [post it             
                            sem_post((sem_t*)current-> data );
                            current = current->next;
                        }
                        pthread_mutex_unlock(&channel->r_mutex);
            			
            		
        		} 
            		else {

                		return WOULDBLOCK;
                    }
            }
        
        return SUCCESS; 
}

// Reads data from the given channel and stores it in the function’s input parameter, data (Note that it is a double pointer).
// This can be both a blocking call i.e., the function only returns on a successful completion of receive (blocking = true), and
// a non-blocking call i.e., the function simply returns if the channel is empty (blocking = false)
// In case of the blocking call when the channel is empty, the function waits till the channel has some data to read
// Returns SUCCESS for successful retrieval of data,
// WOULDBLOCK if the channel is empty and nothing was stored in data (non-blocking calls only),
// CLOSED_ERROR if the channel is closed, and
// OTHER_ERROR on encountering any other generic error of any sort
enum chan_status channel_receive(chan_t* channel, void** data, bool blocking)
{
    /* IMPLEMENT THIS */
//here in bloking case, first wait in th channel, then lock and check if the channel is closed, then remove data and send signal data, and unlock. 
    if (blocking == true) {
            		sem_wait(&channel->sem_out);
            		pthread_mutex_lock(&channel->mutex);
            		if (channel->closed){//if the channel is closed, send data signal for the send semaphore and unlock, and return closed_error
            			sem_post(&channel->sem_out); //to notify other recievers
            			pthread_mutex_unlock(&channel->mutex); 
            			return CLOSED_ERROR;
            		}
                    
                    
                    //pthread_mutex_lock(&channel->mutex);
            		*data = buffer_remove(channel->buffer); 
            		pthread_mutex_unlock(&channel->mutex);
            		sem_post(&channel->sem_in);//send data signal for send semaphore
                    pthread_mutex_lock(&channel->s_mutex);
                    list_node_t *current = channel->send_list->head;
                        while (current != NULL) {
                            //list_node_t *t_data = &current->data;                   
                            sem_post((sem_t*)current-> data );
                            current = current->next;
                            }
                    pthread_mutex_unlock(&channel->s_mutex);
            		
            		
        	} 
        	else {//here in non bloking case if there available space within the channel without waiting, lock and check if the channel is closed then remove data and sed the data signal and then unlock.
            		if (sem_trywait(&channel->sem_out) == 0) {
                 		pthread_mutex_lock(&channel->mutex);
            			if (channel->closed){
            				sem_post(&channel->sem_out);

            				pthread_mutex_unlock(&channel->mutex); 
            				return CLOSED_ERROR;
            			}
            			*data = buffer_remove(channel->buffer); 
            			pthread_mutex_unlock(&channel->mutex);
            			
            			sem_post(&channel->sem_in);//send data signal for send semaphore
                        pthread_mutex_lock(&channel->s_mutex);
                        list_node_t *current = channel->send_list->head;
                        while (current != NULL) {                
                            sem_post((sem_t*)current-> data );
                            current = current->next;
                            }
                        pthread_mutex_unlock(&channel->s_mutex);
            			
            		
        		} 
            		else {//if the channel is full, return wouldblock
                		return WOULDBLOCK;
            }
            }
        
	
        return SUCCESS;
}

// Closes the channel and informs all the blocking send/receive/select calls to return with CLOSED_ERROR
// Once the channel is closed, send/receive/select operations will cease to function and just return CLOSED_ERROR
// Returns SUCCESS if close is successful,
// CLOSED_ERROR if the channel is already closed, and
// OTHER_ERROR in any other error case
enum chan_status channel_close(chan_t* channel)
{
    /* IMPLEMENT THIS */
    pthread_mutex_lock(&channel->mutex);

    // Check if the channel is already closed, if closed unlock the mutex and return an error.
    if (channel->closed) {
        pthread_mutex_unlock(&channel->mutex); 
        return CLOSED_ERROR;
    }
    
    channel->closed = true;
    pthread_mutex_unlock(&channel->mutex);
    
    //check if any threads waiting on the channel's in and out semaphore.
    sem_post(&channel->sem_in);
    sem_post(&channel->sem_out);
    
    pthread_mutex_lock(&channel->s_mutex);

    // Iterate through the send list and wake up semaphore at the waiting threads.
    list_node_t *current_s = channel->send_list->head;
        while (current_s != NULL) {
                              
            sem_post((sem_t*)current_s -> data);
            current_s = current_s->next;
        }
    
    pthread_mutex_unlock(&channel->s_mutex);

    pthread_mutex_lock(&channel->r_mutex);
    // Iterate through the receive list and wake up semaphore at the waiting threads.
    list_node_t *current_r = channel->receive_list->head;
        while (current_r != NULL) {
                              
            sem_post((sem_t*)current_r-> data );
            current_r = current_r->next;
         }
    
    pthread_mutex_unlock(&channel->r_mutex);
    
    return SUCCESS;
}

// Frees all the memory allocated to the channel
// The caller is responsible for calling channel_close and waiting for all threads to finish their tasks before calling channel_destroy
// Returns SUCCESS if destroy is successful,
// DESTROY_ERROR if channel_destroy is called on an open channel, and
// OTHER_ERROR in any other error case
enum chan_status channel_destroy(chan_t* channel)
{
    /* IMPLEMENT THIS */
    //pthread_mutex_lock(&channel->mutex);

    if (channel->closed == false)//check if the channel is closed prior before going on, return destroy_error 
    {
    	//pthread_mutex_unlock(&channel->mutex);
        return DESTROY_ERROR;
    }
    //pthread_mutex_unlock(&channel->mutex);
	// Free the buffer if the channel is buffered
    if (channel->buffered == true) {
        buffer_free(channel->buffer);
    }
    //destroy all semaphore in the channel
    sem_destroy(&channel->sem_in);
    sem_destroy(&channel->sem_out);
	
    //destroy the mutex and free the channel
   
    pthread_mutex_destroy(&channel->mutex);
    pthread_mutex_destroy(&channel->s_mutex);
    pthread_mutex_destroy(&channel->r_mutex);
   
    // call the list_destroy to free the memory
    list_destroy(channel-> send_list);
    list_destroy(channel->receive_list);
    
    free(channel);

    
    return SUCCESS;
}

// Takes an array of channels, channel_list, of type select_t and the array length, channel_count, as inputs
// This API iterates over the provided list and finds the set of possible channels which can be used to invoke the required operation (send or receive) specified in select_t
// If multiple options are available, it selects the first option and performs its corresponding action
// If no channel is available, the call is blocked and waits till it finds a channel which supports its required operation
// Once an operation has been successfully performed, select should set selected_index to the index of the channel that performed the operation and then return SUCCESS
// In the event that a channel is closed or encounters any error, the error should be propagated and returned through select
// Additionally, selected_index is set to the index of the channel that generated the error
enum chan_status channel_select(size_t channel_count, select_t* channel_list, size_t* selected_index)
{
	
	
    /* IMPLEMENT THIS */
    sem_t select_semaphore;
    //int j =0;
    
    enum chan_status s;

    // Initialize the semaphore
    sem_init(&select_semaphore, 0, 0);

      
    // Iterate over all channels in the provided list
    for (size_t i = 0; i < (unsigned int) channel_count; i++) {
       
       if (channel_list[i].is_send) {
            pthread_mutex_lock(&channel_list[i].channel->s_mutex);
            if (list_find(channel_list[i].channel->send_list, &select_semaphore)== NULL) {
                //pthread_mutex_lock(&channel_list[i].channel->s_mutex);
                //insert the semaphore into the send list
                list_insert(channel_list[i].channel->send_list, &select_semaphore);
                pthread_mutex_unlock(&channel_list[i].channel->s_mutex);
                
            }
            
        } else {
            
            //check if the semaphore is not in the receive list
            pthread_mutex_lock(&channel_list[i].channel->r_mutex);
            if (list_find(channel_list[i].channel->receive_list, &select_semaphore)==NULL) {
                //pthread_mutex_lock(&channel_list[i].channel->r_mutex);
                //insert the semaphore in the receive list
                list_insert(channel_list[i].channel->receive_list, &select_semaphore);
                pthread_mutex_unlock(&channel_list[i].channel->r_mutex);
            }
            
        }
        
    }
    
        // loop until the channel is ready
        while (true) {

                        for(size_t j = 0; j <  channel_count; j++){
                            if (channel_list[j].is_send){
                                s = channel_send(channel_list[j].channel, channel_list[j].data, false);
                            }
                            else{
                                s = channel_receive(channel_list[j].channel, &channel_list[j].data, false);
                            }

                            // Check if the operation was successful
                            if (s != WOULDBLOCK){
                                //update the selected index
                                    *selected_index =  j;

                                    //remove the semaphore from all channel lists
                                    for (size_t k=0; k <  channel_count; k++){
                                    list_t* new_list;
                                        // check if the current channel is for sending or receiving
                                        if(!channel_list[k].is_send){
                                            pthread_mutex_lock(&channel_list[k].channel->r_mutex);
                                            new_list = channel_list[k].channel->receive_list;
                                            //look for the semaphore node is in the current list
                                            list_node_t* node = list_find(new_list, &select_semaphore);
                                            
                                            //if found, remove it from the list
                                            if (node != NULL){
                                                list_remove(new_list, node);
                                                pthread_mutex_unlock(&channel_list[k].channel->r_mutex);
                                            }
                                        }

                                        else{
                                            pthread_mutex_lock(&channel_list[k].channel->s_mutex);
                                            new_list = channel_list[k].channel->send_list;
                                            list_node_t* node = list_find(new_list, &select_semaphore);
                                            if(node != NULL){
                                                list_remove(new_list, node);
                                                pthread_mutex_unlock(&channel_list[k].channel->s_mutex);
                                            }
                                        }
                                    }

                                    //destroy the semaphore and return the status
                                    sem_destroy(&select_semaphore);
                                
                                    return s;
                                    //printf("%s",s);
                        
                        
                            } 
                                
                        }

                            //waitn on the semaphore if no channel is ready
                            sem_wait(&select_semaphore);
                    }

}
