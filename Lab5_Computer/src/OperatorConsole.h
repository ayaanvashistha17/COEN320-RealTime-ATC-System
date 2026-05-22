#ifndef OPERATORCONSOLE_H_
#define OPERATORCONSOLE_H_

#include <iostream>
#include <sys/dispatch.h>
#include <thread>
#include <string>
#include "Msg_structs.h"

class OperatorConsole {
public:
    OperatorConsole();
    ~OperatorConsole();

private:
    void HandleConsoleInputs();
    void logCommand(const std::string& command);

    std::thread Operator_Console;
    bool exit_console; // Renamed to avoid conflict with the standard std::exit() function
};

#endif /* OPERATORCONSOLE_H_ */
