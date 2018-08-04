#pragma once
#include <thread>
#include <array>
#include <mutex>
#include <vector>
#include <string>
#include "Event.h"
namespace Utils::HttpServer
{
	constexpr int ThreadPoolSize = 4;
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
		std::vector<SOCKET> RequestList;
		std::mutex ThreadAccessLocker;
	public:
		using EventHttpSocketRequestReceved = Event::Event<HttpSocketRequest>;
		EventHttpSocketRequestReceved HttpSocketRequestRecevedEvent;
	public:
		class HttpRequest
		{
		public:
#pragma region 构造,析构函数
			HttpRequest() = delete;
			HttpRequest(const char* HttpRequestBuffer,int64_t bufferLength);
			HttpRequest(const HttpRequest& req);
			HttpRequest(HttpRequest&& rreq);
			~HttpRequest();
#pragma endregion
#undef DELETE
		public:
#pragma region 请求上报的字段数据定义
			//请求类型
			enum class RequsetMethod
			{
				//请求指定的页面信息，并返回实体主体。
				GET,
				//类似于GET请求，只不过返回的响应中没有具体的内容，用于获取报头
				HEAD,
				//向指定资源提交数据进行处理请求（例如提交表单或者上传文件）。数据被包含在请求体中。POST请求可能会导致新的资源的建立和/或已有资源的修改。
				POST,
				//	从客户端向服务器传送的数据取代指定的文档的内容。
				PUT,
				//请求服务器删除指定的页面。
				DELETE,
				//HTTP/1.1协议中预留给能够将连接改为管道方式的代理服务器。
				CONNECT,
				//允许客户端查看服务器的性能。
				OPTIONS,
				//回显服务器收到的请求，主要用于测试或诊断。
				TRACE
			};
			//请求类型
			using string = std::string; //Magic ?
			RequsetMethod Method;
			string RequsetUrl = "";
			string Accept = "";
			string Accept_Charset = "";
			string Accept_Encoding = "";
			string Accept_Language = "";
			string Accept_Datetime = "";
			string Authorization = "";
			enum class CacheControl
			{
				//所有内容都将被缓存(客户端和代理服务器都可缓存)
				Public,
				//内容只缓存到私有缓存中(仅客户端可以缓存，代理服务器不可缓存)
				Private,
				//必须先与服务器确认返回的响应是否被更改，然后才能使用该响应来满足后续对同一个网址的请求。因此，如果存在合适的验证令牌 (ETag)，no-cache 会发起往返通信来验证缓存的响应，如果资源未被更改，可以避免下载。
				NoCache,
				//所有内容都不会被缓存到缓存或 Internet 临时文件中
				NoStore,
				//如果缓存的内容失效，请求必须发送到服务器以进行重新验证
				MustRevalidation,
				//如果缓存的内容失效，请求必须发送到代理以进行重新验证
				ProxyRevalidation,
				//缓存的内容将在 Cache_Control_Max_Age 秒后失效, 这个选项只在HTTP 1.1可用, 并如果和Last-Modified一起使用时, 优先级较高
				MaxAge,
				//请求的客户端没有上报此字段
				None,
				//请求的客户端上报的字段此服务器不支持
				NotSupport
			};
			CacheControl Cache_Control = CacheControl::None;
			int64_t Cache_Control_Max_Age = -1;
			enum class ConnectionType 
			{
				//长连接
				KeepAlive,
				//短链接
				Close,
				//请求的客户端没有上报此字段
				None
			};
			ConnectionType Connection_Type = ConnectionType::None;
			string Cookie = "";
			int64_t Content_Length = -1;
			string Content_MD5 = "";
			string Content_Type = "";
			//shit，don't want to touch.
			/*
			struct HttpDateTime 
			{
				int Year;
				int Month;
				int Day;
				int Hour;
				int Minute;
				int Second;
				enum class week
				{
					Mon = 1,
					Tues = 2,
					Wed = 3,
					Thur = 4,
					Fri = 5,
					Sat = 6,
					Sun = 7
				};
				week WeekDat;
			};
			HttpDateTime Date = { 0 };
			*/
			string Expect = "";
			string From = "";
			string Host = "";
			string If_Match = "";
			string If_Modified_Since = "";
			string If_None_Match = "";
			string If_Range = "";
			string If_Unmodified_Since = "";
			int64_t Max_Forwards = -1;
			string Origin = "";
			string Pragma = "";
			string Proxy_Authorization = "";
			string Range = "";
			string Referer = "";
			string TE = "";
			string User_Agent = "";
			string Upgrade = "";
			string Via = "";
			string Warning = "";
#pragma endregion
		};
	};
	using HttpSocketRequest = HttpServer::HttpSocketRequest;

}

