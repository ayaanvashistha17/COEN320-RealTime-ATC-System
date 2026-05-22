#include <iostream>
#include <iomanip>
#include <vector>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/dispatch.h>
#include <cstring>
#include <chrono> // Needed for the frame limiter
#include "Msg_structs.h"

using namespace std;

// Function to draw the static dashboard without breaking Momentics console
void drawDashboard(SharedMemory* shm, const vector<pair<int, int>>& collisions) {
    // Use newlines to push the old grid up
    cout << "\n\n\n\n\n";

    cout << "=============================================================\n";
    cout << "                 ATC LIVE AIRSPACE DASHBOARD                 \n";
    cout << "=============================================================\n";
    cout << left << setw(6) << "ID"
         << setw(10) << "X" << setw(10) << "Y" << setw(10) << "Z"
         << setw(10) << "Vx" << setw(10) << "Vy" << setw(10) << "Vz" << "\n";
    cout << "-------------------------------------------------------------\n";

    if (shm->is_empty.load()) {
        cout << " No aircraft currently in airspace.\n";
    } else {
        for (int i = 0; i < shm->count; ++i) {
            auto p = shm->plane_data[i];
            cout << left << setw(6) << p.id
                 << setw(10) << p.PositionX << setw(10) << p.PositionY << setw(10) << p.PositionZ
                 << setw(10) << p.VelocityX << setw(10) << p.VelocityY << setw(10) << p.VelocityZ << "\n";
        }
    }

    cout << "\n=============================================================\n";
    cout << "                    COLLISION WARNINGS                       \n";
    cout << "=============================================================\n";

    if (collisions.empty()) {
        cout << "  ✓ No active collision warnings.\n";
    } else {
        for (auto c : collisions) {
            cout << "  *** COLLISION WARNING: Aircraft " << c.first << " and " << c.second << " ***\n";
        }
    }
    cout << "=============================================================\n";
}

int main() {
    cout << "Starting Display System...\n";

    // Connect to Radar Shared Memory
    int shm_fd = -1;
    while (shm_fd == -1) {
        shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
        if (shm_fd == -1) {
            cout << "Display: Waiting for Radar to initialize airspace...\n";
            sleep(1);
        }
    }

    SharedMemory* shared_mem = (SharedMemory*)mmap(0, sizeof(SharedMemory), PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shared_mem == MAP_FAILED) {
        perror("Display: mmap failed");
        return EXIT_FAILURE;
    }

    // Create IPC Channel for Warnings
    name_attach_t* attach = name_attach(NULL, DISPLAY_CHANNEL_NAME, 0);
    if (attach == NULL) {
        perror("Display: Failed to attach IPC name");
        return EXIT_FAILURE;
    }

    vector<pair<int, int>> active_collisions;

    // IDE Rendering Limiter Time Tracker
    auto last_draw = std::chrono::steady_clock::now();

    // Main Display Loop
    while (true) {
        Message_inter_process msg;
        uint64_t timeout = 1000000000ULL; // 1 second timeout
        TimerTimeout(CLOCK_REALTIME, _NTO_TIMEOUT_RECEIVE, NULL, &timeout, NULL);
        int rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), NULL);

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_draw).count();

        if (rcvid > 0) {
            uint16_t msg_type = *reinterpret_cast<uint16_t*>(&msg);
            if (msg_type >= _IO_BASE && msg_type <= _IO_MAX) {
                MsgReply(rcvid, EOK, NULL, 0);
                continue;
            }

            if (msg.type == MessageType::COLLISION_DETECTED) {
                active_collisions.clear();
                int numPairs = msg.dataSize / sizeof(pair<int, int>);
                pair<int, int>* pairs = reinterpret_cast<pair<int, int>*>(msg.data.data());

                for (int i = 0; i < numPairs; i++) {
                    active_collisions.push_back(pairs[i]);
                }
            }
            MsgReply(rcvid, EOK, NULL, 0);

            // IDE FRAME LIMITER: Only draw if 1 full second has passed
            // This prevents the terminal from choking during a collision spam
            if (elapsed >= 1000) {
                drawDashboard(shared_mem, active_collisions);
                last_draw = now;
            }

        } else if (rcvid == -1 && errno == ETIMEDOUT) {
            // Normal 1-second interval with no collision messages
            active_collisions.clear();
            drawDashboard(shared_mem, active_collisions);
            last_draw = std::chrono::steady_clock::now();
        }
    }

    return 0;
}
