#include "ComputerSystem.h"
#include "ATCTimer.h"
#include <iostream>
#include <cmath>
#include <sys/dispatch.h>
#include <cstring>

ComputerSystem::ComputerSystem() : shm_fd(-1), shared_mem(nullptr), running(false) {}

ComputerSystem::~ComputerSystem() {
    running = false;
    joinThread();
    cleanupSharedMemory();
}

bool ComputerSystem::initializeSharedMemory() {
    while (true) {
        shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
        if (shm_fd == -1) {
            usleep(200000);
            continue;
        }

        shared_mem = static_cast<SharedMemory*>(
            mmap(nullptr, sizeof(SharedMemory), PROT_READ, MAP_SHARED, shm_fd, 0)
        );

        if (shared_mem == MAP_FAILED) {
            perror("ComputerSystem mmap failed");
            shared_mem = nullptr;
            close(shm_fd);
            shm_fd = -1;
            usleep(200000);
            continue;
        }
        std::cout << "ComputerSystem: Shared memory initialized successfully." << std::endl;
        return true;
    }
}

void ComputerSystem::cleanupSharedMemory() {
    if (shared_mem && shared_mem != MAP_FAILED) {
        munmap(shared_mem, sizeof(SharedMemory));
    }
    if (shm_fd != -1) {
        close(shm_fd);
    }
}

bool ComputerSystem::startMonitoring() {
    if (initializeSharedMemory()) {
        running = true;
        std::cout << "ComputerSystem: Starting monitoring thread." << std::endl;
        monitorThread = std::thread(&ComputerSystem::monitorAirspace, this);
        return true;
    }
    return false;
}

void ComputerSystem::joinThread() {
    if (monitorThread.joinable()) {
        monitorThread.join();
    }
}

void ComputerSystem::monitorAirspace() {
    ATCTimer timer(1, 0);
    std::vector<msg_plane_info> plane_data_vector;
    uint64_t timestamp = 0;

    while (running && shared_mem->is_empty.load()) {
        timer.waitTimer();
    }

    while (running) {
        if (shared_mem->is_empty.load()) {
            std::cout << "ComputerSystem: No planes in airspace. Stopping monitoring.\n";
            running = false;
            break;
        } else {
            plane_data_vector.clear();
            timestamp = shared_mem->timestamp;

            for (int i = 0; i < shared_mem->count; ++i) {
                plane_data_vector.push_back(shared_mem->plane_data[i]);
            }
        }

        if (plane_data_vector.size() > 1) {
            checkCollision(timestamp, plane_data_vector);
        }
        timer.waitTimer();
    }
}

bool ComputerSystem::checkAxes(msg_plane_info plane1, msg_plane_info plane2) {
    double dx = std::abs(plane1.PositionX - plane2.PositionX);
    double dy = std::abs(plane1.PositionY - plane2.PositionY);
    double dz = std::abs(plane1.PositionZ - plane2.PositionZ);

    return (dx <= CONSTRAINT_X && dy <= CONSTRAINT_Y && dz <= CONSTRAINT_Z);
}

void ComputerSystem::checkCollision(uint64_t currentTime, std::vector<msg_plane_info> planes) {
    std::vector<std::pair<int, int>> collisionPairs;

    for (size_t i = 0; i < planes.size(); ++i) {
        for (size_t j = i + 1; j < planes.size(); ++j) {
            if (checkAxes(planes[i], planes[j])) {
                collisionPairs.emplace_back(planes[i].id, planes[j].id);
            }
        }
    }

    if (!collisionPairs.empty()) {
        Message_inter_process msg_to_send{};
        msg_to_send.header = true;
        msg_to_send.planeID = -1;
        msg_to_send.type = MessageType::COLLISION_DETECTED;

        size_t dataSize = collisionPairs.size() * sizeof(std::pair<int, int>);
        msg_to_send.dataSize = dataSize;
        std::memcpy(msg_to_send.data.data(), collisionPairs.data(), dataSize);

        sendCollisionToDisplay(msg_to_send);
    }
}

void ComputerSystem::sendCollisionToDisplay(const Message_inter_process& msg) {
    // CRITICAL FIX: static variable keeps the channel open forever across multiple function calls
    static int display_channel = -1;

    // Only open the channel the very first time
    if (display_channel == -1) {
        display_channel = name_open(DISPLAY_CHANNEL_NAME, 0);
    }

    // If the Display isn't running yet, just abort safely
    if (display_channel == -1) {
        return;
    }

    int reply = 0;
    MsgSend(display_channel, &msg, sizeof(msg), &reply, sizeof(reply));

    // name_close(display_channel); <-- Removed to prevent QNX pulse flooding and timer deadlock!
}
