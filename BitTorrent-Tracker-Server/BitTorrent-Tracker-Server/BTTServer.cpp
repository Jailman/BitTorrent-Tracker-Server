#include "stdafx.h"
#include "BTTServer.h"
#include <exception>
#include <thread>
#include <vector>
#include <array>
const std::exception SocketException("socket error");

auto SocketConnectionHandler = [](SOCKET sclient,byte* buffer,int totalLenght)
{ 
	delete[] buffer;
	return;
};
auto ServerListenThreadFunc = [](SOCKET& slisten)
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
			sClient = accept(slisten, (SOCKADDR *)&remoteAddr, &nAddrlen);
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
				TotalLenth += RecCount;
				RecBuffer.push_back( Tempbuffer );
				if (RecBuffer.size() > 1024)
				{
					closesocket(sClient);
					std::vector<BufferArray>().swap(RecBuffer);
					throw std::exception("Bad Request");
				}
			}
			auto buffer = new byte[TotalLenth];
			for (auto i = 0; i < TotalLenth; i++)
			{
				buffer[i] = RecBuffer[i / 256][i % 256];
			}
			std::thread HandlerTh(SocketConnectionHandler,sClient, buffer, TotalLenth);
		}
		catch(std::exception e){}
	}
};

BTTServer::BTTServer(int ListenPort)
{
	int hr = 0;
	//初始化
	hr = WSAStartup(sockVersion, &wsaData);
	if (hr != 0)throw SocketException;
	slisten = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (slisten == INVALID_SOCKET)throw SocketException;

	this->ListenPort = ListenPort;

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

	loopTh = std::thread(ServerListenThreadFunc, std::ref(slisten));
}


BTTServer::~BTTServer()
{
}
