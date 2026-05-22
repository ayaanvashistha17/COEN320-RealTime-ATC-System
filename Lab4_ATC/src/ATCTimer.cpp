#include "ATCTimer.h"

// Constructor to initialize timer with seconds and milliseconds
ATCTimer::ATCTimer(uint32_t sec, uint32_t msec) {
    channel_id = ChannelCreate(0);
    if (channel_id == -1) {
        std::cerr << "Timer, ChannelCreate error: " << errno << "\n";
    }

    connection_id = ConnectAttach(0, 0, channel_id, 0, 0);
    if (connection_id == -1) {
        std::cerr << "Timer, ConnectAttach error: " << errno << "\n";
    }

    SIGEV_PULSE_INIT(&sig_event, connection_id, SIGEV_PULSE_PRIO_INHERIT, 1, 0);

    if (timer_create(CLOCK_REALTIME, &sig_event, &timer_id) == -1) {
        std::cerr << "Timer, timer_create error: " << errno << "\n";
    }

    setTimerSpecification(sec, 1000000 * msec); // ms -> ns
    cycles_per_sec = SYSPAGE_ENTRY(qtime)->cycles_per_sec;
}

ATCTimer::~ATCTimer() {
    timer_delete(timer_id);
    if (connection_id != -1) ConnectDetach(connection_id);
    if (channel_id != -1) ChannelDestroy(channel_id);
}

void ATCTimer::startTimer() {
    timer_settime(timer_id, 0, &timer_spec, NULL);
}

void ATCTimer::setTimerSpecification(uint32_t sec, uint32_t nano) {
    timer_spec.it_value.tv_sec = sec;
    timer_spec.it_value.tv_nsec = nano;

    timer_spec.it_interval.tv_sec = sec;
    timer_spec.it_interval.tv_nsec = nano;

    timer_settime(timer_id, 0, &timer_spec, NULL);
}

void ATCTimer::waitTimer() {
    int receive_message_id = MsgReceive(channel_id, &msg_buffer, sizeof(msg_buffer), NULL);
    (void)receive_message_id;
}

void ATCTimer::tick() {
    tick_cycles = ClockCycles();
}

double ATCTimer::tock() {
    tock_cycles = ClockCycles();
    return (double)(tock_cycles - tick_cycles) / cycles_per_sec * 1000.0;
}
