#pragma once
#include <thread>
#include <array>
#include <mutex>
#include <vector>

#include "Event.h"
namespace Utils::HttpServer
{
	constexpr int ThreadPoolSize = 16;
	class HttpServer
	{
	public:
		HttpServer(int ListenPort);
		~HttpServer();
		struct HttpSocketRequest
		{
			SOCKET sClient;
			byte* buffer;
			int totalLength;
		};
		class HttpGetRequset 
		{
			HttpGetRequset(const HttpSocketRequest& req) {}
		};
		class HttpPostRequset {};
	private:
		const WORD sockVersion = MAKEWORD(2, 2);
		WSADATA wsaData;
		int ListenPort = 0;
		SOCKET slisten;
	private:
		std::thread loopTh;
		std::array<std::thread, ThreadPoolSize> threadPool;
		std::vector<HttpSocketRequest> RequestList;
		std::mutex ThreadAccessLocker;
	public:
		using EventHttpSocketRequestReceved = Event::Event<HttpSocketRequest>;
		EventHttpSocketRequestReceved HttpSocketRequestRecevedEvent;
	};
	using HttpSocketRequest = HttpServer::HttpSocketRequest;

}

