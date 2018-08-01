#pragma once
#include <thread>

class BTTServer
{
public:
	BTTServer(int ListenPort);
	~BTTServer();
private:
	const WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	int ListenPort = 0;
	SOCKET slisten;

	std::thread loopTh;

};

