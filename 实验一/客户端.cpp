#include<iostream>
#include<winsock2.h>
#include<cstring>
#include<ctime>
#pragma comment(lib,"ws2_32.lib")//socket库
using namespace std;
int main()
{
	WSADATA wsaData;
	//wVersionRequested是Windows Sockets API提供的调用方可使用的最高版本号。
	//高位字节指出副版本(修正)号，低位字节指明主版本号。
	WORD mVersionRequested = MAKEWORD(2, 2);
	//加载套接字库 flag表示是否成功
	int flag=WSAStartup(mVersionRequested, &wsaData);
	//获取当前时间
	time_t now = time(nullptr);
	char* curr_time = ctime(&now);
	if (flag == 0)
	{
		cout <<curr_time<< "套接字库加载成功" << endl;
	}
	else
	{
		cout <<curr_time<< "套接字库加载失败" << endl;
	}
	//创建socket，指定地址类型为AF_INET，流式套接字，TCP协议
	SOCKET client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//为之后连接和传输数据做准备
	SOCKADDR_IN ClientAddr;
	ClientAddr.sin_family = AF_INET;
	ClientAddr.sin_addr.s_addr = inet_addr("127.0.0.1");//IP地址
	ClientAddr.sin_port = htons(6666);//端口号
	//进行连接
	flag=connect(client, (SOCKADDR*)&ClientAddr, sizeof(SOCKADDR));
	now = time(nullptr);
	curr_time = ctime(&now);
	if (flag == 0)
	{
		cout << curr_time << "建立连接成功" << endl;
	}
	else
	{
		cout << curr_time << "建立连接失败" << endl;
	}
	//创建接受消息的线程
	DWORD WINAPI recvMsgThread(LPVOID IpParameter);
	//不许要任何句柄，所以直接关闭
	CloseHandle(CreateThread(NULL, 0, recvMsgThread, (LPVOID)&client, 0, 0));

	//发送消息
	char clienttext[500] =  {};
	cout << "——————————聊天开始————————" << endl;
	while (1)
	{
		//cin >> clienttext;
		cin.getline(clienttext, 499);
		if (!(strcmp(clienttext, "quit()")))//退出聊天信息
		{
			cout << "您已经退出聊天" << endl;
			break;
		}
		send(client, clienttext, sizeof(clienttext), 0);
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "信息\""<<clienttext<<"\"发送成功" << endl;
		memset(clienttext, 0, sizeof(clienttext));
	}
	//关闭套接字\套接字库
	//send(client, "聊天结束\n", sizeof("聊天结束\n"), 0);
	closesocket(client);
	WSACleanup();
	return 0;
}
DWORD WINAPI recvMsgThread(LPVOID IpParameter)//接收消息的线程
{
	SOCKET client = *(SOCKET*)IpParameter;//获取客户端的SOCKET参数
	char servertext[1000];//接收到的消息
	string printtext;
	while (1)
	{
		//获取当前时间
		time_t now = time(0);
		char* curr_time = ctime(&now);
		int recvlength=recv(client, servertext, sizeof(servertext), 0);//sizeof(servertext)
		printtext = servertext;
		if (recvlength<0)
		{
			cout << "聊天已经结束！" << endl;
			break;
		}
		if (recvlength == 0||printtext=="")//没收到消息就什么也不干
		{

		}
		else 
		{
			//cout << "recvlength=" << recvlength << endl;
			now = time(nullptr);
			curr_time = ctime(&now);
			//cout << curr_time;
			cout << servertext << endl;
			//memset(servertext, 0, sizeof(servertext));
			printtext = "";
		}
		printtext = "";
		//memset(servertext, 0, sizeof(servertext));
	}
	return 0;
}