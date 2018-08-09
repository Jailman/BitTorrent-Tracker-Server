#pragma once
#include "HttpServer.h"
#include "stdafx.h"
#include <vector>
namespace BTTServer 
{
	class BTTServer final : protected Utils::HttpServer::HttpServer
	{
	public:
		BTTServer(int ListenPort);
		~BTTServer();
		class Hash 
		{
		private:
			byte hash[20] = { 0 };
		public:
			Hash() {};
			Hash(std::string hashString);
			Hash(const char* hashString);
			Hash(const void* hashptr);
			~Hash();
			void CopyTo(void* target);
			byte* data();
		public:
			bool operator==(const Hash& h);
			bool operator!=(const Hash& h);
			Hash operator= (const Hash& h);
			Hash operator= (const Hash&&) = delete;
			Hash operator= (std::string hashString);
			Hash operator= (const char* hashString);
			Hash operator= (const void* hashptr);
		public:
			Hash(const Hash& h);
			Hash(const Hash&&) = delete;
			
		};
		class HttpBTRequest final : protected Utils::HttpServer::HttpServer::HttpRequest
		{
		public:
			HttpBTRequest(SOCKET ClientSocket);
		public:
			Hash peer_info_hash;
			char peer_peer_id[21] = { 0 };
			uint16_t peer_port;
			int64_t peer_uploaded;
			int64_t peer_downloaded;
			int64_t peer_left;
			int64_t peer_corrupt;
			std::string peer_key;
			enum class _Event
			{
				empty,
				started,
				stopped,
				completed
			};
			_Event peer_Event = _Event::empty;
			int64_t peer_numwant = 50;
		};
	public:
		using EventHttpBTRequestReceved = Event::Event<HttpBTRequest&>;
		EventHttpBTRequestReceved HttpBTRequestRecevedEvent;
	};
}

/*
info_hash=SO%e6%5c%ab%12%98%d6%3e%89%e8%02%1b%c3%be%be%cc%dcA6&
peer_id=-qB4110-wqjw60mapBg*&
port=3988&
uploaded=0&
downloaded=0&
left=55921780076&
corrupt=0&
key=EBCCD8C2&
event=started&
numwant=200&
compact=1&
no_peer_id=1&
supportcrypto=1&
redundant=0
*/
