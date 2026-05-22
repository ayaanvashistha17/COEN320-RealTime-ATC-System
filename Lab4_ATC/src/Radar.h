#ifndef RADAR_H
#define RADAR_H

#include <atomic>
#include <iostream>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <thread>
#include <string>
#include <vector>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include "Aircraft.h"
#include "Msg_structs.h"
#include "ATCTimer.h"

#define SHARED_MEMORY_SIZE sizeof(SharedMemory)

class Radar {
public:
    Radar(uint64_t& tick_counter);
    ~Radar();

    void ListenAirspaceArrivalAndDeparture();
    void ListenUpdatePosition();

    void writeToSharedMemory();
    void clearSharedMemory();

private:
    uint64_t& tick_counter_ref;
    std::unordered_set<int> planesInAirspace;

    std::thread Arrival_Departure;
    std::thread UpdatePosition;

    void addPlaneToAirspace(Message msg);
    void removePlaneFromAirspace(int ID);
    void pollAirspace();
    msg_plane_info getAircraftData(int id);

    name_attach_t* Radar_channel;

    std::mutex airspaceMutex;
    std::mutex bufferSwitchMutex;

    std::vector<msg_plane_info>& getActiveBuffer();

    std::vector<msg_plane_info> planesInAirspaceData[2];
    std::atomic<int> activeBufferIndex;

    ATCTimer timer;

    SharedMemory* sharedMemPtr;
    bool wasAirspaceEmpty;
    int shm_fd;
    std::atomic<bool> stopThreads;

    void shutdown();
};

#endif /* RADAR_H_ */
