#include "stdafx.h"
#include "HttpServer.h"
namespace Utils::HttpServer
{
	const std::exception SocketException("socket error");

	auto ServerListenThreadFunc = [](SOCKET& slisten, std::vector<SOCKET>& RequestList, std::mutex& ThreadAccessLocker)
	{
		constexpr int RecvTimeOut = 3000;
		while (true)
		{
			try
			{
				SOCKET sClient;
				sockaddr_in remoteAddr;
				int nAddrlen = sizeof(remoteAddr);

				//等待TCP连接
				sClient = accept(slisten, (SOCKADDR *)&remoteAddr, &nAddrlen);

				//unsigned long ul = 1;
				//ioctlsocket(sClient, FIONBIO, (unsigned long *)&ul);

				if (sClient == INVALID_SOCKET)
				{
					continue;
				}
				setsockopt(sClient, SOL_SOCKET, SO_RCVTIMEO, (char *)&RecvTimeOut, sizeof(int));
				//提交给线程池处理
				ThreadAccessLocker.lock();
				RequestList.push_back(sClient);
				ThreadAccessLocker.unlock();
				//std::thread(SocketConnectionHandler,sClient, buffer, TotalLenth); 这个会因为析构崩溃
			}
			catch (std::exception e) {}
		}
	};
	auto ServerThreadPoolFunc = [](HttpServer& host,std::vector<SOCKET>& RequestList, std::mutex& ThreadAccessLocker)
	{
		constexpr int BufferLength = 256;
		constexpr int RecvTimeOut = 3000;
		using BufferArray = std::array<byte, BufferLength>;
		while (true)
		{
			if (RequestList.size() < 8)Sleep(1);
			if (RequestList.size() == 0)continue;
			ThreadAccessLocker.lock();
			if (RequestList.size() == 0) { ThreadAccessLocker.unlock();continue; }
			auto sClient = RequestList[0];
			RequestList.erase(RequestList.begin());
			ThreadAccessLocker.unlock();

			//接收数据 
			std::vector<BufferArray> HttpRecBuffer;
			BufferArray Tempbuffer = { 0 };
			auto RecCount = 0;
			auto TotalLenth = 0;
			auto headLength = 0;
			auto RetryCount = 0;
			while (true)
			{
				auto SingleRecCount = recv(sClient, (char*)Tempbuffer._Elems + RecCount, BufferLength - RecCount, 0);
				if (SingleRecCount == -1)
				{
					if (RetryCount == 10)
					{
						break;
					}
					RetryCount++;
					Sleep(1);
					continue;

				};
				RecCount += SingleRecCount;
				if (strstr((char*)Tempbuffer._Elems, "\r\n\r\n"))
				{
					headLength = strstr((char*)Tempbuffer._Elems, "\r\n\r\n") - (char*)Tempbuffer._Elems + TotalLenth + 4;
					HttpRecBuffer.push_back(Tempbuffer);
					TotalLenth += RecCount;
					break;
				}
				TotalLenth += RecCount;
				if (TotalLenth%BufferLength == 0)
				{
					RecCount = 0;
					HttpRecBuffer.push_back(Tempbuffer);
					Tempbuffer = { 0 };
				};
			}
			if (HttpRecBuffer.empty()) { closesocket(sClient); continue; };
			byte* buffer = nullptr;
			if (strstr((char*)HttpRecBuffer[0]._Elems, "POST") == (char*)HttpRecBuffer[0]._Elems)
			{
				//HTTP POST
				char* HttpHeader = new char[headLength];
				for (auto i = 0; i < headLength; i++)
				{
					HttpHeader[i] = HttpRecBuffer[i / BufferLength][i % BufferLength];
				}
				auto ContentLengthPos = strstr(HttpHeader, "Content-Length: ") + 16;
				auto ContentLength = atoi(ContentLengthPos);
				buffer = new byte[ContentLength];
				for (auto i = 0; i < TotalLenth; i++)
				{
					buffer[i] = HttpRecBuffer[i / BufferLength][i % BufferLength];
				}
				while (TotalLenth != ContentLength)
				{
					RecCount = recv(sClient, (char*)buffer + TotalLenth, ContentLength - TotalLenth, 0);
					TotalLenth += RecCount;
				}
			}
			else
			{
				buffer = new byte[TotalLenth];
				for (auto i = 0; i < TotalLenth; i++)
				{
					buffer[i] = HttpRecBuffer[i / BufferLength][i % BufferLength];
				}
			}
			HttpSocketRequest req = { sClient,buffer,TotalLenth };

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