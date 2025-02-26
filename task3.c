/*
   Task 3 - Training school
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
    
    //Create input and output channels
    printf("Create channels\n");
    int in_channel_id = get_cpuid();
    int out_channel_id = (get_cpuid() + 1) % get_cpucnt();
    qpd_t * in_channel = mp_create_qport(in_channel_id, SINK, MP_CHAN_BUF_SIZE, MP_CHAN_NUM_BUF);
    qpd_t * out_channel = mp_create_qport(out_channel_id, SOURCE, MP_CHAN_BUF_SIZE, MP_CHAN_NUM_BUF);
    mp_init_ports();

    //Clear first 'cpucnt' locations of sending buffer
    for(int i = 0; i < get_cpucnt(); i++) {
        (( volatile int _SPM * ) ( out_channel->write_buf ))[i] = 0;               
    }

    //Writing 0 at location 0 of the array and sending    
    printf("Sending\n");
    (( volatile int _SPM * ) ( out_channel->write_buf ))[get_cpuid()] = get_cpuid();
    mp_send(out_channel, 0);

    //Receiving the final array
    printf("Receiving\n");
    mp_recv(in_channel, 0);

    //Print the received array
    printf("Result:\n");
    for(int i = 0; i < get_cpucnt(); i++) {
        printf("%d\n",(( volatile int _SPM * ) ( in_channel->read_buf ))[i]);               
    }
    
    //Acknowledge reception and release receive buffer
    mp_ack(in_channel,0);    

    return 0;
}

void worker_core_job(void* param) {
    //Create input and output channels
    int in_channel_id = get_cpuid();
    int out_channel_id = (get_cpuid() + 1) % get_cpucnt();
    qpd_t * in_channel = mp_create_qport(in_channel_id, SINK, MP_CHAN_BUF_SIZE, MP_CHAN_NUM_BUF);
    qpd_t * out_channel = mp_create_qport(out_channel_id, SOURCE, MP_CHAN_BUF_SIZE, MP_CHAN_NUM_BUF);
    mp_init_ports();

    //Receive array from previous core and add 'core id' at position 'core id'
    mp_recv(in_channel, 0);
    
    //Copy input buffer to output buffer
    for(int i = 0; i < get_cpucnt(); i++) {
        (( volatile int _SPM * ) ( out_channel->write_buf ))[i] = (( volatile int _SPM * ) ( in_channel->read_buf ))[i];

    }
    //Acknowledge reception and release receive buffer
    mp_ack(in_channel,0);    

    (( volatile int _SPM * ) ( out_channel->write_buf ))[get_cpuid()] = get_cpuid();  
    
    //Send array to next core
    mp_send(out_channel,0);
      
    return;
}

