/*
   Task 4 solution - Week 5 exercise session for course 02211
   Author: Luca Pezzarossa (lpez@dtu.dk) Copyright: DTU, BSD License
*/

//const int NOC_MAIN_CORE_ID = 0;
const int NOC_MASTER = 0;
#define CORETHREAD_INIT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <machine/patmos.h>
#include "libnoc/noc.h"
#include "libcorethread/corethread.h"
#include "libmp/mp.h"

#define MP_CHAN_NUM_BUF 2
#define MP_CHAN_BUF_SIZE 40

void worker_core_job(void* param);
void sort_array(int array[], int size);

int main() {
    int worker_core_param = 0;

    //Start all the worker threads
    printf("Create threads\n");
    for(int i = 0; i < get_cpucnt(); i++) {
        if (i != NOC_MASTER) {
            if(corethread_create(i,&worker_core_job,(void*)worker_core_param) != 0){
                printf("Corethread %d not created\n",i);
            }
        }
    }
    
    //Declare array to order
    int array_to_order[5][10] = {
        {2,1,3,6,4,5,7,8,9,0},
        {17,18,14,15,11,12,13,10,19,16},
        {26,27,23,25,24,21,20,29,28,22},
        {35,34,30,39,38,37,31,32,36,33},
        {43,45,44,47,49,48,46,41,42,40},
    };
    
    //Declare a clear array to place the ordered result
    int ordered_array[5][10] = {
        {0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0},
    };
    
    //Create input and output channels
    printf("Create channels\n");
    qpd_t * out_channel[5];
    qpd_t * in_channel[5];
    for(int i = 0; i < 5; i++){
        out_channel[i] = mp_create_qport(i+1, SOURCE, MP_CHAN_BUF_SIZE, MP_CHAN_NUM_BUF);
        in_channel[i] = mp_create_qport(i+1+5, SINK, MP_CHAN_BUF_SIZE, MP_CHAN_NUM_BUF);
    }
    mp_init_ports();
    
    //Send each row of the array to the relative worker core
    printf("Sending\n");
    for(int i = 0; i < 5; i++) {
    	for(int j = 0; j < 10; j++) {
            (( volatile int _SPM * ) ( out_channel[i]->write_buf ))[j] = array_to_order[i][j];
        }
        mp_send(out_channel[i], 0);                   
    }
    
    //Receive the ordered rows of the array from all the workers cores and populate the 'ordered_array'
    printf("Receiving\n");
    for(int i = 0; i < 5; i++) {
    	mp_recv(in_channel[i], 0);
    	for(int j = 0; j < 10; j++) {
            ordered_array[i][j] = (( volatile int _SPM * ) ( in_channel[i]->write_buf ))[j];
        }
        mp_ack(in_channel[i],0);
    }
    
    //Print the ordered array
    printf("Ordered array:\n");
    for(int i = 0; i < 5; i++) {
    	for(int j = 0; j < 10; j++) {
            printf("%d ",ordered_array[i][j]);
        }
        printf("\n");
    }

    return 0;
}

void worker_core_job(void* param) {
    //Declare local array row
    int row_to_order[10];
    
    //Create input and output channels
    int in_channel_id = get_cpuid();
    int out_channel_id = get_cpuid() + 5;
    qpd_t * in_channel = mp_create_qport(in_channel_id, SINK, MP_CHAN_BUF_SIZE, MP_CHAN_NUM_BUF);
    qpd_t * out_channel = mp_create_qport(out_channel_id, SOURCE, MP_CHAN_BUF_SIZE, MP_CHAN_NUM_BUF);
    mp_init_ports();

    //Receive the array row to order
    mp_recv(in_channel, 0);
    
    //Copy input buffer (the receive row) to local array
    for(int i = 0; i < 10; i++) {
        row_to_order[i] = (( volatile int _SPM * ) ( in_channel->read_buf ))[i];
    }
    
    //Acknowledge reception and release receive buffer
    mp_ack(in_channel,0);    

    //Sort the local array using helper function
    sort_array(row_to_order, 10);
    
    //Copy local ordered array to sending buffer
    for(int i = 0; i < 10; i++) {
        (( volatile int _SPM * ) ( out_channel->read_buf ))[i] = row_to_order[i];
    }
        
    //Send back to master core
    mp_send(out_channel,0);
      
    return;
}

//Helper function to sort and array of a given size
void sort_array(int array[], int size){
    int i, j;
    for (i = 0; i < size - 1; i++){
        for (j = 0; j < size - i - 1; j++){
            if (array[j] > array[j + 1]){
                int temp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = temp;
            }
        }
    }
}


