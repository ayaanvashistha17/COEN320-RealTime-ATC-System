#include <iostream>
#include <iomanip>
#include <memory>
#include <pthread.h>
#include <cstring>
#include <cstdlib>
#include <sys/iomsg.h>
#include "Aircraft.h"
#include "ATCTimer.h"

void* updatePositionThread(void* arg) {
    Aircraft* aircraft = static_cast<Aircraft*>(arg);
    return reinterpret_cast<void*>((intptr_t)aircraft->updatePosition());
}

Aircraft::Aircraft(int id, double x, double y, double z, double sx, double sy, double sz, int t)
    : id(id), posX(x), posY(y), posZ(z), speedX(sx), speedY(sy), speedZ(sz), arrivalTime(t), inAirspace(true) {
    message_id = -1; Radar_id = -1; airspace = {0, 100000, 0, 100000, 15000, 40000};
    if (pthread_create(&thread_id, nullptr, updatePositionThread, (void*)this) != 0) {
        perror("pthread_create failed for Aircraft");
    }
}

Aircraft::~Aircraft() {}

void Aircraft::printInitialAircraftData() const { }

void Aircraft::changeHeading(double Vx, double Vy, double Vz) { speedX = Vx; speedY = Vy; speedZ = Vz; }

std::string Aircraft::getPlaneChannelName() const {
    return std::string(GROUP_TAG) + "_" + std::to_string(id);
}

int Aircraft::updatePosition() {
    ATCTimer timer(1, 0);
    int currentTime = 0;

    // 1. Initial Delay using ATCTimer
    while (currentTime < arrivalTime) {
        timer.waitTimer();
        ++currentTime;
    }

    // 2. Register with Radar
    if ((Radar_id = name_open(RADAR_CHANNEL_NAME, 0)) == -1) return EXIT_FAILURE;

    Message enterAirspaceMessage{};
    enterAirspaceMessage.header = false;
    enterAirspaceMessage.type = MessageType::ENTER_AIRSPACE;
    enterAirspaceMessage.planeID = id;

    if (MsgSend(Radar_id, &enterAirspaceMessage, sizeof(enterAirspaceMessage), 0, 0) == -1) {
        name_close(Radar_id); return EXIT_FAILURE;
    }

    std::string id_str = getPlaneChannelName();
    name_attach_t* Plane_channel = name_attach(NULL, id_str.c_str(), 0);
    if (Plane_channel == NULL) { name_close(Radar_id); return EXIT_FAILURE; }

    // 3. Main Event Loop (Event-driven, no waitTimer to avoid deadlock)
    while (true) {
        char buffer[sizeof(Message_inter_process)];
        int rcvid = MsgReceive(Plane_channel->chid, buffer, sizeof(buffer), NULL);

        if (rcvid > 0) {
            // Handle hidden QNX system messages
            uint16_t msg_type = *reinterpret_cast<uint16_t*>(buffer);
            if (msg_type >= _IO_BASE && msg_type <= _IO_MAX) {
                MsgReply(rcvid, EOK, NULL, 0);
                continue;
            }

            bool isInterProcess = reinterpret_cast<Message_inter_process*>(buffer)->header;
            if (isInterProcess) {
                // Operator Commands
                Message_inter_process* receivedMsg = reinterpret_cast<Message_inter_process*>(buffer);
                switch (receivedMsg->type) {
                    case MessageType::REQUEST_CHANGE_OF_HEADING: {
                        msg_change_heading cmd{}; std::memcpy(&cmd, receivedMsg->data.data(), sizeof(cmd));
                        changeHeading(cmd.VelocityX, cmd.VelocityY, cmd.VelocityZ);
                        int reply = 0; MsgReply(rcvid, 0, &reply, sizeof(reply)); break;
                    }
                    case MessageType::REQUEST_CHANGE_POSITION: {
                        msg_change_position cmd{}; std::memcpy(&cmd, receivedMsg->data.data(), sizeof(cmd));
                        posX = cmd.x; posY = cmd.y; posZ = cmd.z;
                        int reply = 0; MsgReply(rcvid, 0, &reply, sizeof(reply)); break;
                    }
                    case MessageType::REQUEST_CHANGE_ALTITUDE: {
                        double newAltitude = 0.0; std::memcpy(&newAltitude, receivedMsg->data.data(), sizeof(double));
                        posZ = newAltitude;
                        int reply = 0; MsgReply(rcvid, 0, &reply, sizeof(reply)); break;
                    }
                    case MessageType::EXIT: {
                        int reply = 0; MsgReply(rcvid, 0, &reply, sizeof(reply));
                        name_detach(Plane_channel, 0); name_close(Radar_id); pthread_exit(NULL); break;
                    }
                    default: { int reply = -1; MsgReply(rcvid, 0, &reply, sizeof(reply)); break; }
                }
            } else {
                // Radar Commands
                Message* receivedMsg = reinterpret_cast<Message*>(buffer);
                if (receivedMsg->type == MessageType::REQUEST_POSITION) {

                    // A. Pack the current position and reply to the Radar
                    Message_inter_process reply_msg{};
                    reply_msg.header = false;
                    reply_msg.type = MessageType::POSITION_UPDATE;
                    reply_msg.planeID = id;

                    msg_plane_info info{id, posX, posY, posZ, speedX, speedY, speedZ};
                    reply_msg.dataSize = sizeof(msg_plane_info);
                    std::memcpy(reply_msg.data.data(), &info, sizeof(msg_plane_info));

                    MsgReply(rcvid, 0, &reply_msg, sizeof(reply_msg));

                    // B. TICK THE PLANE FORWARD (Exactly 1 second of movement)
                    posX += speedX; posY += speedY; posZ += speedZ;

                    // C. CHECK BOUNDARIES
                    if (posX < airspace.lower_x_boundary || posX > airspace.upper_x_boundary ||
                        posY < airspace.lower_y_boundary || posY > airspace.upper_y_boundary ||
                        posZ < airspace.lower_z_boundary || posZ > airspace.upper_z_boundary) {

                        Message exitAirspaceMessage{};
                        exitAirspaceMessage.header = false;
                        exitAirspaceMessage.type = MessageType::EXIT_AIRSPACE;
                        exitAirspaceMessage.planeID = id;
                        MsgSend(Radar_id, &exitAirspaceMessage, sizeof(exitAirspaceMessage), 0, 0);

                        name_detach(Plane_channel, 0); name_close(Radar_id); pthread_exit(NULL);
                        return 0;
                    }
                }
            }
        }
    }
    return 0;
}

int Aircraft::getArrivalTime() { return arrivalTime; }
int Aircraft::getID() { return id; }

Message Aircraft::createEnterAirspaceMessage(int planeID) { Message msg{}; return msg; }
Message Aircraft::createExitAirspaceMessage(int planeID) { Message msg{}; return msg; }
Message Aircraft::createPositionUpdateMessage(int planeID, const msg_plane_info& info) { Message msg{}; return msg; }
