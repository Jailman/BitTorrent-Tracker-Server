#pragma once
#include <thread>
#include <array>
#include <mutex>
#include <vector>

#include "Event.h"
namespace Utils::HttpServer
{
	class HttpServer
	{
	public:
		HttpServer(int ListenPort);
		~HttpServer();
		struct HttpRequest
		{
			SOCKET sClient;
			byte* buffer;
			int totalLength;
		};
	private:
		const WORD sockVersion = MAKEWORD(2, 2);
		WSADATA wsaData;
		int ListenPort = 0;
		SOCKET slisten;
	private:
		std::thread loopTh;
		std::array<std::thread, 8> threadPool;
		std::vector<HttpRequest> RequestList;
		std::mutex ThreadAccessLocker;
	};
}

