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

	const std::exception SocketException("Socket Error");
	const std::exception BadRequestException("Bad Request");
	const std::exception BadConnectionException("Bad Connection");

	auto ServerListenThreadFunc = [](SOCKET& HostSocket, std::vector<SOCKET>& RequestList, std::mutex& ThreadAccessLocker,bool& ExitFlag)
	{
		constexpr int RecvTimeOut = 3000;
		while (ExitFlag)
		{
			try
			{
				SOCKET sClient;
				sockaddr_in remoteAddr;
				int nAddrlen = sizeof(remoteAddr);

				//等待TCP连接
				sClient = accept(HostSocket, (SOCKADDR *)&remoteAddr, &nAddrlen);

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
				//std::thread(SocketConnectionHandler,sClient, RequestBuffer, TotalLenth); 这个会因为析构崩溃
			}
			catch (std::exception e) {}
		}
	};
	auto ServerThreadPoolFunc = [](HttpServer& Host,std::vector<SOCKET>& RequestList, std::mutex& ThreadAccessLocker, bool& ExitFlag)
	{
		while (ExitFlag)
		{
			if (RequestList.size() < 8)Sleep(1);
			if (RequestList.size() == 0)continue;
			ThreadAccessLocker.lock();
			if (RequestList.size() == 0) { ThreadAccessLocker.unlock();continue; }
			auto sClient = RequestList[0];
			RequestList.erase(RequestList.begin());
			ThreadAccessLocker.unlock();
			HttpServer::HttpRequest req(sClient);
			Host.HttpSocketRequestRecevedEvent.Active(req);
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
			threadPool[i] = std::thread(ServerThreadPoolFunc, std::ref(*this), std::ref(RequestList), std::ref(ThreadAccessLocker), std::ref(this->ExitFlag));
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

		loopTh = std::thread(ServerListenThreadFunc, std::ref(slisten), std::ref(RequestList), std::ref(ThreadAccessLocker), std::ref(this->ExitFlag));
	}
	HttpServer::~HttpServer()
	{
		this->ExitFlag = false;
		for (auto& thread : threadPool) 
			if (thread.joinable())thread.join();
		if (loopTh.joinable())loopTh.join();
		std::vector<SOCKET>().swap(this->RequestList);
		closesocket(slisten);
		WSACleanup();
	}
	HttpServer::HttpRequest::HttpRequest(SOCKET sClient)
	{
#pragma region Socket参数获取
		this->ClientSocket = sClient;
		int SockAddrSize = sizeof(sockaddr_in);
		getpeername(sClient, (sockaddr*)&(this->ClientAddr), &SockAddrSize);
#pragma endregion

#pragma region 接收数据
		constexpr int BufferLength = 256;
		constexpr int RecvTimeOut = 3000;
		using BufferArray = std::array<byte, BufferLength>;
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
		if (HttpRecBuffer.empty()) { closesocket(sClient); throw BadRequestException; };
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
			RequestBuffer = new byte[ContentLength];
			for (auto i = 0; i < TotalLenth; i++)
			{
				RequestBuffer[i] = HttpRecBuffer[i / BufferLength][i % BufferLength];
			}
			while (TotalLenth != ContentLength)
			{
				RecCount = recv(sClient, (char*)RequestBuffer + TotalLenth, ContentLength - TotalLenth, 0);
				TotalLenth += RecCount;
			}
		}
		else
		{
			RequestBuffer = new byte[TotalLenth];
			for (auto i = 0; i < TotalLenth; i++)
			{
				RequestBuffer[i] = HttpRecBuffer[i / BufferLength][i % BufferLength];
			}
		}
#pragma endregion
#pragma region 处理请求
		const auto HttpHeadEndLength = strlen("\r\n\r\n");
		auto reqHeaderEnd = strstr((char*)(this->RequestBuffer), "\r\n\r\n") + HttpHeadEndLength;
		if (reqHeaderEnd == 0)throw BadRequestException;
		auto HeaderLength = reqHeaderEnd - (char*)RequestBuffer;
		this->RequestHeader = std::string((char*)RequestBuffer, HeaderLength);
#pragma region 判断请求类型
		switch (RequestBuffer[0])
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
			if (RequestBuffer[1] == 'O')this->Method = RequsetMethod::POST;
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
		auto urlStart = RequestHeader.find(" ") + 1;
		auto urlLength = RequestHeader.find(" ", urlStart) - urlStart;
		this->RequsetUrl = std::string(RequestHeader.data() + urlStart, urlLength);
#pragma endregion
#pragma region Cache-Control处理
		if (RequestHeader.find("Cache-Control") != -1)
		{
			auto Cache_Control_Method = HttpRequestHandler(RequestHeader, "Cache-Control:");
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
#pragma region Connection-Type处理
		if (RequestHeader.find("Connection-Type") != -1)
		{
			auto Cache_Control_Method = HttpRequestHandler(RequestHeader, "Connection-Type");
			this->Connection_Type = ConnectionType::None;
			if (Cache_Control_Method == "keep-alive")
				this->Connection_Type = ConnectionType::KeepAlive;
			else if (Cache_Control_Method == "close")
				this->Connection_Type = ConnectionType::Close;
		}
#pragma endregion
#pragma region 数值类字段处理
#pragma region Content-Length处理
		if (RequestHeader.find("Content-Length") != -1)
		{
			this->Content_Length = atoi(RequestHeader.data() + RequestHeader.find("Content-Length") + strlen("Content-Length"));
		}
#pragma endregion
#pragma region Max-Forwards处理
		if (RequestHeader.find("Max-Forwards") != -1)
		{
			this->Max_Forwards = atoi(RequestHeader.data() + RequestHeader.find("Max-Forwards") + strlen("Max-Forwards"));
		}
#pragma endregion
#pragma endregion	
#pragma region 字符类字段处理

#pragma region Accept字段处理
		if (RequestHeader.find("Accept:") != -1)
			this->Accept = HttpRequestHandler(RequestHeader, "Accept:");
#pragma endregion
#pragma region Accept-Encoding字段处理
		if (RequestHeader.find("Accept-Encoding:") != -1)
			this->Accept_Encoding = HttpRequestHandler(RequestHeader, "Accept-Encoding:");
#pragma endregion
#pragma region Accept-Language字段处理
		if (RequestHeader.find("Accept-Language:") != -1)
			this->Accept_Language = HttpRequestHandler(RequestHeader, "Accept-Language:");
#pragma endregion
#pragma region Accept-Datetime字段处理
		if (RequestHeader.find("Accept-Datetime") != -1)
			this->Accept_Datetime = HttpRequestHandler(RequestHeader, "Accept-Datetime:");
#pragma endregion
#pragma region Authorization字段处理
		if (RequestHeader.find("Authorization") != -1)
			this->Authorization = HttpRequestHandler(RequestHeader, "Authorization:");
#pragma endregion
#pragma region Cookie字段处理
		if (RequestHeader.find("Cookie") != -1)
			this->Cookie = HttpRequestHandler(RequestHeader, "Cookie:");
#pragma endregion
#pragma region Content_MD5字段处理
		if (RequestHeader.find("Content-MD5") != -1)
			this->Content_MD5 = HttpRequestHandler(RequestHeader, "Content-MD5:");
#pragma endregion
#pragma region Content_Type字段处理
		if (RequestHeader.find("Content-Type") != -1)
			this->Content_Type = HttpRequestHandler(RequestHeader, "Content-Type:");
#pragma endregion
#pragma region Expect字段处理
		if (RequestHeader.find("Expect") != -1)
			this->Expect = HttpRequestHandler(RequestHeader, "Expect:");
#pragma endregion
#pragma region From字段处理
		if (RequestHeader.find("From") != -1)
			this->From = HttpRequestHandler(RequestHeader, "From:");
#pragma endregion
#pragma region Host字段处理
		if (RequestHeader.find("Host") != -1)
			this->Host = HttpRequestHandler(RequestHeader, "Host:");
#pragma endregion
#pragma region If_Match字段处理
		if (RequestHeader.find("If-Match") != -1)
			this->If_Match = HttpRequestHandler(RequestHeader, "If-Match:");
#pragma endregion
#pragma region If_Modified_Since字段处理
		if (RequestHeader.find("If-Modified-Since") != -1)
			this->If_Modified_Since = HttpRequestHandler(RequestHeader, "If-Modified-Since:");
#pragma endregion
#pragma region If_None_Match字段处理
		if (RequestHeader.find("If-None-Match") != -1)
			this->If_None_Match = HttpRequestHandler(RequestHeader, "If-None-Match:");
#pragma endregion
#pragma region If_Range字段处理
		if (RequestHeader.find("If-Range") != -1)
			this->If_Range = HttpRequestHandler(RequestHeader, "If-Range:");
#pragma endregion
#pragma region If_Unmodified_Since字段处理
		if (RequestHeader.find("If-Unmodified-Since") != -1)
			this->If_Unmodified_Since = HttpRequestHandler(RequestHeader, "If-Unmodified-Since:");
#pragma endregion
#pragma region Origin字段处理
		if (RequestHeader.find("Origin") != -1)
			this->Origin = HttpRequestHandler(RequestHeader, "Origin:");
#pragma endregion
#pragma region Pragma字段处理
		if (RequestHeader.find("Pragma") != -1)
			this->Pragma = HttpRequestHandler(RequestHeader, "Pragma:");
#pragma endregion
#pragma region Proxy_Authorization字段处理
		if (RequestHeader.find("Proxy-Authorization") != -1)
			this->Proxy_Authorization = HttpRequestHandler(RequestHeader, "Proxy-Authorization:");
#pragma endregion
#pragma region Range字段处理
		if (RequestHeader.find("Range") != -1)
			this->Range = HttpRequestHandler(RequestHeader, "Range:");
#pragma endregion
#pragma region Referer字段处理
		if (RequestHeader.find("Referer") != -1)
			this->Referer = HttpRequestHandler(RequestHeader, "Referer:");
#pragma endregion
#pragma region TE字段处理
		if (RequestHeader.find("TE") != -1)
			this->TE = HttpRequestHandler(RequestHeader, "TE:");
#pragma endregion
#pragma region User_Agent字段处理
		if (RequestHeader.find("User-Agent") != -1)
			this->User_Agent = HttpRequestHandler(RequestHeader, "User-Agent:");
#pragma endregion
#pragma region Upgrade字段处理
		if (RequestHeader.find("Upgrade") != -1)
			this->Upgrade = HttpRequestHandler(RequestHeader, "Upgrade:");
#pragma endregion
#pragma region Via字段处理
		if (RequestHeader.find("Via") != -1)
			this->Via = HttpRequestHandler(RequestHeader, "Via:");
#pragma endregion
#pragma region Warning字段处理
		if (RequestHeader.find("Warning") != -1)
			this->Warning = HttpRequestHandler(RequestHeader, "Warning:");
#pragma endregion
#pragma endregion
#pragma endregion
	}
	HttpServer::HttpRequest::HttpRequest(const HttpRequest & req)
	{
		this->Accept = req.Accept;
		this->Accept_Charset = req.Accept_Charset;
		this->Accept_Datetime = req.Accept_Datetime;
		this->Accept_Encoding = req.Accept_Encoding;
		this->Accept_Language = req.Accept_Language;
		this->Authorization = req.Authorization;
		this->Cache_Control = req.Cache_Control;
		this->Cache_Control_Max_Age = req.Cache_Control_Max_Age;
		this->Connection_Type = req.Connection_Type;
		this->Content_Length = req.Content_Length;
		this->Content_MD5 = req.Content_MD5;
		this->Content_Type = req.Content_Type;
		this->Cookie = req.Cookie;
		this->Expect = req.Expect;
		this->From = req.From;
		this->Host = req.Host;
		this->If_Match = req.If_Match;
		this->If_Modified_Since = req.If_Modified_Since;
		this->If_None_Match = req.If_None_Match;
		this->If_Range = req.If_Range;
		this->If_Unmodified_Since = req.If_Unmodified_Since;
		this->Max_Forwards = req.Max_Forwards;
		this->Method = req.Method;
		this->Origin = req.Origin;
		this->Pragma = req.Pragma;
		this->Proxy_Authorization = req.Proxy_Authorization;
		this->Range = req.Range;
		this->Referer = req.Referer;
		this->RequsetUrl = req.RequsetUrl;
		this->TE = req.TE;
		this->Upgrade = req.Upgrade;
		this->User_Agent = req.User_Agent;
		this->Via = req.Via;
		this->Warning = req.Warning;
	}
	HttpServer::HttpRequest::HttpRequest(HttpRequest && rreq)
	
	{
		this->Accept = std::move(rreq.Accept);
		this->Accept_Charset = std::move(rreq.Accept_Charset);
		this->Accept_Datetime = std::move(rreq.Accept_Datetime);
		this->Accept_Encoding = std::move(rreq.Accept_Encoding);
		this->Accept_Language = std::move(rreq.Accept_Language);
		this->Authorization = std::move(rreq.Authorization);
		this->Cache_Control = std::move(rreq.Cache_Control);
		this->Cache_Control_Max_Age = std::move(rreq.Cache_Control_Max_Age);
		this->Connection_Type = std::move(rreq.Connection_Type);
		this->Content_Length = std::move(rreq.Content_Length);
		this->Content_MD5 = std::move(rreq.Content_MD5);
		this->Content_Type = std::move(rreq.Content_Type);
		this->Cookie = std::move(rreq.Cookie);
		this->Expect = std::move(rreq.Expect);
		this->From = std::move(rreq.From);
		this->Host = std::move(rreq.Host);
		this->If_Match = std::move(rreq.If_Match);
		this->If_Modified_Since = std::move(rreq.If_Modified_Since);
		this->If_None_Match = std::move(rreq.If_None_Match);
		this->If_Range = std::move(rreq.If_Range);
		this->If_Unmodified_Since = std::move(rreq.If_Unmodified_Since);
		this->Max_Forwards = std::move(rreq.Max_Forwards);
		this->Method = std::move(rreq.Method);
		this->Origin = std::move(rreq.Origin);
		this->Pragma = std::move(rreq.Pragma);
		this->Proxy_Authorization = std::move(rreq.Proxy_Authorization);
		this->Range = std::move(rreq.Range);
		this->Referer = std::move(rreq.Referer);
		this->RequsetUrl = std::move(rreq.RequsetUrl);
		this->TE = std::move(rreq.TE);
		this->Upgrade = std::move(rreq.Upgrade);
		this->User_Agent = std::move(rreq.User_Agent);
		this->Via = std::move(rreq.Via);
		this->Warning = std::move(rreq.Warning);
	}
	HttpServer::HttpRequest::~HttpRequest()
	{
		if (RequestBuffer != nullptr)delete[] RequestBuffer;
		closesocket(ClientSocket);
	}
}