#include "AirTrafficControl.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

AirTrafficControl::AirTrafficControl() {}

AirTrafficControl::~AirTrafficControl() {
    for (Aircraft* plane : planes) {
        delete plane;
    }
    planes.clear();
}

void AirTrafficControl::readPlanesFromFile(const std::string& fileName) {
    std::ifstream file(fileName);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << fileName << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        PlaneData data{};

        ss >> data.arrivaTime >> data.id >> data.posX >> data.posY >> data.posZ
           >> data.speedX >> data.speedY >> data.speedZ;

        if (ss.fail()) {
            std::cerr << "Error parsing line: " << line << std::endl;
            continue;
        }

        planeData.push_back(data);
    }

    file.close();
}

void AirTrafficControl::startPlanes() {
    for (const auto& data : planeData) {
        std::cout << "Creating Aircraft " << data.id << ": "
                  << "Pos(" << data.posX << ", " << data.posY << ", " << data.posZ << ") "
                  << "Speed(" << data.speedX << ", " << data.speedY << ", " << data.speedZ << ") "
                  << "ArrivalTime(" << data.arrivaTime << ")\n";

        Aircraft* plane = new Aircraft(data.id, data.posX, data.posY, data.posZ,
                                       data.speedX, data.speedY, data.speedZ, data.arrivaTime);
        planes.push_back(plane);
    }

    for (Aircraft* Plane : planes) {
        if (pthread_join(Plane->thread_id, nullptr) != 0) {
            std::cerr << "Error: pthread_join failed for Aircraft " << Plane->getID() << std::endl;
            exit(1);
        }
    }

    allPlanesFinished = true;
    std::cout << "All aircraft have finished their tasks and are no longer active.\n";
}

bool AirTrafficControl::areAllPlanesFinished() const {
    return allPlanesFinished;
}
