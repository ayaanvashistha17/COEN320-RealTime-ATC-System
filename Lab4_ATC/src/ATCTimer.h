#ifndef ATCTIMER_H_
#define ATCTIMER_H_

#include <stdio.h>
#include <iostream>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sync.h>
#include <sys/siginfo.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include <sys/syspage.h>
#include <inttypes.h>
#include <stdint.h>

class ATCTimer {
    int channel_id;
    int connection_id;

    struct sigevent sig_event;
    struct itimerspec timer_spec;
    timer_t timer_id;

    char msg_buffer[100];

    uint64_t cycles_per_sec;
    uint64_t tick_cycles, tock_cycles;

public:
    ATCTimer(uint32_t sec, uint32_t msec);
    void setTimerSpecification(uint32_t sec, uint32_t nano);
    void waitTimer();
    void startTimer();
    void tick();
    double tock();
    virtual ~ATCTimer();
};

#endif /* ATCTIMER_H_ */
