#include "CommunicationsSystem.h"
#include <cstring>

CommunicationsSystem::CommunicationsSystem() {}
CommunicationsSystem::~CommunicationsSystem() {}

bool CommunicationsSystem::messageAircraft(const Message_inter_process& msg, int planeID) {
    std::string channelName = std::string(GROUP_TAG) + "_" + std::to_string(planeID);
    int plane_channel = name_open(channelName.c_str(), 0);

    if (plane_channel == -1) {
        std::cerr << "Comms: Could not find plane " << planeID << "\n";
        return false;
    }

    int reply = 0;
    int status = MsgSend(plane_channel, &msg, sizeof(msg), &reply, sizeof(reply));
    name_close(plane_channel);

    return status != -1;
}

bool CommunicationsSystem::sendHeadingChange(int planeID, double vx, double vy, double vz) {
    Message_inter_process msg{};
    msg.header = true;
    msg.type = MessageType::REQUEST_CHANGE_OF_HEADING;
    msg.planeID = planeID;

    msg_change_heading heading{planeID, vx, vy, vz, 0};
    msg.dataSize = sizeof(heading);
    std::memcpy(msg.data.data(), &heading, sizeof(heading));

    return messageAircraft(msg, planeID);
}

bool CommunicationsSystem::sendPositionChange(int planeID, double x, double y, double z) {
    Message_inter_process msg{};
    msg.header = true;
    msg.type = MessageType::REQUEST_CHANGE_POSITION;
    msg.planeID = planeID;

    msg_change_position pos{x, y, z};
    msg.dataSize = sizeof(pos);
    std::memcpy(msg.data.data(), &pos, sizeof(pos));

    return messageAircraft(msg, planeID);
}

bool CommunicationsSystem::sendAltitudeChange(int planeID, double altitude) {
    Message_inter_process msg{};
    msg.header = true;
    msg.type = MessageType::REQUEST_CHANGE_ALTITUDE;
    msg.planeID = planeID;

    msg.dataSize = sizeof(double);
    std::memcpy(msg.data.data(), &altitude, sizeof(double));

    return messageAircraft(msg, planeID);
}
