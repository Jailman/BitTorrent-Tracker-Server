// BitTorrent-Tracker-Server.cpp: 定义控制台应用程序的入口点。
//
#include "stdafx.h"
#include "BTTServer.h"
int main()
{
	BTTServer server(2333);
	while (true)Sleep(1);
    return 0;
}

