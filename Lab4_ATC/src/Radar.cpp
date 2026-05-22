#include "Radar.h"
#include <iostream>
#include <vector>
#include <string>
#include <sys/dispatch.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <unordered_map>
#include <cstring>
#include "Msg_structs.h"

using namespace std;

Radar::Radar(uint64_t& currentTime) : tick_counter_ref(currentTime), timer(1, 0) {
    cout << "Radar System Starting...\n";

    // 1. Initialize Shared Memory
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Radar: Failed to create shared memory");
        return;
    }

    ftruncate(shm_fd, sizeof(SharedMemory));
    SharedMemory* shared_mem = (SharedMemory*)mmap(0, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_mem == MAP_FAILED) {
        perror("Radar: mmap failed");
        return;
    }

    shared_mem->count = 0;
    shared_mem->is_empty.store(true);
    shared_mem->start = true;
    shared_mem->timestamp = 0;

    // 2. Setup IPC Channel
    // We attempt to open and close the name first to see if a zombie exists
    int zombie_check = name_open(RADAR_CHANNEL_NAME, 0);
    if (zombie_check != -1) {
        name_close(zombie_check);
    }

    // Correct type: name_attach_t
    name_attach_t* attach = name_attach(NULL, RADAR_CHANNEL_NAME, 0);
    if (attach == NULL) {
        // If it still fails, the OS is holding it. This is where 'slay' is needed.
        perror("Radar: name_attach failed. Try 'slay' in the terminal");
        return;
    }

    vector<int> active_planes;
    unordered_map<int, int> plane_coids;
    bool planes_have_entered = false;

    // 3. Main Operational Loop
    while (true) {
        Message msg;
        int rcvid;

        uint64_t timeout = 1000000000ULL;
        TimerTimeout(CLOCK_REALTIME, _NTO_TIMEOUT_RECEIVE, NULL, &timeout, NULL);
        rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), NULL);

        if (rcvid > 0) {
            uint16_t msg_type = *reinterpret_cast<uint16_t*>(&msg);
            if (msg_type >= _IO_BASE && msg_type <= _IO_MAX) {
                MsgReply(rcvid, EOK, NULL, 0);
                continue;
            }

            if (msg.type == MessageType::ENTER_AIRSPACE) {
                cout << "Radar: Plane " << msg.planeID << " entered airspace.\n";
                active_planes.push_back(msg.planeID);
                planes_have_entered = true;
                MsgReply(rcvid, EOK, NULL, 0);
            }
            else if (msg.type == MessageType::EXIT_AIRSPACE) {
                cout << "Radar: Plane " << msg.planeID << " exited airspace.\n";
                active_planes.erase(remove(active_planes.begin(), active_planes.end(), msg.planeID), active_planes.end());

                if (plane_coids.count(msg.planeID)) {
                    name_close(plane_coids[msg.planeID]);
                    plane_coids.erase(msg.planeID);
                }
                MsgReply(rcvid, EOK, NULL, 0);

                if (planes_have_entered && active_planes.empty()) break;
            } else {
                MsgReply(rcvid, EOK, NULL, 0);
            }

        } else if (rcvid == -1 && errno == ETIMEDOUT) {
            vector<msg_plane_info> current_positions;

            for (int pid : active_planes) {
                if (plane_coids.find(pid) == plane_coids.end()) {
                    string ch_name = string(GROUP_TAG) + "_" + to_string(pid);
                    int coid = name_open(ch_name.c_str(), 0);
                    if (coid != -1) plane_coids[pid] = coid;
                }

                int coid = plane_coids[pid];
                if (coid != -1) {
                    Message req{};
                    req.header = false;
                    req.type = MessageType::REQUEST_POSITION;
                    req.planeID = pid;

                    Message_inter_process reply{};
                    if (MsgSend(coid, &req, sizeof(req), &reply, sizeof(reply)) != -1) {
                        if (reply.type == MessageType::POSITION_UPDATE) {
                            msg_plane_info info;
                            memcpy(&info, reply.data.data(), sizeof(msg_plane_info));
                            current_positions.push_back(info);
                        }
                    }
                }
            }

            shared_mem->count = (int)current_positions.size();
            for (size_t i = 0; i < current_positions.size(); i++) {
                shared_mem->plane_data[i] = current_positions[i];
            }
            shared_mem->timestamp = tick_counter_ref;
            shared_mem->is_empty.store(current_positions.empty());
        }
    }

    cout << "Radar System closing cleanly...\n";
    shared_mem->is_empty.store(true);
    munmap(shared_mem, sizeof(SharedMemory));
    shm_unlink(SHM_NAME);
    name_detach(attach, 0);
}

Radar::~Radar() {}
