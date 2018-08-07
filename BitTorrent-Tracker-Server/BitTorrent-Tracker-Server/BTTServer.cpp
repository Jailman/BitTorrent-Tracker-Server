#include "stdafx.h"
#include "BTTServer.h"
#include <exception>
#include <vector>
namespace BTTServer
{
#pragma region HashÀàº¯Êý

	const std::exception ParamErrorException("Param Error");
	BTTServer::Hash::Hash(std::string hashString)
	{
		auto FilledBitsCount = 0;
		auto AnalyzedStringCount = 0;
		while (FilledBitsCount < 20)
		{
			if (AnalyzedStringCount == hashString.length())throw ParamErrorException;
			if (hashString.data()[AnalyzedStringCount] == '%')
			{
				auto first = (hashString.data()[++AnalyzedStringCount]);
				auto second = (hashString.data()[++AnalyzedStringCount]);
				const auto GetUint8FronHexChar = [](char ch)
				{
					auto value = static_cast<uint8_t>(ch);
					if ('0' <= value && value <= '9')return value - '0';
					else if ('a' <= value && value <= 'z')return 10 + value - 'a';
					else if ('A' <= value && value <= 'Z')return 10 + value - 'A';
				};
				auto value = GetUint8FronHexChar(first) << 4 + GetUint8FronHexChar(second);
				this->hash[FilledBitsCount++] = value;
			}
			else
			{
				this->hash[FilledBitsCount++] = hashString.data()[AnalyzedStringCount++];
			}
		}
	}
	BTTServer::Hash::Hash(const char * hashString)

	{
		auto FilledBitsCount = 0;
		auto AnalyzedStringCount = 0;
		while (FilledBitsCount < 20)
		{
			if (hashString[AnalyzedStringCount] == 0)throw ParamErrorException;
			if (hashString[AnalyzedStringCount] == '%')
			{
				auto first = (hashString[++AnalyzedStringCount]);
				auto second = (hashString[++AnalyzedStringCount]);
				const auto GetUint8FronHexChar = [](char ch)
				{
					auto value = static_cast<uint8_t>(ch);
					if ('0' <= value && value <= '9')return value - '0';
					else if ('a' <= value && value <= 'z')return 10 + value - 'a';
					else if ('A' <= value && value <= 'Z')return 10 + value - 'A';
				};
				auto value = (GetUint8FronHexChar(first) << 4) + GetUint8FronHexChar(second);
				this->hash[FilledBitsCount++] = value;
			}
			else
			{
				this->hash[FilledBitsCount++] = hashString[AnalyzedStringCount++];
			}
		}
	}
	BTTServer::Hash::Hash(const void * hashptr)
	{
		for (auto i = 0;i < 20;i++)
			this->hash[i] = static_cast<const byte*>(hashptr)[i];
	}
	BTTServer::Hash::~Hash()
	{
	}
	void BTTServer::Hash::CopyTo(void * target)
	{
		for (auto i = 0;i < 20;i++)
			static_cast<byte*>(target)[i] = this->hash[i];
	}
	byte * BTTServer::Hash::data()
	{
		return this->hash;
	}
	bool BTTServer::Hash::operator==(const Hash & h)
	{
		for (auto i = 0;i < 20;i++)
			if (h.hash[i] != this->hash[i])return false;
		return true;
	}
	bool BTTServer::Hash::operator!=(const Hash & h)
	{
		for (auto i = 0;i < 20;i++)
			if (h.hash[i] != this->hash[i])return true;
		return false;
	}
	BTTServer::Hash BTTServer::Hash::operator=(const Hash & h)
	{
		for (auto i = 0;i < 20;i++)
			this->hash[i] = h.hash[i];
		return *this;
	}
	BTTServer::Hash::Hash(const Hash & h)
	{
		for (auto i = 0;i < 20;i++)
			this->hash[i] = h.hash[i];
		return;
	}
#pragma endregion

	const auto GetAllParamNameAndValue = [](std::string requestUrl) 
	{
		std::vector<std::pair<std::string, std::string>> value;
		if (requestUrl.find('?') == -1) return value;
		auto params = requestUrl.substr(requestUrl.find('?') + 1);
		while (params.length() != 0)
		{
			auto paramName = params.substr(0, params.find('='));
			auto paramValueStart = params.find('=') + 1;
			auto paramValueLength = (params.find('&') == -1 ? params.length() : params.find('&')) - paramValueStart;
			auto paramValue = params.substr(paramValueStart, paramValueLength);
			auto param = std::pair<std::string, std::string>(paramName, paramValue);
			value.push_back(param);
			params = params.substr(params.find('&') == -1 ? params.length() : (params.find('&') + 1));
		}
		return value;
	};


	BTTServer::HttpBTRequest::HttpBTRequest(Utils::HttpServer::HttpServer::HttpRequest Request)
	{
		auto params = GetAllParamNameAndValue(Request.RequsetUrl);
	}

	BTTServer::BTTServer(int ListenPort) : Utils::HttpServer::HttpServer(ListenPort)
	{
		this->HttpSocketRequestRecevedEvent += [this](Utils::HttpServer::HttpServer::HttpRequest req)
		{
			auto breq = HttpBTRequest(req);
			this->HttpBTRequestRecevedEvent.Active(breq);
		};
	}

	BTTServer::~BTTServer()
	{
	}

}