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

				//�ȴ�TCP����
				sClient = accept(slisten, (SOCKADDR *)&remoteAddr, &nAddrlen);

				//unsigned long ul = 1;
				//ioctlsocket(sClient, FIONBIO, (unsigned long *)&ul);

				if (sClient == INVALID_SOCKET)
				{
					continue;
				}
				setsockopt(sClient, SOL_SOCKET, SO_RCVTIMEO, (char *)&RecvTimeOut, sizeof(int));
				//�ύ���̳߳ش���
				ThreadAccessLocker.lock();
				RequestList.push_back(sClient);
				ThreadAccessLocker.unlock();
				//std::thread(SocketConnectionHandler,sClient, buffer, TotalLenth); �������Ϊ��������
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

			//�������� 
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
		//��ʼ��
		hr = WSAStartup(sockVersion, &wsaData);
		if (hr != 0)throw SocketException;
		slisten = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (slisten == INVALID_SOCKET)throw SocketException;

		this->ListenPort = ListenPort;

		for (auto i = 0; i < threadPool.size(); i++)
		{
			threadPool[i] = std::thread(ServerThreadPoolFunc, std::ref(*this), std::ref(RequestList), std::ref(ThreadAccessLocker));
		}

		//�󶨶˿�
		sockaddr_in sin;
		sin.sin_family = AF_INET;
		sin.sin_port = htons(ListenPort);
		sin.sin_addr.S_un.S_addr = INADDR_ANY;
		if (::bind(slisten, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR)
			throw SocketException;

		//��ʼ����  
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
		std::string RequestHeader(HttpRequestBuffer, HeaderLength);

#pragma region �ж���������
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
#pragma region �ж���������
		auto urlStart = RequestHeader.find(" ") + 1;
		auto urlLength = RequestHeader.find(" ", urlStart) - urlStart;
		this->RequsetUrl = std::string(RequestHeader.data() + urlStart, urlLength);
#pragma endregion

#pragma region Cache-Control����
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
#pragma region Connection-Type����
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

#pragma region ��ֵ���ֶδ���
#pragma region Content-Length����
		if (RequestHeader.find("Content-Length") != -1)
		{
			this->Content_Length = atoi(RequestHeader.data() + RequestHeader.find("Content-Length") + strlen("Content-Length"));
		}
#pragma endregion
#pragma region Max-Forwards����
		if (RequestHeader.find("Max-Forwards") != -1)
		{
			this->Max_Forwards = atoi(RequestHeader.data() + RequestHeader.find("Max-Forwards") + strlen("Max-Forwards"));
		}
#pragma endregion
#pragma endregion	
#pragma region �ַ����ֶδ���

#pragma region Accept�ֶδ���
		if (RequestHeader.find("Accept:") != -1)
			this->Accept = HttpRequestHandler(RequestHeader, "Accept:");
#pragma endregion
#pragma region Accept-Encoding�ֶδ���
		if (RequestHeader.find("Accept-Encoding:") != -1)
			this->Accept_Encoding = HttpRequestHandler(RequestHeader, "Accept-Encoding:");
#pragma endregion
#pragma region Accept-Language�ֶδ���
		if (RequestHeader.find("Accept-Language:") != -1)
			this->Accept_Language = HttpRequestHandler(RequestHeader, "Accept-Language:");
#pragma endregion
#pragma region Accept-Datetime�ֶδ���
		if (RequestHeader.find("Accept-Datetime") != -1)
			this->Accept_Datetime = HttpRequestHandler(RequestHeader, "Accept-Datetime:");
#pragma endregion
#pragma region Authorization�ֶδ���
		if (RequestHeader.find("Authorization") != -1)
			this->Authorization = HttpRequestHandler(RequestHeader, "Authorization:");
#pragma endregion
#pragma region Cookie�ֶδ���
		if (RequestHeader.find("Cookie") != -1)
			this->Cookie = HttpRequestHandler(RequestHeader, "Cookie:");
#pragma endregion
#pragma region Content_MD5�ֶδ���
		if (RequestHeader.find("Content-MD5") != -1)
			this->Content_MD5 = HttpRequestHandler(RequestHeader, "Content-MD5:");
#pragma endregion
#pragma region Content_Type�ֶδ���
		if (RequestHeader.find("Content-Type") != -1)
			this->Content_Type = HttpRequestHandler(RequestHeader, "Content-Type:");
#pragma endregion
#pragma region Expect�ֶδ���
		if (RequestHeader.find("Expect") != -1)
			this->Expect = HttpRequestHandler(RequestHeader, "Expect:");
#pragma endregion
#pragma region From�ֶδ���
		if (RequestHeader.find("From") != -1)
			this->From = HttpRequestHandler(RequestHeader, "From:");
#pragma endregion
#pragma region Host�ֶδ���
		if (RequestHeader.find("Host") != -1)
			this->Host = HttpRequestHandler(RequestHeader, "Host:");
#pragma endregion
#pragma region If_Match�ֶδ���
		if (RequestHeader.find("If-Match") != -1)
			this->If_Match = HttpRequestHandler(RequestHeader, "If-Match:");
#pragma endregion
#pragma region If_Modified_Since�ֶδ���
		if (RequestHeader.find("If-Modified-Since") != -1)
			this->If_Modified_Since = HttpRequestHandler(RequestHeader, "If-Modified-Since:");
#pragma endregion
#pragma region If_None_Match�ֶδ���
		if (RequestHeader.find("If-None-Match") != -1)
			this->If_None_Match = HttpRequestHandler(RequestHeader, "If-None-Match:");
#pragma endregion
#pragma region If_Range�ֶδ���
		if (RequestHeader.find("If-Range") != -1)
			this->If_Range = HttpRequestHandler(RequestHeader, "If-Range:");
#pragma endregion
#pragma region If_Unmodified_Since�ֶδ���
		if (RequestHeader.find("If-Unmodified-Since") != -1)
			this->If_Unmodified_Since = HttpRequestHandler(RequestHeader, "If-Unmodified-Since:");
#pragma endregion
#pragma region Origin�ֶδ���
		if (RequestHeader.find("Origin") != -1)
			this->Origin = HttpRequestHandler(RequestHeader, "Origin:");
#pragma endregion
#pragma region Pragma�ֶδ���
		if (RequestHeader.find("Pragma") != -1)
			this->Pragma = HttpRequestHandler(RequestHeader, "Pragma:");
#pragma endregion
#pragma region Proxy_Authorization�ֶδ���
		if (RequestHeader.find("Proxy-Authorization") != -1)
			this->Proxy_Authorization = HttpRequestHandler(RequestHeader, "Proxy-Authorization:");
#pragma endregion
#pragma region Range�ֶδ���
		if (RequestHeader.find("Range") != -1)
			this->Range = HttpRequestHandler(RequestHeader, "Range:");
#pragma endregion
#pragma region Referer�ֶδ���
		if (RequestHeader.find("Referer") != -1)
			this->Referer = HttpRequestHandler(RequestHeader, "Referer:");
#pragma endregion
#pragma region TE�ֶδ���
		if (RequestHeader.find("TE") != -1)
			this->TE = HttpRequestHandler(RequestHeader, "TE:");
#pragma endregion
#pragma region User_Agent�ֶδ���
		if (RequestHeader.find("User-Agent") != -1)
			this->User_Agent = HttpRequestHandler(RequestHeader, "User-Agent:");
#pragma endregion
#pragma region Upgrade�ֶδ���
		if (RequestHeader.find("Upgrade") != -1)
			this->Upgrade = HttpRequestHandler(RequestHeader, "Upgrade:");
#pragma endregion
#pragma region Via�ֶδ���
		if (RequestHeader.find("Via") != -1)
			this->Via = HttpRequestHandler(RequestHeader, "Via:");
#pragma endregion
#pragma region Warning�ֶδ���
		if (RequestHeader.find("Warning") != -1)
			this->Warning = HttpRequestHandler(RequestHeader, "Warning:");
#pragma endregion
#pragma endregion

	}
	HttpServer::HttpRequest::~HttpRequest()
	{
	}
}