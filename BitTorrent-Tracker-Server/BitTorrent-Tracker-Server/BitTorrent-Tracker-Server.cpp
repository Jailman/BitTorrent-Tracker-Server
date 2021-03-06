// BitTorrent-Tracker-Server.cpp: 定义控制台应用程序的入口点。
//
// This Project Use ISO : C++ 17 Standerd
// 这个项目的代码使用 ISO : C++ 17 标准编写而成

#include "stdafx.h"
#include "BTTServer.h"
#include "HttpServer.h"
#include <iostream>
using std::cout;
using std::endl;
auto func = [](Utils::HttpServer::HttpServer::HttpRequest& req)
{ 
	if (req.RequsetUrl.find("ico") != -1)return;
	cout << "检测到来自" << (int)req.ClientAddr.sin_addr.S_un.S_un_b.s_b1 << "." <<
		(int)req.ClientAddr.sin_addr.S_un.S_un_b.s_b2 << "." <<
		(int)req.ClientAddr.sin_addr.S_un.S_un_b.s_b3 << "." <<
		(int)req.ClientAddr.sin_addr.S_un.S_un_b.s_b4 << "的访问" << endl;
	auto Header = "HTTP/1.1 200 OK\r\nServer: nginx\r\nDate: Wed, 01 Aug 2018 16:46:21 GMT\r\nContent-Type: text/html; charset=UTF-8\r\nConnection: keep-alive\r\n\r\n";
	auto Content = u8"<!doctype html>\r\n<head><meta charset=utf-8></head><h1>这是一个测试页面</h1><br>如果您看到这个页面，说明当前HTTP Server工作正常";
	req.SendBuffer(Header, 135);
	req.SendBuffer(Content, 152);
	return; 
};
int main()
{
	/*Utils::HttpServer::HttpServer server(2333);
	server.HttpSocketRequestRecevedEvent += func;*/

	BTTServer::BTTServer server(2333);



	//Magic, For Debugging, don't touch.
	//麻吉，调试用，别动。
	while (true)Sleep(1);


    return 0;
}

