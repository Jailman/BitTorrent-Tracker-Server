#include "stdafx.h"
#include "HttpServer.h"
namespace Utils::HttpServer
{
	const std::exception SocketException("socket error");

	auto ServerListenThreadFunc = [](SOCKET& slisten, std::vector<HttpSocketRequest>& RequestList, std::mutex& ThreadAccessLocker)
	{
		using BufferArray = std::pair<std::array<byte, 1024>, int>;
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

				unsigned long ul = 1;
				ioctlsocket(sClient, FIONBIO, (unsigned long *)&ul);

				if (sClient == INVALID_SOCKET)
				{
					continue;
				}
				//屎，要重写
				//接收数据 
				//auto RecCount = -1;
				//auto TotalLenth = 0;
				//auto sleepCount = 0;
				//while (RecCount != 0)
				//{
				//	RecCount = recv(sClient, (char*)Tempbuffer.first._Elems, 1024, 0);
				//	if (RecCount == -1) 
				//	{
				//		if (sleepCount > 10)break;
				//		Sleep(5);
				//		continue;
				//	}
				//	TotalLenth += RecCount;
				//	RecBuffer.push_back(Tempbuffer);
				//	if (RecBuffer.size() > 1024)
				//	{
				//		closesocket(sClient);
				//		std::vector<BufferArray>().swap(RecBuffer);
				//		throw std::exception("Bad Request");
				//	}
				//	if (RecCount != 1024)
				//		break;
				//}
				//auto buffer = new byte[TotalLenth];
				//for (auto i = 0; i < TotalLenth; i++)
				//{
				//	if (i % 1024 > RecBuffer[i / 1024].second);
				//	buffer[i] = RecBuffer[i / 1024].first[i % 1024];
				//}
				ThreadAccessLocker.lock();
				auto req = HttpSocketRequest({ sClient,buffer,TotalLenth });
				RequestList.push_back(req);
				ThreadAccessLocker.unlock();
				//std::thread(SocketConnectionHandler,sClient, buffer, TotalLenth); 这个会因为析构崩溃
			}
			catch (std::exception e) {}
		}
	};
	auto ServerThreadPoolFunc = [](HttpServer& host,std::vector<HttpSocketRequest>& RequestList, std::mutex& ThreadAccessLocker)
	{
		while (true)
		{
			if (RequestList.size() < 8)Sleep(1);
			if (RequestList.size() == 0)continue;
			ThreadAccessLocker.lock();
			if (RequestList.size() == 0) { ThreadAccessLocker.unlock();continue; }
			auto req = RequestList[0];
			RequestList.erase(RequestList.begin());
			ThreadAccessLocker.unlock();
			host.HttpSocketRequestRecevedEvent.Active(req);
			delete[] req.buffer;
			closesocket(req.sClient);
		}
	};

	HttpServer::HttpServer(int ListenPort)
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
			threadPool[i] = std::thread(ServerThreadPoolFunc, std::ref(*this), std::ref(RequestList), std::ref(ThreadAccessLocker));
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
	HttpServer::~HttpServer()
	{
		closesocket(slisten);
		WSACleanup();
	}
}