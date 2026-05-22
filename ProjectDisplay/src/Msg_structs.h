#ifndef MSG_STRUCTS_H_
#define MSG_STRUCTS_H_

#include <atomic>
#include <array>
#include <cstddef>
#include <cstdint>

// =========================
// CHANGE THIS TO YOUR GROUP
// =========================
#define GROUP_TAG "ayaan_group"

// Common names
#define RADAR_CHANNEL_NAME   GROUP_TAG "_Radar"
#define DISPLAY_CHANNEL_NAME GROUP_TAG "_Display"
#define SHM_NAME             "/" GROUP_TAG "_radar_shared_mem"

enum class MessageType {
    ENTER_AIRSPACE,
    EXIT_AIRSPACE,
    POSITION_UPDATE,
    REQUEST_POSITION,
    REQUEST_CHANGE_OF_HEADING,
    REQUEST_CHANGE_POSITION,
    REQUEST_CHANGE_ALTITUDE,
    REQUEST_AUGMENTED_INFO,
    CHANGE_TIME_CONSTRAINT_COLLISIONS,
    EXIT,
    COLLISION_DETECTED
};

struct Message {
    bool header;          // false = intra-process style message
    MessageType type;
    int planeID;
    void* data;
    size_t dataSize;
};

typedef struct {
    int id;
    double PositionX, PositionY, PositionZ;
    double VelocityX, VelocityY, VelocityZ;
} msg_plane_info;

typedef struct {
    int ID;
    double VelocityX, VelocityY, VelocityZ;
    double altitude;
} msg_change_heading;

typedef struct {
    double x, y, z;
} msg_change_position;

// Shared memory structure
struct SharedMemory {
    msg_plane_info plane_data[100];
    int count;
    std::atomic<bool> is_empty;
    bool start;
    uint64_t timestamp;
};

struct Message_inter_process {
    bool header; // true = interprocess
    MessageType type;
    int planeID;
    std::array<char, 256> data;
    size_t dataSize;
};

#endif /* MSG_STRUCTS_H_ */
