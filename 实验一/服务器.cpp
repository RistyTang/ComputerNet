#include<iostream>
#include<WinSock2.h>
#include<cstring>
#include<ctime>
#include<vector>
#include<string>
#pragma comment(lib,"ws2_32.lib")//socket库
using namespace std;
DWORD WINAPI servEventThread(LPVOID& IpParameter);//服务器端收消息线程
DWORD WINAPI ClientThread(LPVOID IpParameter);//服务器端多客户端线程函数声明
int curr_clients;//当前一共有多少个客户端
SOCKET clientsarray[10];//一共十个客户端们
string sendtext;//服务器要分发的消息
int from_client;//收到来自哪个客户端的消息
int main()
{
	WSADATA wsaData;
	//wVersionRequested是Windows Sockets API提供的调用方可使用的最高版本号。
	//高位字节指出副版本(修正)号，低位字节指明主版本号。
	WORD mVersionRequested = MAKEWORD(2, 2);
	//加载套接字库 flag表示是否成功
	int flag = WSAStartup(mVersionRequested, &wsaData);
	time_t now = time(nullptr);
	char* curr_time = ctime(&now);
	if (flag == 0)
	{
		cout << curr_time << "套接字库加载成功" << endl;
	}
	else
	{
		cout << curr_time << "套接字库加载失败" << endl;
	}
	//创建socket，指定地址类型为AF_INET，流式套接字，TCP协议
	SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//为之后绑定套接字做准备
	SOCKADDR_IN ServerAddr;
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");//IP地址
	ServerAddr.sin_port = htons(6666);//端口号
	// 绑定套接字到一个IP地址和一个端口上
	flag = bind(server, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR));
	now = time(nullptr);
	curr_time = ctime(&now);
	if (flag == 0)//判断绑定
	{
		cout << curr_time << "绑定成功" << endl;
	}
	else
	{
		cout << curr_time << "绑定失败" << endl;
	}

	//将套接字设置为监听模式等待连接请求
	flag = listen(server, 10);//同一队列中最多十个请求

	now = time(nullptr);
	curr_time = ctime(&now);
	if (flag == 0)
	{
		cout << curr_time << "监听成功" << endl;
	}
	else
	{
		cout << curr_time << "监听失败" << endl;
	}

	//请求到来后，接受连接请求，返回一个新的对应于此次连接的套接字
	int len = sizeof(SOCKADDR);

	curr_clients = 0;
	//主线程负责接收客户端连接
	while (1)
	{
		SOCKET accepter = accept(server, (SOCKADDR*)&ServerAddr, &len);
		//创建线程用来负责多客户端
		if (curr_clients<10)//不能多于监听数目上限
		{
			clientsarray[curr_clients] = accepter;//记录不同客户端
			HANDLE hThread = CreateThread(NULL, 0, ClientThread, (LPVOID)&accepter, 0, 0);
			CloseHandle(hThread);//关闭对线程的引用
			
		}
		//accepter = INVALID_SOCKET;//接收一个关闭一个
	}
	//不需要句柄所以直接关闭
	//CloseHandle(CreateThread(NULL, 0, servEventThread, (LPVOID)&accepter, 0, 0));


	//关闭套接字，关闭加载的套接字库(closesocket()/WSACleanup())

	closesocket(server);
	WSACleanup();

}
DWORD WINAPI servEventThread(LPVOID IpParameter)
{
	SOCKET accepter = *(SOCKET*)IpParameter;//获取服务器端的SOCKET参数
	time_t now = time(nullptr);
	char* curr_time = ctime(&now);
	char recvtext[500];
	//cout << "进入此函数" << endl;
	for (int i = 0;i < curr_clients;i++)//判断来自于哪个客户端
	{
		if (accepter == clientsarray[i])
			from_client = i;
	}
	while (1)
	{
		int recvlength = recv(accepter, recvtext, 500, 0);
		for (int i = 0;i < curr_clients;i++)//判断来自于哪个客户端
		{
			if (accepter == clientsarray[i])
				from_client = i;
		}
		sendtext = curr_time;//封装时间
		sendtext.append("客户端");
		sendtext.append(to_string(from_client + 1));
		sendtext.append("发来消息:\"");
		string temp=recvtext;
		sendtext.append(temp);
		sendtext.append("\"");//将消息封装打包为时间+客户端i发来消息“--”
		if (recvlength <= 0)
		{
			cout << "客户端"<<from_client+1<<"已经退出聊天。" << endl;
			curr_clients -= 1;
			cout << "当前客户端人数为：" << curr_clients << endl;
			break;
		}
		if (temp == "")
		{

		}
		else
		{
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time;
			cout << "收到来自客户端"<<from_client+1<<"的信息：\"";
			cout << recvtext;
			cout << "\"" << endl;
			//接下来将所收到的消息转发给各个用户
			string  message;//服务器端真正显示的消息
			for (int i = 0;i < curr_clients/*&&i!=from_client*/;i++)
			{
				//cout << "now from client" << from_client << endl;
				if (i != from_client) 
				{
					message = curr_time;
					message.append("给客户端");
					message.append(to_string(i + 1));
					message.append("号发送消息\"");
					message.append(sendtext);
					message.append("\"成功");
					//strcpy(message, now_send_client.c_str());//
					send(clientsarray[i], sendtext.c_str(), 1000, 0);
					now = time(nullptr);
					curr_time = ctime(&now);
					cout << message << endl;
					//cout << sendtext << endl;
					message = "给客户端";//相当于清空数据
				}
			}
			sendtext = "";//清空消息
			//cout << "sendtext=="<<sendtext << endl;
			//break;
			//memset(recvtext, 0, sizeof(recvtext));
		}
		
		//memset(recvtext, 0, sizeof(recvtext));
		temp = "";//清空消息
	}
	return 0;
}
DWORD WINAPI ClientThread(LPVOID IpParameter)//用来跟多个客户端交互。
{
	SOCKET accepter = *(SOCKET*)IpParameter;//获取服务器端的SOCKET参数
	//再开个线程收数据
	HANDLE hThread = CreateThread(NULL, 0, servEventThread, (LPVOID)&accepter, 0, 0);
	CloseHandle(hThread);
	//用返回的套接字和客户端进行通信(send()/recv())；
	cout << "——————————聊天开始————————" << endl;
	curr_clients += 1;
	cout << "当前客户端人数为：" << curr_clients << endl;
	
	//获取当前时间
	//cout << "我在这" << endl;
	//string  message;//服务器端真正显示的消息
	//如：给客户端x发送“--”消息成功
	/*
	while (1)
	{
		//cin>>sendtext;
		//获取当前时间
		time_t now = time(nullptr);
		string curr_time = ctime(&now);
		if (!(strcmp(sendtext.c_str(), "quit()")))//退出聊天信息
		{
			cout << "您已经选择结束聊天。" << endl;
			return 0;
		}
		else if (sendtext == "")
		{
			//cout << "发送消息不能为空！" << endl;
			sendtext = "now no message";
			//break;
		}
		else if(sendtext!="now no message")
		{
			for (int i = 0;i < curr_clients&&i!=from_client;i++)
			{
				message = curr_time;
				message.append( "给客户端");
				message.append(to_string(i+1));
				message.append("发送消息\"");
				message.append(sendtext);
				message.append("\"成功");
				//strcpy(message, now_send_client.c_str());//
				send(clientsarray[i], sendtext.c_str(), sizeof(sendtext), 0);
				now = time(nullptr);
				curr_time = ctime(&now);
				cout << message<< endl;
				message = "给客户端";//相当于清空数据
			}
			sendtext = "now no message";//清空消息
			break;
		}
	}*/
	return 0;
}

