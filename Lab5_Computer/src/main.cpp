#include "ComputerSystem.h"
#include "OperatorConsole.h"

int main() {
    ComputerSystem computerSystem;

    // Instantiate the operator console (it starts its own thread in the constructor)
    OperatorConsole console;

    if (computerSystem.startMonitoring()) {
        // Blocks until the shared memory signals that all planes have left
        computerSystem.joinThread();
    } else {
        std::cerr << "Failed to start monitoring." << std::endl;
    }

    std::cout << "Monitoring stopped. Exiting main." << std::endl;
    return 0;
}
