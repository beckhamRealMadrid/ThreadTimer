#include "pch.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include "CThreadTimer.h"
#include "CTaskScheduler.h"

int main()
{
	std::cout << "Starting CThreadTimer test (type 'stop' to exit)..." << std::endl;

	// 타이머 스레드와 태스크 스케줄러 시작
	CTaskScheduler::This().Open();
	CThreadTimer::This().StartThread();

	// 커맨드 입력 대기 루프
	std::string command;
	while (true)
	{
		std::cout << "> ";
		std::getline(std::cin, command);

		if (command == "stop")
		{
			std::cout << "Stopping CThreadTimer and CTaskScheduler..." << std::endl;
			CThreadTimer::This().StopThread();
			CTaskScheduler::This().Close();
			std::cout << "All systems stopped successfully." << std::endl;
			break;
		}
		else if (command == "status")
		{
			std::cout << "Timer thread is running. Type 'stop' to exit." << std::endl;
		}
		else
		{
			std::cout << "Unknown command. Available: 'stop', 'status'" << std::endl;
		}
	}

	std::cout << "CThreadTimer test completed." << std::endl;
	return 0;
}