#include <csignal>
#include <atomic>
#include <iostream>
#include <unistd.h>

#include "Program.h"
#include "IO/Display.h"

std::atomic<bool> isRunning(true);

void SignalHandler(int signum)
{
    static int callCount = 0;
    if (callCount++ > 0)
    {
        std::cout << "\nForced exit!\n";
        std::_Exit(signum);
    }

    std::cout << "\nTermination signal (" << signum << ") received. Shutting down display...\n";
    
    isRunning = false; 
    
    usleep(50000);

	Program::GetInstance()->GetDisplay()->Shutdown();
	std::exit(0);
}

int main(void)
{
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    Program* program = Program::GetInstance();
    program->Init();
    program->Start();

    return 0;
}