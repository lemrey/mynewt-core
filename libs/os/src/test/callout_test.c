/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
 
#include "testutil/testutil.h"
#include "os/os.h"
#include "os_test_priv.h"
#include "os/os_eventq.h"
#include "os/os_callout.h"

/* Task 1 for sending */
#define CALLOUT_STACK_SIZE        (5120)
#define SEND_CALLOUT_TASK_PRIO        (1)
struct os_task callout_task_struct_send;
os_stack_t callout_task_stack_send[CALLOUT_STACK_SIZE];

#define RECEIVE_CALLOUT_TASK_PRIO        (2)
struct os_task callout_task_struct_receive;
os_stack_t callout_task_stack_receive[CALLOUT_STACK_SIZE];

/* Delearing variables for callout_func */
struct os_callout_func callout_func_test;

/* The event to be sent*/
struct os_eventq callout_evq;
struct os_event callout_ev;

int i; 
int j;
 
/* The callout_stop task */
#define SEND_STOP_CALLOUT_TASK_PRIO        (3)
struct os_task callout_task_struct_stop_send;
os_stack_t callout_task_stack_stop_send[CALLOUT_STACK_SIZE];

#define RECEIVE_STOP_CALLOUT_TASK_PRIO        (4)
struct os_task callout_task_struct_stop_receive;
os_stack_t callout_task_stack_stop_receive[CALLOUT_STACK_SIZE];

/* Delearing variables for callout_stop_func */
#define MULTI_SIZE    (2)
struct os_callout_func callout_func_stop_test[MULTI_SIZE];

/* The event to be sent*/
struct os_eventq callout_stop_evq[MULTI_SIZE];
struct os_event callout_stop_ev;

/* This is the function for callout_init*/
void
my_callout_func(void *arg)
{
    int p;
    p = 4;
    TEST_ASSERT(p == 4);
}

/* This is the function for callout_init*/
void
my_callout_stop_func(void *arg)
{
    int q;
    q = 1;
    TEST_ASSERT(q == 1);
}

/* This is a callout task to send data */
void
callout_task_send()
{
   
    /* should say whether callout is armed or not */
    i= os_callout_queued(&callout_func_test.cf_c);
    TEST_ASSERT(i == 0);

    /* Arm the callout */
    i = os_callout_reset(&callout_func_test.cf_c, OS_TICKS_PER_SEC/ 50);
    TEST_ASSERT_FATAL(i == 0);

    /* should say whether callout is armed or not */
    i = os_callout_queued(&callout_func_test.cf_c);
    TEST_ASSERT(i == 1);

    /* Send the callout */ 
    os_time_delay(OS_TICKS_PER_SEC );
}

/* This is the callout to receive data */
void
callout_task_receive(void *arg)
{
    struct os_event *event;
    /* Recieving using the os_eventq_poll*/
    event = os_eventq_poll(&callout_func_test.cf_c.c_evq, 1, OS_WAIT_FOREVER);
    TEST_ASSERT(event->ev_type ==  OS_EVENT_T_TIMER);
    TEST_ASSERT(event->ev_arg == NULL);

    TEST_ASSERT(i == 1);
    /* should say whether callout is armed or not */
    i = os_callout_queued(&callout_func_test.cf_c);
    TEST_ASSERT(i == 0);

    /* Finishes the test when OS has been started */
    os_test_restart();

}


/* This is callout to send the stop_callout */
void
callout_task_stop_send()
{
     int k;
     //os_time_t tim;

     /* should say whether callout is armed or not */
    for(k = 0; k<MULTI_SIZE; k++){
        j = os_callout_queued(&callout_func_stop_test[k].cf_c);
        TEST_ASSERT(j == 0);
    }

    /* show that  callout is not armed after calling callout_stop */
    for(k = 0; k<MULTI_SIZE; k++){
        os_callout_stop(&callout_func_stop_test[k].cf_c);
        j = os_callout_queued(&callout_func_stop_test[k].cf_c);
        TEST_ASSERT(j == 0);
    }
    /* Assigning data to send*/
    for(k = 0; k<MULTI_SIZE; k++){
        callout_func_stop_test[k].cf_c.c_ev.ev_type = 10+ k;
        callout_func_stop_test[k].cf_c.c_ev.ev_arg = NULL;
    }
    os_time_delay(OS_TICKS_PER_SEC );
}

/* This is the callout to receive stop_callout data */
void
callout_task_stop_receive()
{
    int k;
    struct os_event *event;
    /* Recieving using the os_eventq_poll*/
    for(k=0; k<MULTI_SIZE; k++){
        event = os_eventq_poll(&callout_func_stop_test[k].cf_c.c_evq, 1,
           OS_WAIT_FOREVER);
        TEST_ASSERT(event->ev_type == 10+k);
        TEST_ASSERT(event->ev_arg == NULL);
     }
     
    /* Showing that event is removed from the queued after calling callout_stop */    
    for(k=0; k<MULTI_SIZE; k++){
        os_callout_stop(&callout_func_stop_test[k].cf_c);
        event = os_eventq_poll(&callout_func_stop_test[k].cf_c.c_evq, MULTI_SIZE,
            OS_WAIT_FOREVER);
        TEST_ASSERT(event->ev_type != 10+k);
     }

    /* Finishes the test when OS has been started */
    os_test_restart();

}

TEST_CASE(callout_test)
{

    /* Initializing the OS */
    os_init();
    
    /* Initialize the sending task */
    os_task_init(&callout_task_struct_send, "callout_task_send",
        callout_task_send, NULL, SEND_CALLOUT_TASK_PRIO, OS_WAIT_FOREVER,
        callout_task_stack_send, CALLOUT_STACK_SIZE);

    /* Initialize the receive task */
    os_task_init(&callout_task_struct_receive, "callout_task_receive",
        callout_task_receive, NULL, RECEIVE_CALLOUT_TASK_PRIO, OS_WAIT_FOREVER,
        callout_task_stack_receive, CALLOUT_STACK_SIZE);

    os_eventq_init(&callout_evq);
    
    /* Initialize the callout function */
    os_callout_func_init(&callout_func_test, &callout_evq,
        my_callout_func, NULL);

    /* Does not return until OS_restart is called */
    os_start();

}

/* Test case of the callout_task_stop */
TEST_CASE(callout_test_stop)
{
    int k;
    /* Initializing the OS */
    os_init();

    /* Initialize the sending task */
    os_task_init(&callout_task_struct_stop_send, "callout_task_stop_send",
        callout_task_stop_send, NULL, SEND_STOP_CALLOUT_TASK_PRIO, OS_WAIT_FOREVER,
        callout_task_stack_stop_send, CALLOUT_STACK_SIZE);

    /* Initialize the receiving task */
    os_task_init(&callout_task_struct_stop_receive, "callout_task_stop_receive",
        callout_task_stop_receive, NULL, RECEIVE_STOP_CALLOUT_TASK_PRIO,
            OS_WAIT_FOREVER, callout_task_stack_stop_receive, CALLOUT_STACK_SIZE);

    for(k = 0; k< MULTI_SIZE; k++){
        os_eventq_init(&callout_stop_evq[k]);
    }
    
    /* Initialize the callout function */
    for(k = 0; k<MULTI_SIZE; k++){
        os_callout_func_init(&callout_func_stop_test[k], &callout_stop_evq[k],
           my_callout_stop_func, NULL);
    }

    /* Does not return until OS_restart is called */
    os_start();

}

TEST_SUITE(os_callout_test_suite)
{   
    //callout_test();
    callout_test_stop();
}