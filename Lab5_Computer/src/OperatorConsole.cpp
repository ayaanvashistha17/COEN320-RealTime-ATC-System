#include "OperatorConsole.h"
#include "CommunicationsSystem.h"
#include <fstream>
#include <string>

// Constructor starts the interactive console in its own thread
OperatorConsole::OperatorConsole() : exit_console(false) {
    Operator_Console = std::thread(&OperatorConsole::HandleConsoleInputs, this);
}

// Destructor signals the thread to close and waits for it
OperatorConsole::~OperatorConsole() {
    exit_console = true;
    if (Operator_Console.joinable()) {
        Operator_Console.join();
    }
}

// Logs the commands to a file on the QNX target
void OperatorConsole::logCommand(const std::string& command) {
    std::ofstream log("/tmp/operator_console.log", std::ios::app);
    if (log.is_open()) {
        log << command << "\n";
    }
}

// The main loop for user interaction
void OperatorConsole::HandleConsoleInputs() {
    CommunicationsSystem comms;

    // Uses the renamed boolean flag
    while (!exit_console) {
        std::cout << "\n=== Operator Console ===\n";
        std::cout << "1. Change heading (velocity)\n";
        std::cout << "2. Change position\n";
        std::cout << "3. Change altitude\n";
        std::cout << "4. Exit Console\n";
        std::cout << "Choice: ";

        int choice = 0;
        std::cin >> choice;

        if (choice == 1) {
            int id; double vx, vy, vz;
            std::cout << "Plane ID: "; std::cin >> id;
            std::cout << "New Vx Vy Vz (space separated): "; std::cin >> vx >> vy >> vz;
            if (comms.sendHeadingChange(id, vx, vy, vz)) {
                logCommand("Changed heading of plane " + std::to_string(id));
            }
        } else if (choice == 2) {
            int id; double x, y, z;
            std::cout << "Plane ID: "; std::cin >> id;
            std::cout << "New X Y Z (space separated): "; std::cin >> x >> y >> z;
            if (comms.sendPositionChange(id, x, y, z)) {
                logCommand("Changed position of plane " + std::to_string(id));
            }
        } else if (choice == 3) {
            int id; double alt;
            std::cout << "Plane ID: "; std::cin >> id;
            std::cout << "New Altitude (Z): "; std::cin >> alt;
            if (comms.sendAltitudeChange(id, alt)) {
                logCommand("Changed altitude of plane " + std::to_string(id));
            }
        } else if (choice == 4) {
            exit_console = true;
        } else {
            std::cout << "Invalid choice.\n";
        }
    }
}
