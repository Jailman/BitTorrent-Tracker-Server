#include "stdafx.h"
#include "BTTServer.h"
#include <exception>
#include <thread>
#include <vector>
#include <array>
const std::exception SocketException("socket error");

auto SocketConnectionHandler = [](SOCKET sClient,byte* buffer,int totalLenght)
{ 
	delete[] buffer;
	closesocket(sClient);
	return;
};
auto ServerListenThreadFunc = [](SOCKET& slisten, std::vector<BTTServer::HttpRequest>& RequestList, std::mutex& ThreadAccessLocker)
{
	using BufferArray = std::array<byte, 256>;
	while (true)
	{
		try
		{
			SOCKET sClient;
			sockaddr_in remoteAddr;
			int nAddrlen = sizeof(remoteAddr);
			std::vector<BufferArray> RecBuffer;
			BufferArray Tempbuffer;
			//等待TCP连接
			const auto timeout = 3000;
			sClient = accept(slisten, (SOCKADDR *)&remoteAddr, &nAddrlen);
			setsockopt(sClient, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(int));
			if (sClient == INVALID_SOCKET)
			{
				continue;
			}
			//接收数据
			auto RecCount = -1;
			auto TotalLenth = 0;
			while (RecCount != 0)
			{
				RecCount = recv(sClient, (char*)Tempbuffer._Elems, 256, 0);
				if (RecCount == -1)break;
				TotalLenth += RecCount;
				RecBuffer.push_back( Tempbuffer );
				if (RecBuffer.size() > 1024)
				{
					closesocket(sClient);
					std::vector<BufferArray>().swap(RecBuffer);
					throw std::exception("Bad Request");
				}
				const int tempTimeout = 200;
				if(RecCount!=256)
					setsockopt(sClient, SOL_SOCKET, SO_RCVTIMEO, (char *)&tempTimeout, sizeof(int));
			}
			auto buffer = new byte[TotalLenth];
			for (auto i = 0; i < TotalLenth; i++)
			{
				buffer[i] = RecBuffer[i / 256][i % 256];
			}
			ThreadAccessLocker.lock();
			BTTServer::HttpRequest req = { sClient,buffer,TotalLenth };
			RequestList.push_back(req);
			ThreadAccessLocker.unlock();
			//std::thread(SocketConnectionHandler,sClient, buffer, TotalLenth); 这个会因为析构崩溃
		}
		catch(std::exception e){}
	}
};
auto ServerThreadPoolFunc = [](std::vector<BTTServer::HttpRequest>& RequestList,std::mutex& ThreadAccessLocker)
{
	while (true) 
	{
		if (RequestList.size() < 8)Sleep(1);
		if (RequestList.size() == 0)continue;
		ThreadAccessLocker.lock();
		if (RequestList.size() == 0)continue;
		auto req = RequestList[0];
		RequestList.erase(RequestList.begin());
		ThreadAccessLocker.unlock();
		SocketConnectionHandler(req.sClient, req.buffer, req.totalLength);
	}
};
BTTServer::BTTServer(int ListenPort)
{
	auto hr = 0;
	//初始化
	hr = WSAStartup(sockVersion, &wsaData);
	if (hr != 0)throw SocketException;
	slisten = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (slisten == INVALID_SOCKET)throw SocketException;

	this->ListenPort = ListenPort;

	for (auto i = 0; i < threadPool.size(); i++)
	{
		threadPool[i] = std::thread(ServerThreadPoolFunc, std::ref(RequestList), std::ref(ThreadAccessLocker));
	}

	//绑定端口
	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(ListenPort);
	sin.sin_addr.S_un.S_addr = INADDR_ANY;
	if (::bind(slisten, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR)
		throw SocketException;

	//开始监听  
	if (listen(slisten, 5) == SOCKET_ERROR)
		throw SocketException;

	loopTh = std::thread(ServerListenThreadFunc, std::ref(slisten), std::ref(RequestList), std::ref(ThreadAccessLocker));
}


BTTServer::~BTTServer()
{
}
