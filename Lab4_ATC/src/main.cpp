#include "AirTrafficControl.h"
#include "Radar.h"
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>

uint64_t tick_counter = 0;
std::atomic<bool> running(true);

void timer_tick() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        tick_counter++;
    }
}

// Wrapper function to run radar concurrently
void run_radar() {
    Radar radar(tick_counter);
}

int main() {
    AirTrafficControl atc;

    // IMPORTANT: Path must point to the /tmp/ directory for the target Pi
    atc.readPlanesFromFile("/tmp/planes.txt");

    std::thread timer_thread(timer_tick);

    // 1. Start the Radar in the background FIRST so it's ready to listen
    std::thread radar_thread(run_radar);

    // Give Radar 1 second to initialize its IPC channel and memory
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 2. Start planes (this will block until all planes finish flying)
    atc.startPlanes();

    if (atc.areAllPlanesFinished()) {
        std::cout << "Main function received signal that all aircraft are inactive.\n";
        running = false;
        timer_thread.join();
        radar_thread.detach();
    }

    return 0;
}
