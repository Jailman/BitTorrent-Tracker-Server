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
#pragma region ����,��������
			HttpRequest() = delete;
			HttpRequest(const char* HttpRequestBuffer,int64_t bufferLength);
			HttpRequest(const HttpRequest& req);
			HttpRequest(HttpRequest&& rreq);
			~HttpRequest();
#pragma endregion
#undef DELETE
		public:
#pragma region �����ϱ����ֶ����ݶ���
			//��������
			enum class RequsetMethod
			{
				//����ָ����ҳ����Ϣ��������ʵ�����塣
				GET,
				//������GET����ֻ�������ص���Ӧ��û�о�������ݣ����ڻ�ȡ��ͷ
				HEAD,
				//��ָ����Դ�ύ���ݽ��д������������ύ�������ϴ��ļ��������ݱ��������������С�POST������ܻᵼ���µ���Դ�Ľ�����/��������Դ���޸ġ�
				POST,
				//	�ӿͻ�������������͵�����ȡ��ָ�����ĵ������ݡ�
				PUT,
				//���������ɾ��ָ����ҳ�档
				DELETE,
				//HTTP/1.1Э����Ԥ�����ܹ������Ӹ�Ϊ�ܵ���ʽ�Ĵ����������
				CONNECT,
				//����ͻ��˲鿴�����������ܡ�
				OPTIONS,
				//���Է������յ���������Ҫ���ڲ��Ի���ϡ�
				TRACE
			};
			//��������
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
				//�������ݶ���������(�ͻ��˺ʹ�����������ɻ���)
				Public,
				//����ֻ���浽˽�л�����(���ͻ��˿��Ի��棬������������ɻ���)
				Private,
				//�������������ȷ�Ϸ��ص���Ӧ�Ƿ񱻸��ģ�Ȼ�����ʹ�ø���Ӧ�����������ͬһ����ַ��������ˣ�������ں��ʵ���֤���� (ETag)��no-cache �ᷢ������ͨ������֤�������Ӧ�������Դδ�����ģ����Ա������ء�
				NoCache,
				//�������ݶ����ᱻ���浽����� Internet ��ʱ�ļ���
				NoStore,
				//������������ʧЧ��������뷢�͵��������Խ���������֤
				MustRevalidation,
				//������������ʧЧ��������뷢�͵������Խ���������֤
				ProxyRevalidation,
				//��������ݽ��� Cache_Control_Max_Age ���ʧЧ, ���ѡ��ֻ��HTTP 1.1����, �������Last-Modifiedһ��ʹ��ʱ, ���ȼ��ϸ�
				MaxAge,
				//����Ŀͻ���û���ϱ����ֶ�
				None,
				//����Ŀͻ����ϱ����ֶδ˷�������֧��
				NotSupport
			};
			CacheControl Cache_Control = CacheControl::None;
			int64_t Cache_Control_Max_Age = -1;
			enum class ConnectionType 
			{
				//������
				KeepAlive,
				//������
				Close,
				//����Ŀͻ���û���ϱ����ֶ�
				None
			};
			ConnectionType Connection_Type = ConnectionType::None;
			string Cookie = "";
			int64_t Content_Length = -1;
			string Content_MD5 = "";
			string Content_Type = "";
			//shit��don't want to touch.
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

