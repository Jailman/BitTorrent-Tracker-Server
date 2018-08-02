#include "stdafx.h"
#include "HttpServer.h"
namespace Utils::HttpServer
{

	auto HttpRequestHandler = [](const std::string& HttpRequestBuffer,const char* const Header)
	{
		auto indexStart = HttpRequestBuffer.find(Header) + strlen(Header);
		auto indexEnd = HttpRequestBuffer.find("\r\n", indexStart);
		auto indexLength = indexEnd - indexStart;
		std::string value(HttpRequestBuffer.data() + indexStart, indexLength);
		return value;
	};
	const std::exception SocketException("socket error");
	const std::exception BadRequestException("bad request");
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
	HttpServer::HttpRequest::HttpRequest(const char *HttpRequestBuffer, int64_t bufferLength)
	{
		const auto HttpHeadEndLength = strlen("\r\n\r\n");
		auto reqHeaderEnd = strstr(HttpRequestBuffer, "\r\n\r\n") + HttpHeadEndLength;
		if (reqHeaderEnd == 0)throw BadRequestException;
		auto HeaderLength = reqHeaderEnd - HttpRequestBuffer;
		std::string RequestHeade(HttpRequestBuffer, HeaderLength);
#pragma region 判断请求类型
		switch (HttpRequestBuffer[0])
		{
		case 'C':
			this->Method = RequsetMethod::CONNECT;
			break;
		case 'D':
			this->Method = RequsetMethod::DELETE;
			break;
		case 'G':
			this->Method = RequsetMethod::GET;
			break;
		case 'H':
			this->Method = RequsetMethod::HEAD;
			break;
		case 'O':
			this->Method = RequsetMethod::OPTIONS;
			break;
		case 'P':
			if (HttpRequestBuffer[1] == 'O')this->Method = RequsetMethod::POST;
			else this->Method = RequsetMethod::PUT;
			break;
		case 'T':
			this->Method = RequsetMethod::TRACE;
		default:
			throw BadRequestException;
			break;
		}
#pragma endregion
#pragma region 判断请求链接
		auto urlStart = RequestHeade.find(" ") + 1;
		auto urlLength = RequestHeade.find(" ", urlStart) - urlStart;
		this->RequsetUrl = std::string(RequestHeade.data() + urlStart, urlLength);
#pragma endregion
#pragma region Cache_Control处理
		if (RequestHeade.find("Cache-Control") != -1)
		{
		auto Cache_Control_Method = HttpRequestHandler(RequestHeade, "Cache-Control:");
		this->Cache_Control = CacheControl::NotSupport;
		if (Cache_Control_Method.find("max-age=") != -1) 
		{
			this->Cache_Control = CacheControl::MaxAge;
			const auto MaxAgeLength = strlen("max-age=");
			this->Cache_Control_Max_Age = atoi(Cache_Control_Method.data() + MaxAgeLength);
		}
		else if (Cache_Control_Method == "public")
			this->Cache_Control = CacheControl::Public;
		else if (Cache_Control_Method == "no-cache")
			this->Cache_Control = CacheControl::NoCache;
		else if (Cache_Control_Method == "no-store")
			this->Cache_Control = CacheControl::NoStore;
		else if (Cache_Control_Method == "must-revalidation")
			this->Cache_Control = CacheControl::MustRevalidation;
		else if (Cache_Control_Method == "private")
			this->Cache_Control = CacheControl::Private;
		else if (Cache_Control_Method == "proxy-revalidation")
			this->Cache_Control = CacheControl::ProxyRevalidation;
		
		}
#pragma endregion


#pragma region 字符类字段处理

#pragma region Accept字段处理
		if (RequestHeade.find("Accept:") != -1)
			this->Accept = HttpRequestHandler(RequestHeade, "Accept:");
#pragma endregion
#pragma region Accept-Encoding字段处理
		if (RequestHeade.find("Accept-Encoding:") != -1)
			this->Accept_Encoding = HttpRequestHandler(RequestHeade, "Accept-Encoding:");
#pragma endregion
#pragma region Accept-Language字段处理
		if (RequestHeade.find("Accept-Language:") != -1)
			this->Accept_Language = HttpRequestHandler(RequestHeade, "Accept-Language:");
#pragma endregion
#pragma region Accept-Datetime字段处理
		if (RequestHeade.find("Accept-Datetime") != -1)
			this->Accept_Datetime = HttpRequestHandler(RequestHeade, "Accept-Datetime:");
#pragma endregion
#pragma region Authorization字段处理
		if (RequestHeade.find("Authorization") != -1)
			this->Authorization = HttpRequestHandler(RequestHeade, "Authorization:");
#pragma endregion
#pragma region Cookie字段处理
		if (RequestHeade.find("Cookie") != -1)
			this->Cookie = HttpRequestHandler(RequestHeade, "Cookie:");
#pragma endregion
#pragma region Content_MD5字段处理
		if (RequestHeade.find("Content-MD5") != -1)
			this->Content_MD5 = HttpRequestHandler(RequestHeade, "Content-MD5:");
#pragma endregion
#pragma region Content_Type字段处理
		if (RequestHeade.find("Content-Type") != -1)
			this->Content_Type = HttpRequestHandler(RequestHeade, "Content-Type:");
#pragma endregion
#pragma region Expect字段处理
		if (RequestHeade.find("Expect") != -1)
			this->Expect = HttpRequestHandler(RequestHeade, "Expect:");
#pragma endregion
#pragma region From字段处理
		if (RequestHeade.find("From") != -1)
			this->From = HttpRequestHandler(RequestHeade, "From:");
#pragma endregion
#pragma region Host字段处理
		if (RequestHeade.find("Host") != -1)
			this->Host = HttpRequestHandler(RequestHeade, "Host:");
#pragma endregion
#pragma region If_Match字段处理
		if (RequestHeade.find("If-Match") != -1)
			this->If_Match = HttpRequestHandler(RequestHeade, "If-Match:");
#pragma endregion
#pragma region If_Modified_Since字段处理
		if (RequestHeade.find("If-Modified-Since") != -1)
			this->If_Modified_Since = HttpRequestHandler(RequestHeade, "If-Modified-Since:");
#pragma endregion
#pragma region If_None_Match字段处理
		if (RequestHeade.find("If-None-Match") != -1)
			this->If_None_Match = HttpRequestHandler(RequestHeade, "If-None-Match:");
#pragma endregion
#pragma region If_Range字段处理
		if (RequestHeade.find("If-Range") != -1)
			this->If_Range = HttpRequestHandler(RequestHeade, "If-Range:");
#pragma endregion
#pragma region If_Unmodified_Since字段处理
		if (RequestHeade.find("If-Unmodified-Since") != -1)
			this->If_Unmodified_Since = HttpRequestHandler(RequestHeade, "If-Unmodified-Since:");
#pragma endregion
#pragma region Origin字段处理
		if (RequestHeade.find("Origin") != -1)
			this->Origin = HttpRequestHandler(RequestHeade, "Origin:");
#pragma endregion
#pragma region Pragma字段处理
		if (RequestHeade.find("Pragma") != -1)
			this->Pragma = HttpRequestHandler(RequestHeade, "Pragma:");
#pragma endregion
#pragma region Proxy_Authorization字段处理
		if (RequestHeade.find("Proxy-Authorization") != -1)
			this->Proxy_Authorization = HttpRequestHandler(RequestHeade, "Proxy-Authorization:");
#pragma endregion
#pragma region Range字段处理
		if (RequestHeade.find("Range") != -1)
			this->Range = HttpRequestHandler(RequestHeade, "Range:");
#pragma endregion
#pragma region Referer字段处理
		if (RequestHeade.find("Referer") != -1)
			this->Referer = HttpRequestHandler(RequestHeade, "Referer:");
#pragma endregion
#pragma region TE字段处理
		if (RequestHeade.find("TE") != -1)
			this->TE = HttpRequestHandler(RequestHeade, "TE:");
#pragma endregion
#pragma region User_Agent字段处理
		if (RequestHeade.find("User-Agent") != -1)
			this->User_Agent = HttpRequestHandler(RequestHeade, "User-Agent:");
#pragma endregion
#pragma region Upgrade字段处理
		if (RequestHeade.find("Upgrade") != -1)
			this->Upgrade = HttpRequestHandler(RequestHeade, "Upgrade:");
#pragma endregion
#pragma region Via字段处理
		if (RequestHeade.find("Via") != -1)
			this->Via = HttpRequestHandler(RequestHeade, "Via:");
#pragma endregion
#pragma region Warning字段处理
		if (RequestHeade.find("Warning") != -1)
			this->Warning = HttpRequestHandler(RequestHeade, "Warning:");
#pragma endregion
#pragma endregion

	}
	HttpServer::HttpRequest::~HttpRequest()
	{
	}
}