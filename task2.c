/*
   Task 2 - Week 5 exercise session for course 02211
   Author: Luca Pezzarossa (lpez@dtu.dk) Copyright: DTU, BSD License
*/

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
#define MP_CHAN_BUF_SIZE 4

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
    printf("Creating channels\n");
    int in_channel_id = get_cpuid();
    int out_channel_id = (get_cpuid() + 1) % get_cpucnt();
    qpd_t * in_channel = mp_create_qport(in_channel_id, SINK, MP_CHAN_BUF_SIZE, MP_CHAN_NUM_BUF);
    qpd_t * out_channel = mp_create_qport(out_channel_id, SOURCE, MP_CHAN_BUF_SIZE, MP_CHAN_NUM_BUF);
    mp_init_ports();

    //Sending current id value (0)    
    printf("Sending\n");
    *( volatile int _SPM * ) ( out_channel->write_buf ) = (int)get_cpuid();               
    mp_send(out_channel, 0);

    //Receiving the final sum
    printf("Receiving\n");
    mp_recv(in_channel, 0);
    int sum = *(( volatile int _SPM * ) ( in_channel->read_buf ));  
    
    //Acknowledge reception and release receive buffer    
    mp_ack(in_channel,0);    

    //printing the sum
    printf("Sum is %d\n", sum);

    return 0;
}

void worker_core_job(void* param) {
    int sum;
    
    //Create input and output channels
    int in_channel_id = get_cpuid();
    int out_channel_id = (get_cpuid() + 1) % get_cpucnt();
    qpd_t * in_channel = mp_create_qport(in_channel_id, SINK, MP_CHAN_BUF_SIZE, MP_CHAN_NUM_BUF);
    qpd_t * out_channel = mp_create_qport(out_channel_id, SOURCE, MP_CHAN_BUF_SIZE, MP_CHAN_NUM_BUF);
    mp_init_ports();

    //Receive sum value from previous core
    mp_recv(in_channel, 0);
    sum = *(( volatile int _SPM * ) ( in_channel->read_buf ));
    
    //Acknowledge reception and release receive buffer  
    mp_ack(in_channel,0);    
    
    //Add core ID to the sum
    sum = sum + (int)get_cpuid();
    
    //Send sum to next core
    *( volatile int _SPM * ) ( out_channel->write_buf ) = sum;               
    mp_send(out_channel,0);
      
    return;
}

