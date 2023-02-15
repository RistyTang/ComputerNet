#include<iostream>
#include<WinSock2.h>
#include<fstream>
#include<string>
#include<time.h>
#include<Windows.h>
#pragma comment(lib,"ws2_32.lib")//socket库
using namespace std;
const unsigned char SYN = 0x01;//SYN=1 ACK=0
const unsigned char ACK = 0x02;//SYN=0 ACK=1
const unsigned char SYN_ACK = 0x03;//SYN=1 ACK=1
const unsigned char FIN = 0x04;//FYN=1
const unsigned char FIN_ACK = 0x05;//FYN=1 ACK=1
const unsigned char FINAL_ACK = 0x06;//成功接收到最后一个包
const unsigned char FINAL = 0x07;//标识为最后一个包
time_t now;
char* curr_time = ctime(&now);

struct UDPpackethead
{
	unsigned int seq;//序列号 32位
	u_short Check;//校验 16位
	u_short len;//数据部分总长度
	unsigned char flags;//标志位 
	UDPpackethead()
	{
		len = 0;
		seq = 0;
		Check = 0;
		flags = 0;
	}
};
struct UDPpacket
{
	UDPpackethead head;
	char data[2048];//每段data长度上限
};
//计算校验和 抄ppt
u_short packetcheck(u_short* packet, int packelength)
{
	//UDP检验和的计算方法是：
	//按每16位求和得出一个32位的数；
	//如果这个32位的数，高16位不为0，则高16位加低16位再得到一个32位的数；
	//重复第2步直到高16位为0，将低16位取反，得到校验和。
	//register关键字命令编译器尽可能的将变量存在CPU内部寄存器中
	//而不是通过内存寻址访问以提高效率。
	//u_long32位，不能使用unsigned int 只有16位
	register u_long sum = 0;
	int count = (packelength + 1) / 2;//两个字节的计算
	//u_short* buf = (u_short*)malloc(packelength + 1);
	u_short* buf = new u_short[packelength + 1];
	memset(buf, 0, packelength + 1);
	memcpy(buf, packet, packelength);
	//cout << "count=" << packelength << endl;
	while (count--)
	{
		sum += *buf++;
		if (sum & 0xFFFF0000)
		{
			sum &= 0xFFFF;
			sum++;
		}
	}
	return ~(sum & 0xFFFF);
}
//握手建立连接阶段
//客户端发送请求连接数据报SYN——第一次握手
//服务器向客户端发送一个SYN_ACK——第二次握手
//客户端发送ACK——第三次握手
int connect(SOCKET& server, SOCKADDR_IN& ServerAddr, int& len)
{
	//设置为非阻塞模式，避免卡在recvfrom
	int iMode = 1; //0：阻塞
	ioctlsocket(server, FIONBIO, (u_long FAR*) & iMode);//非阻塞设置
	UDPpacket pack1;
	//缓冲区
	char* buffer1 = new char[sizeof(pack1)];
	//接收第一次握手请求信息
	now = time(nullptr);
	curr_time = ctime(&now);
	cout << curr_time << "[System]:";
	cout << "进入连接阶段" << endl;
	int flag;
	while (1)
	{
		//没收到包就一直循环
		flag = recvfrom(server, buffer1, sizeof(pack1), 0, (sockaddr*)&ServerAddr, &len);
		if (flag <= 0)
		{
			continue;
		}
		//接收成功
		memcpy(&(pack1), buffer1, sizeof(pack1.head));
		//cout << "服务器收到head为：check" << pack1.head.Check << "len:" << pack1.head.len << "flags:" << (u_short)pack1.head.flags << "seq:" << pack1.head.seq << endl;
		u_short res = packetcheck((u_short*)&pack1, sizeof(pack1));
		//只有当收到的数据包正确时才算是成功
		if (pack1.head.flags == SYN && res == 0)
		{
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			cout << "——————第一次握手接收成功————" << endl;
			break;
		}
		//如果发来的包不正确的话，继续这个循环，等待客户端重新发送
	}
	//第二次握手，服务器发送SYN_ACK
	UDPpacket pack2;
	pack2.head.flags = SYN_ACK;
	pack2.head.Check = packetcheck((u_short*)&pack2, sizeof(pack2));
	//发送缓冲区
	char* buffer2 = new char[sizeof(pack2)];
	memcpy(buffer2, &pack2, sizeof(pack2));
	flag = sendto(server, buffer2, sizeof(pack2), 0, (sockaddr*)&ServerAddr, len);
	//超时重传机制
	clock_t starttime = clock();//第二次握手发送时间
	if (flag == -1)//第二次握手发送失败，重传
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "——————第二次握手发送失败————" << endl;
	}
	else
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "——————第二次握手发送成功————" << endl;
	}
	//第三次握手 服务器接收
	UDPpacket pack3;
	//缓冲区
	char* buffer3 = new char[sizeof(pack3)];
	//如果一直没有收到包的话，说明上一个包需要重传
	while (1)
	{
		//超时重传
		if (clock_t() - starttime >= CLOCKS_PER_SEC)
		{
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			cout << "——————第二次握手包正在重传————" << endl;
			sendto(server, buffer2, sizeof(pack2), 0, (sockaddr*)&ServerAddr, len);
			starttime = clock();//更新发送时间
		}
		//没收到包就一直循环
		flag = recvfrom(server, buffer3, sizeof(pack3), 0, (sockaddr*)&ServerAddr, &len);
		if (flag <= 0)
		{
			continue;
		}
		//第三次包接收成功
		memcpy(&(pack3), buffer3, sizeof(pack3));
		//cout << "服务器收到head为：check" << pack3.head.Check << "len:" << pack3.head.len << "flags:" << (u_short)pack3.head.flags << "seq:" << pack3.head.seq << endl;
		u_short res = packetcheck((u_short*)&pack3, sizeof(pack3));
		if (pack3.head.flags == ACK && res == 0)
		{
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			cout << "——————第三次握手接收成功————" << endl;
			break;//跳出循环
		}
		//否则的话就一直等到能接收正确的包
	}
	//等两个时钟周期
	starttime = clock();
	while ((clock() - starttime) <= 2 * CLOCKS_PER_SEC)
	{
	}
	iMode = 0; //0：阻塞
	ioctlsocket(server, FIONBIO, (u_long FAR*) & iMode);//恢复成阻塞模式
	return 1;
}
//挥手
//客户端发送FIN
//服务器发送FIN_ACK
//服务器发送FIN
//客户端发送FIN_ACK
int disconnect(SOCKET& server, SOCKADDR_IN& ServerAddr, int& len)
{
	//设置为非阻塞模式，避免卡在recvfrom
	int iMode = 1; //0：阻塞
	ioctlsocket(server, FIONBIO, (u_long FAR*) & iMode);//非阻塞设置
	UDPpacket pack1;
	//缓冲区
	char* buffer1 = new char[sizeof(pack1)];
	int flag;
	//接收第一次挥手请求信息
	while (1)
	{
		flag = recvfrom(server, buffer1, sizeof(pack1), 0, (sockaddr*)&ServerAddr, &len);
		//没有收到消息则一直循环
		if (flag <= 0)
		{
			continue;
		}
		//接收成功
		memcpy(&(pack1), buffer1, sizeof(pack1));
		u_short res = packetcheck((u_short*)&pack1, sizeof(pack1));
		//是第一次握手
		if (pack1.head.flags == FIN && res == 0)
		{
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			cout << "——————第一次挥手接收成功————" << endl;
			break;
		}
		//否则就一直等传送来正确的包
	}
	//第二次挥手，服务器发送FIN_ACK
	UDPpacket pack2;
	pack2.head.flags = FIN_ACK;
	pack2.head.Check = packetcheck((u_short*)&pack2, sizeof(pack2));
	//发送缓冲区
	char* buffer2 = new char[sizeof(pack2)];
	memcpy(buffer2, &pack2, sizeof(pack2));
	flag = sendto(server, buffer2, sizeof(pack2), 0, (sockaddr*)&ServerAddr, len);
	if (flag == -1)
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "——————第二次挥手发送失败————" << endl;
	}
	else
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "——————第二次挥手发送成功————" << endl;
	}
	clock_t starttime = clock();//记录第二次挥手发送时间
	while (clock() - starttime <= 2 * CLOCKS_PER_SEC)
	{
		//没收到
		flag = recvfrom(server, buffer1, sizeof(pack1), 0, (sockaddr*)&ServerAddr, &len);
		if (flag <= 0)
		{
			continue;
		}
		else//收到了，说明第二次挥手没发出去，需要重新发送
		{
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			cout << "超时重发第二次挥手" << endl;
			sendto(server, buffer2, sizeof(pack2), 0, (sockaddr*)&ServerAddr, len);
			//更新发送时间
			starttime = clock();
		}
	}

	//第三次挥手，服务器要发送FIN
	UDPpacket pack3;
	pack3.head.flags = FIN;
	pack3.head.Check = packetcheck((u_short*)&pack3, sizeof(pack3));
	//发送缓冲区
	char* buffer3 = new char[sizeof(pack3)];
	memcpy(buffer3, &pack3, sizeof(pack3));
	flag = sendto(server, buffer3, sizeof(pack3), 0, (sockaddr*)&ServerAddr, len);
	if (flag == -1)
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "——————第三次挥手发送失败————" << endl;
	}
	else
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "——————第三次挥手发送成功————" << endl;
	}
	starttime = clock();//第三次挥手发送时间
	//接收第四次挥手请求信息
	UDPpacket pack4;
	//缓冲区
	char* buffer4 = new char[sizeof(pack4)];
	while (1)
	{
		if (clock() - starttime >= 2 * CLOCKS_PER_SEC)
		{
			starttime = clock();
			//重新发送第三次挥手
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			cout << "超时重发第三次挥手" << endl;
			sendto(server, buffer3, sizeof(pack3), 0, (sockaddr*)&ServerAddr, len);
		}
		flag = recvfrom(server, buffer4, sizeof(pack4), 0, (sockaddr*)&ServerAddr, &len);
		if (flag <= 0)
		{
			continue;
		}
		//接收成功
		memcpy(&(pack4), buffer4, sizeof(pack4));
		u_short res = packetcheck((u_short*)&pack4, sizeof(pack4));
		//是第四次挥手
		if (pack4.head.flags == FIN_ACK && res == 0)
		{
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			cout << "——————第四次挥手接收成功————" << endl;
			break;
		}
		//否则的话一直循环等待
	}
	return 1;
}

//接收消息
u_long ReceiveMessage(char* message, int& addrlength, SOCKET& server, SOCKADDR_IN& ClientAddr)
{
	unsigned int filelength = 0;//记录文件长度
	UDPpacket recvpacket, sendpacket;
	//接收缓冲区
	char* recvbuffer = new char[sizeof(recvpacket)];
	//发送缓冲区
	char* sendbuffer = new char[sizeof(sendpacket)];
	unsigned int Ssequence = 0;//当前按序接收到最高序号分组
	while (1)
	{
		//接收报文长度
		int recvlength = recvfrom(server, recvbuffer, sizeof(recvpacket), 0, (sockaddr*)&ClientAddr, &addrlength);
		//cout << "recvlength=" << recvlength << endl;
		memcpy(&recvpacket, recvbuffer, sizeof(recvpacket));
		u_short res = packetcheck((u_short*)&recvpacket, sizeof(recvpacket));
		//u_short res = packetcheck((u_short*)&recvpacket.head, sizeof(recvpacket.head));
		//cout << "服务器收到head为：check" << recvpacket.head.Check << "len:" << recvpacket.head.len << "flags:" << (u_short)recvpacket.head.flags << "seq:" << recvpacket.head.seq << endl;
		//cout << "校验结果为：" << res << endl;
		if (res != 0)//出错了
		{
			cout << "信息传输校验码错误！" << endl;
			sendpacket.head.flags = ACK;
			//此时传回的sequence仍然为上一次传送的值
			//表示客户端之前传来的包错误
			sendpacket.head.Check = 0;
			sendpacket.head.seq = Ssequence;
			//计算校验和
			sendpacket.head.Check = packetcheck((u_short*)&sendpacket, sizeof(sendpacket));
			memcpy(sendbuffer, &sendpacket, sizeof(sendpacket));
			//期待重传
			int flag = sendto(server, sendbuffer, sizeof(sendpacket), 0, (sockaddr*)&ClientAddr, addrlength);
			if (flag == -1)
			{
				cout << "发送失败" << endl;
			}
			continue;
		}
		else//至少校验码是对的
		{
			//判断一下这是不是最后一个包
			if ((recvpacket.head.flags == FINAL) && (Ssequence == recvpacket.head.seq - 1))
			{
				//放进消息里面
				memcpy(message + filelength, recvpacket.data, recvpacket.head.len);
				filelength += recvpacket.head.len;
				now = time(nullptr);
				curr_time = ctime(&now);
				cout << curr_time << "[System]:";
				cout << "————最后一个包seq:" << recvpacket.head.seq << "已经接收完毕————" << endl;
				//Ssequence += 1;
				Ssequence = recvpacket.head.seq;
				sendpacket.head.flags = FINAL_ACK;
				sendpacket.head.Check = 0;
				sendpacket.head.seq = Ssequence;
				sendpacket.head.Check = packetcheck((u_short*)&sendpacket, sizeof(sendpacket));
				//cout << "服务器发送的确认包head为：check" << sendpacket.head.Check << "len:" << sendpacket.head.len << "flags:" << (u_short)sendpacket.head.flags << "seq:" << sendpacket.head.seq << endl;
				memcpy(sendbuffer, &sendpacket, sizeof(sendpacket));
				int flag = sendto(server, sendbuffer, sizeof(sendpacket), 0, (sockaddr*)&ClientAddr, addrlength);
				clock_t starttime = clock();//最后一个ACK的发送时间
				now = time(nullptr);
				curr_time = ctime(&now);
				cout << curr_time << "[System]:";
				cout << "发送最后一个确认包seq:" << sendpacket.head.seq << "  datalength:" << sendpacket.head.len << "  Check:" << sendpacket.head.Check << "  flags:" << (u_short)sendpacket.head.flags << endl;
				//为了防止最后一个包的ACK发送丢失，需要等2MSL
				//设置为非阻塞模式，避免卡在recvfrom
				int iMode = 1; //0：阻塞
				ioctlsocket(server, FIONBIO, (u_long FAR*) & iMode);//非阻塞设置
				while (clock() - starttime <= 2 * CLOCKS_PER_SEC)
				{
					if (recvfrom(server, recvbuffer, sizeof(recvpacket), 0, (sockaddr*)&ClientAddr, &addrlength) <= 0)
					{
						//一直没有收到包，说明对面正确接收了ACK
						continue;
					}
					//否则需要重传最后一个包的ACK
					else
					{
						now = time(nullptr);
						curr_time = ctime(&now);
						cout << curr_time << "[System]:";
						cout << "重传最后一个确认包seq:" << sendpacket.head.seq << "  datalength:" << sendpacket.head.len << "  Check:" << sendpacket.head.Check << "  flags:" << (u_short)sendpacket.head.flags << endl;
						sendto(server, sendbuffer, sizeof(sendpacket), 0, (sockaddr*)&ClientAddr, addrlength);
						starttime = clock();
					}
				}
				//恢复阻塞模式
				iMode = 0; //0：阻塞
				ioctlsocket(server, FIONBIO, (u_long FAR*) & iMode);//非阻塞设置
				return filelength;
			}
			else//后面还有别的包
			{
				//如果收到的包不是想要的包的话
				if (Ssequence != recvpacket.head.seq - 1)
				{
					//说明出了问题，返回ACK
					sendpacket.head.flags = ACK;
					sendpacket.head.len = 0;
					sendpacket.head.Check = 0;
					//此时仍然应该返回之前的seq
					sendpacket.head.seq = Ssequence;
					//计算校验和
					sendpacket.head.Check = packetcheck((u_short*)&sendpacket, sizeof(sendpacket));
					memcpy(sendbuffer, &sendpacket, sizeof(sendpacket));
					//重发该包的ACK
					int flag = sendto(server, sendbuffer, sizeof(sendpacket), 0, (sockaddr*)&ClientAddr, addrlength);
					if (flag == -1)
					{
						//int i=WSAGetLastError();
						//cout << "wrong code:" << i << endl;
						//错误代码10038
						now = time(nullptr);
						curr_time = ctime(&now);
						cout << curr_time << "[System]:";
						//重发数据包
						cout << "重新发送确认包seq:" << sendpacket.head.seq << "  datalength:" << sendpacket.head.len << "  Check:" << sendpacket.head.Check << "  flags:" << (u_short)sendpacket.head.flags << "失败" << endl;
						continue;//丢弃该数据包
					}
					else
					{
						now = time(nullptr);
						curr_time = ctime(&now);
						cout << curr_time << "[System]:";
						cout << "重新发送确认包seq:" << sendpacket.head.seq << "  datalength:" << sendpacket.head.len << "  Check:" << sendpacket.head.Check << "  flags:" << (u_short)sendpacket.head.flags << "成功" << endl;
						continue;
					}

				}
				else//收到的数据包是此时服务器想要的
				{
					//放进消息里面
					memcpy(message + filelength, recvpacket.data, recvpacket.head.len);
					filelength += recvpacket.head.len;
					//接下来发送ACK
					Ssequence = recvpacket.head.seq;
					sendpacket.head.flags = ACK;
					sendpacket.head.seq = Ssequence;
					sendpacket.head.Check = 0;
					sendpacket.head.Check = packetcheck((u_short*)&sendpacket, sizeof(sendpacket));
					memcpy(sendbuffer, &sendpacket, sizeof(sendpacket));
					int flag = sendto(server, sendbuffer, sizeof(sendpacket), 0, (SOCKADDR*)&ClientAddr, addrlength);
					//为什么第三个发送失败？——buffer太小了……
					if (flag == -1)
					{
						now = time(nullptr);
						curr_time = ctime(&now);
						cout << curr_time << "[System]:";
						//cout << "正确接收包：" << recvpacket.head.seq << endl;
						cout << "——————ACK发送失败————" << endl;
					}
					else
					{
						now = time(nullptr);
						curr_time = ctime(&now);
						cout << curr_time << "[System]:";
						//cout << "正确接收包：" << recvpacket.head.seq << endl;
						cout << "发送确认包seq:" << sendpacket.head.seq << "  datalength:" << sendpacket.head.len << "  Check:" << sendpacket.head.Check << "  flags:" << (u_short)sendpacket.head.flags << endl;
					}

				}

			}

		}
	}
	//

}

int main()
{
	WSADATA wsaData;
	//wVersionRequested是Windows Sockets API提供的调用方可使用的最高版本号。
	//高位字节指出副版本(修正)号，低位字节指明主版本号。
	WORD mVersionRequested = MAKEWORD(2, 2);
	//加载套接字库 flag表示是否成功
	int flag = WSAStartup(mVersionRequested, &wsaData);
	if (flag == 0)
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "套接字库加载成功" << endl;
	}
	else
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "套接字库加载失败" << endl;
	}
	//创建socket，指定地址类型为AF_INET，数据报套接字
	SOCKET server = socket(AF_INET, SOCK_DGRAM, 0);
	//为之后绑定套接字做准备
	SOCKADDR_IN ServerAddr;
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");//IP地址
	ServerAddr.sin_port = htons(7777);//端口号
	// 绑定套接字到一个IP地址和一个端口上
	flag = bind(server, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr));
	if (flag == 0)//判断绑定
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "绑定成功" << endl;
	}
	else
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "绑定失败" << endl;
		closesocket(server);
		WSACleanup();
		return 0;
	}
	//三次握手建立连接
	int len = sizeof(ServerAddr);
	flag = connect(server, ServerAddr, len);
	if (flag == 1)//判断绑定
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "建立连接成功" << endl;
	}
	else
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "建立连接失败" << endl;
		closesocket(server);
		WSACleanup();
		return 0;
	}
	int addrlen = sizeof(ServerAddr);
	//数据接收
	int namelength = 0;
	int datalength = 0;
	char recvname[100];//文件的名字
	char recvdata[100000000];//文件数据内容
	now = time(nullptr);
	curr_time = ctime(&now);
	cout << curr_time << "[System]:";
	cout << "————开始接收文件名————" << endl;
	namelength = ReceiveMessage(recvname, addrlen, server, ServerAddr);
	now = time(nullptr);
	curr_time = ctime(&now);
	cout << curr_time << "[System]:";
	cout << "————文件名称接收成功————" << endl;
	now = time(nullptr);
	curr_time = ctime(&now);
	cout << curr_time << "[System]:";
	cout << "————开始接收文件内容————" << endl;
	datalength = ReceiveMessage(recvdata, addrlen, server, ServerAddr);
	now = time(nullptr);
	curr_time = ctime(&now);
	cout << curr_time << "[System]:";
	cout << "————文件内容接收成功————" << endl;
	recvname[namelength] = '\0';
	recvdata[datalength] = '\0';
	string filename(recvname);
	now = time(nullptr);
	curr_time = ctime(&now);
	cout << curr_time << "[System]:";
	cout << "文件名为：\"" << filename << "\"" << endl;
	//存储接收到的数据
	ofstream out(filename.c_str(), ofstream::binary);
	for (int i = 0; i < datalength; i++) {
		out << recvdata[i];
	}
	out.close();
	now = time(nullptr);
	curr_time = ctime(&now);
	cout << curr_time << "[System]:";
	cout << "文件\"" << filename << "\"" << "已经收到并写入" << endl;
	//挥手
	flag = disconnect(server, ServerAddr, addrlen);
	if (flag != 0)//判断绑定
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "断开连接成功" << endl;
	}
	else
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "断开连接失败" << endl;
	}
	closesocket(server);
	WSACleanup();
	return 0;
}