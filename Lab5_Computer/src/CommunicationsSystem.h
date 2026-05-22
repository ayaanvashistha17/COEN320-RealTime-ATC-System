#ifndef SRC_COMMUNICATIONSSYSTEM_H_
#define SRC_COMMUNICATIONSSYSTEM_H_

#include <iostream>
#include <string>
#include <sys/dispatch.h>
#include "Msg_structs.h"

class CommunicationsSystem {
public:
    CommunicationsSystem();
    ~CommunicationsSystem();

    bool sendHeadingChange(int planeID, double vx, double vy, double vz);
    bool sendPositionChange(int planeID, double x, double y, double z);
    bool sendAltitudeChange(int planeID, double altitude);

private:
    bool messageAircraft(const Message_inter_process& msg, int planeID);
};

#endif /* SRC_COMMUNICATIONSSYSTEM_H_ */
