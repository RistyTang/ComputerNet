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
//以下是RENO算法涉及的三个状态
const unsigned char SLOW_START = 0x08;//慢启动
const unsigned char CONGESTION_AVOID = 0x09;//拥塞避免
const unsigned char QUICK_RECOVERY = 0x10;//快速恢复
struct sendwindowstruct//发送窗口
{
	unsigned int size;//窗口大小
	unsigned int ceiling;//上限
	unsigned int floor;//下限，已发送的最大seq
	unsigned int sentbutnotchecked;//记录已发送但未确认的最大seq
	unsigned int ssthresh;//慢开始门限
	unsigned int curACKseq;
	unsigned int dupACKcount;//重复ACK的个数
	unsigned char state;//当前状态
	sendwindowstruct()
	{
		size = 1;//窗口大小初始化为1
		floor = 1;//从1开始编号
		ceiling = 1;//上限
		sentbutnotchecked = 0;//初始时没有包被确认
		ssthresh = 14;
		state = SLOW_START;//初始状态为慢启动状态
	}
};
struct UDPpackethead
{
	unsigned int seq;//序列号 16位
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
//全局变量
time_t now;
char* curr_time = ctime(&now);
sendwindowstruct sendwindow;
int packetnum;
bool finished = 0;//消息传送完成
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
	u_short* buf = (u_short*)malloc(packelength + 1);
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
int connect(SOCKET& client, SOCKADDR_IN& ClientAddr, int& len)
{
	//设置为非阻塞模式，避免卡在recvfrom
	int iMode = 1; //0：阻塞
	ioctlsocket(client, FIONBIO, (u_long FAR*) & iMode);//非阻塞设置
	//第一次握手，客户端发送SYN
	UDPpacket pack1;
	pack1.head.flags = SYN;
	//pack1.head.Check = packetcheck((u_short*)&(pack1.head), sizeof(pack1.head));
	pack1.head.Check = packetcheck((u_short*)&(pack1), sizeof(pack1));
	//cout << "客户端head为：check" << pack1.head.Check << "len:" << pack1.head.len << "flags:" << (u_short)pack1.head.flags << "seq:" << pack1.head.seq << endl;
	//发送缓冲区
	char* buffer1 = new char[sizeof(pack1)];
	memcpy(buffer1, &pack1, sizeof(pack1));
	//s：已建好连接的socket,如果利用UDP协议则不需经过connect操作。
	//msg：待发送数据的缓冲区
	//len：缓冲区长度
	//flags：调用方式标志位, 一般为0, 改变flags，将会改变sendto发送的形式。
	//addr：（可选）指针，指向目的套接字的地址
	//tolen：addr所指地址的长度
	int flag = sendto(client, buffer1, sizeof(pack1), 0, (sockaddr*)&ClientAddr, len);
	//第一次握手发送时间
	clock_t pack1starttime = clock();
	if (flag == -1)
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "——————第一次握手发送失败————" << endl;
	}
	else
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "——————第一次握手发送成功————" << endl;
	}
	//第二次握手，等待服务器发来的SYN_ACK
	UDPpacket pack2;
	//缓冲区
	char* buffer2 = new char[sizeof(pack2)];
	//一直没有收到包，说明之前的包出问题了，要重新发送第一次握手
	while (1)
	{
		//如果超时则重传
		if (clock() - pack1starttime >= 2 * CLOCKS_PER_SEC)
		{
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			cout << "————第一次握手包超时重传————" << endl;
			sendto(client, buffer1, sizeof(pack1), 0, (sockaddr*)&ClientAddr, len);
			pack1starttime = clock();
		}
		flag = recvfrom(client, buffer2, sizeof(pack2), 0, (sockaddr*)&ClientAddr, &len);
		if (flag <= 0)//没收到包的话就继续recvfrom
		{
			continue;
		}
		//收到了包
		memcpy(&pack2, buffer2, sizeof(pack2));
		int res = packetcheck((u_short*)&pack2, sizeof(pack2));
		//是第二次握手
		if (pack2.head.flags == SYN_ACK && res == 0)
		{
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			cout << "——————第二次握手接收成功————" << endl;
			break;
		}
		//否则一直处于等待状态
	}
	//第三次握手 客户端发送ACK
	UDPpacket pack3;
	pack3.head.flags = ACK;
	pack3.head.Check = packetcheck((u_short*)&pack3, sizeof(pack3));
	//发送缓冲区
	char* buffer3 = new char[sizeof(pack3)];
	memcpy(buffer3, &pack3, sizeof(pack3));
	flag = sendto(client, buffer3, sizeof(pack3), 0, (sockaddr*)&ClientAddr, len);
	if (flag == -1)
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "——————第三次握手发送失败————" << endl;
		return 0;
	}
	else
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "——————第三次握手发送成功————" << endl;
	}
	clock_t checktime = clock();//记录发送第三个包的时间
	while ((clock() - checktime) <= 2 * CLOCKS_PER_SEC)
	{
		//又把第二个包传过来了
		//否则对面应该就结束传包
		if (recvfrom(client, buffer2, sizeof(pack2), 0, (sockaddr*)&ClientAddr, &len) <= 0)
		{
			continue;
		}
		//如果收到了包，那么就说明第三个包对面没收到，重传
		sendto(client, buffer3, sizeof(pack3), 0, (sockaddr*)&ClientAddr, len);
		checktime = clock();
	}
	iMode = 0; //0：阻塞
	ioctlsocket(client, FIONBIO, (u_long FAR*) & iMode);//恢复成阻塞模式
	//cout << "客户端head为：check" << pack3.head.Check << "len:" << pack3.head.len << "flags:" << (u_short)pack3.head.flags << "seq:" << pack3.head.seq << endl;
	return 1;
}
//挥手
//客户端发送FIN
//服务器发送FIN_ACK
//服务器发送FIN
//客户端发送FIN_ACK
int disconnect(SOCKET& client, SOCKADDR_IN& ClientAddr, int len)
{
	//设置为非阻塞模式，避免卡在recvfrom
	int iMode = 1; //0：阻塞
	ioctlsocket(client, FIONBIO, (u_long FAR*) & iMode);//非阻塞设置
	//第一次挥手，客户端发送FIN
	UDPpacket pack1;
	pack1.head.flags = FIN;
	pack1.head.Check = packetcheck((u_short*)&pack1, sizeof(pack1));
	//发送缓冲区
	char* buffer1 = new char[sizeof(pack1)];
	memcpy(buffer1, &pack1, sizeof(pack1));
	int flag = sendto(client, buffer1, sizeof(pack1), 0, (sockaddr*)&ClientAddr, len);
	if (flag == -1)
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "——————第一次挥手发送失败————" << endl;
	}
	else
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "——————第一次挥手发送成功————" << endl;
	}
	//记录第一次挥手发送时间
	clock_t starttime = clock();
	//第二次挥手，接收服务器发来的FIN_ACK
	UDPpacket pack2;
	//缓冲区
	char* buffer2 = new char[sizeof(pack2)];
	//接收第二次挥手请求信息
	while (1)
	{
		if (clock() - starttime >= 2 * CLOCKS_PER_SEC)
		{
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			//超时了，重传
			cout << "超时重发第一次挥手" << endl;
			sendto(client, buffer1, sizeof(pack1), 0, (sockaddr*)&ClientAddr, len);
			starttime = clock();
		}
		flag = recvfrom(client, buffer2, sizeof(pack2), 0, (sockaddr*)&ClientAddr, &len);
		if (flag <= 0)
		{
			continue;
		}
		//接收成功
		memcpy(&(pack2), buffer2, sizeof(pack2));
		u_short res = packetcheck((u_short*)&pack2, sizeof(pack2));
		//是第二次挥手
		if (pack2.head.flags == FIN_ACK && res == 0)
		{
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			cout << "——————第二次挥手接收成功————" << endl;
			break;
		}
		//传来的包不正确则一直等待
	}
	starttime = clock();
	//等待两个时钟周期，跟server同步
	while (clock() - starttime <= CLOCKS_PER_SEC)
	{

	}
	//接收第三次挥手信息，头部为ACK
	UDPpacket pack3;
	//缓冲区
	char* buffer3 = new char[sizeof(pack3)];
	//接收第三次挥手请求信息
	while (1)
	{
		flag = recvfrom(client, buffer3, sizeof(pack3), 0, (sockaddr*)&ClientAddr, &len);
		if (flag <= 0)
		{
			continue;
		}
		//接收成功
		memcpy(&(pack3), buffer3, sizeof(pack3));
		u_short res = packetcheck((u_short*)&pack3, sizeof(pack3));
		//是第三次挥手
		if (pack3.head.flags == FIN && res == 0)
		{
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			cout << "——————第三次挥手接收成功————" << endl;
			break;
		}
		//否则一直处于等待状态
	}
	//第四次挥手，客户端发送FIN_ACK
	UDPpacket pack4;
	pack4.head.flags = FIN_ACK;
	pack4.head.Check = packetcheck((u_short*)&pack4, sizeof(pack4));
	//发送缓冲区
	char* buffer4 = new char[sizeof(pack4)];
	memcpy(buffer4, &pack4, sizeof(pack4));
	flag = sendto(client, buffer4, sizeof(pack4), 0, (sockaddr*)&ClientAddr, len);
	if (flag == -1)
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "——————第四次挥手发送失败————" << endl;
	}
	else
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "——————第四次挥手发送成功————" << endl;
	}
	starttime = clock();//第四次挥手发送时间
	while (clock() - starttime <= 2 * CLOCKS_PER_SEC)
	{
		//没收到
		if (recvfrom(client, buffer3, sizeof(pack1), 0, (sockaddr*)&ClientAddr, &len) <= 0)
		{
			continue;
		}
		else//收到了，说明第四次挥手没发出去，需要重新发送
		{
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			cout << "超时重发第四次挥手" << endl;
			sendto(client, buffer4, sizeof(pack4), 0, (sockaddr*)&ClientAddr, len);
			//更新发送时间
			starttime = clock();
		}
	}
	return 1;
}
//接收消息的线程
DWORD WINAPI recvMsgThread(LPVOID IpParameter)
{
	SOCKET client = *(SOCKET*)IpParameter;//获取客户端的SOCKET参数
	SOCKADDR_IN ClientAddr = *(SOCKADDR_IN*)IpParameter;//获取clientaddr
	//设置为非阻塞模式，便于在之后进行超时判断
	int iMode = 1; //0：阻塞
	ioctlsocket(client, FIONBIO, (u_long FAR*) & iMode);//非阻塞设置
	UDPpacket recvpacket;//来自服务器发回的数据包
	//接收缓冲区
	char* recvbuffer = new char[sizeof(recvpacket)];
	int addrlength = sizeof(ClientAddr);
	while (1)
	{
		while (recvfrom(client, recvbuffer, sizeof(recvpacket), 0, (sockaddr*)&ClientAddr, &addrlength) <= 0)
		{

		}
		//收到包后需要判断包的正确性
		memcpy(&recvpacket, recvbuffer, sizeof(recvpacket));
		//cout << "客户端收到的ACK包head为：check" << recvpacket.head.Check << "  len:" << recvpacket.head.len << "  flags:" << (u_short)recvpacket.head.flags << "  seq:" << recvpacket.head.seq << endl;
		//计算校验和
		u_short res = packetcheck((u_short*)&recvpacket, sizeof(recvpacket));
		//累积确认思想：收到新的ACK包时，判断以下条件：
		//1.校验码正确
		//2.收到的ACK包序号应当>=当前窗口底部
		if (res == 0 && recvpacket.head.flags == FINAL_ACK)
		{
			finished = 1;//全部传输完成
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			//文件全部正确传送，结束
			cout << "文件已经全部正确传送完毕" << endl;
			//等待2MSL，防止和挥手包混淆
			clock_t starttime = clock();
			while (clock() - starttime <= 2 * CLOCKS_PER_SEC)
			{

			}
			return 1;//关闭线程
		}
		else if (res == 0 && recvpacket.head.seq >= sendwindow.floor)
		{
			//慢启动阶段收到新的ACK，cwnd翻倍
			if (sendwindow.state == SLOW_START)
			{
				//对应窗口前移
				sendwindow.floor = recvpacket.head.seq + 1;
				sendwindow.size *= 2;
				sendwindow.ceiling = sendwindow.floor + sendwindow.size - 1;
				if (sendwindow.ceiling >= packetnum)
				{
					sendwindow.ceiling = packetnum;
				}
				sendwindow.curACKseq = recvpacket.head.seq;
				sendwindow.dupACKcount = 0;
				now = time(nullptr);
				curr_time = ctime(&now);
				cout << curr_time << "[System]:当前为慢启动阶段，窗口大小为：" << sendwindow.size << "。" << endl;
				if (sendwindow.size >= sendwindow.ssthresh)
				{
					sendwindow.state = CONGESTION_AVOID;
					now = time(nullptr);
					curr_time = ctime(&now);
					cout << curr_time << "[System]:进入拥塞避免阶段。" << endl;
				}
			}
			//拥塞避免阶段收到新的ACK，线性增长
			if (sendwindow.state == CONGESTION_AVOID)
			{
				//对应窗口前移
				sendwindow.floor = recvpacket.head.seq + 1;
				sendwindow.size += 1;
				sendwindow.ceiling = sendwindow.floor + sendwindow.size - 1;
				if (sendwindow.ceiling >= packetnum)
				{
					sendwindow.ceiling = packetnum;
				}
				sendwindow.curACKseq = recvpacket.head.seq;
				sendwindow.dupACKcount = 0;
				now = time(nullptr);
				curr_time = ctime(&now);
				cout << curr_time << "[System]:当前为拥塞避免阶段，窗口大小为：" << sendwindow.size << "。" << endl;
			}
			//快速恢复阶段收到新的ACK，进入拥塞避免
			if (sendwindow.state == QUICK_RECOVERY)
			{
				//对应窗口前移
				sendwindow.state = CONGESTION_AVOID;
				sendwindow.floor = recvpacket.head.seq + 1;
				sendwindow.size = sendwindow.ssthresh;
				sendwindow.ceiling = sendwindow.floor + sendwindow.size - 1;
				if (sendwindow.ceiling >= packetnum)
				{
					sendwindow.ceiling = packetnum;
				}
				sendwindow.curACKseq = recvpacket.head.seq;
				sendwindow.dupACKcount = 0;
				now = time(nullptr);
				curr_time = ctime(&now);
				cout << curr_time << "[System]:当前为快速恢复阶段，窗口大小为：" << sendwindow.size << "。" << endl;
				cout << curr_time << "[System]:收到新的ACK，进入拥塞避免阶段。" << endl;
			}
		}
		else
		{
			//收到重复ACK
			if ((res == 0) && (recvpacket.head.seq == sendwindow.curACKseq))
			{
				sendwindow.dupACKcount += 1;
			}
			//一旦收到三个重复ACK就进入快速恢复阶段
			if (sendwindow.dupACKcount == 3)
			{
				sendwindow.state = QUICK_RECOVERY;
				sendwindow.ssthresh = sendwindow.size / 2;
				sendwindow.size = sendwindow.ssthresh + 3;
				sendwindow.ceiling = sendwindow.floor + sendwindow.size - 1;
				if (sendwindow.ceiling >= packetnum)
				{
					sendwindow.ceiling = packetnum;
				}
				now = time(nullptr);
				curr_time = ctime(&now);
				cout << curr_time << "[System]:进入快速恢复阶段，当前窗口大小为：" << sendwindow.size << "。" << endl;
			}
			continue;
		}
	}
}
//传送文件
int sendmessage(int messagelength, char* message, int addrlength, SOCKET& client, SOCKADDR_IN& ServerAddr)
{
	//设置为非阻塞模式，便于在之后进行超时判断
	int iMode = 1; //0：阻塞
	ioctlsocket(client, FIONBIO, (u_long FAR*) & iMode);//非阻塞设置
	//计算要传多少个包
	packetnum = messagelength / 2048;
	if (messagelength % 2048)//向上取整
	{
		packetnum += 1;
	}
	//发送窗口的上限不能大于需要发的数据包总数
	if (packetnum < sendwindow.ceiling)
	{
		sendwindow.ceiling = packetnum;
	}
	now = time(nullptr);
	curr_time = ctime(&now);
	cout << curr_time << "[System]:共有" << messagelength << "字节，因此共要传送" << packetnum << "个包" << endl;
	//创建接受数据包并确认的线程
	DWORD WINAPI recvMsgThread(LPVOID IpParameter);
	//不需要任何句柄，所以直接关闭
	CloseHandle(CreateThread(NULL, 0, recvMsgThread, (LPVOID)&client, 0, 0));
	int sequence = 1;//记录下一个要发送的数据包的序列号
	clock_t starttime;
	UDPpacket sendpack;//发送包
	char* sendbuffer = new char[sizeof(sendpack)];
	int flag;
	starttime = clock();
	//根据sentbutnotchecked和ceiling判断发包个数
	while (sequence <= packetnum)
	{
		//当前文件传输完成
		if (finished == 1)
		{
			finished = 0;
			return 1;
		}
		//窗口移动则导致sendwindow.sentbutnotcheck<sendwindow.ceiling
		else if (sendwindow.sentbutnotchecked < sendwindow.ceiling)
		{
			//将窗口内未发送的数据包发送出去
			for (int i = sendwindow.sentbutnotchecked + 1;i <= sendwindow.ceiling;i++)
			{
				//当前是最后一个包
				if (i == packetnum)
				{
					sendpack.head.flags = FINAL;
					sendpack.head.len = messagelength - 2048 * (i - 1);//数据部分长度
					sendpack.head.seq = sequence;//序列号
					sendpack.head.Check = 0;
					//消息复制 报错——内存太小
					memcpy(&(sendpack.data), message + 2048 * (i - 1), sendpack.head.len);
					//设置校验和
					sendpack.head.Check = packetcheck((u_short*)&sendpack, sizeof(sendpack));
					//进行传输
					memcpy(sendbuffer, &(sendpack), sizeof(sendpack));
					int flag = sendto(client, sendbuffer, sizeof(sendpack), 0, (sockaddr*)&ServerAddr, addrlength);
					if (flag == -1)
					{
						now = time(nullptr);
						curr_time = ctime(&now);
						cout << curr_time << "[System]:";
						cout << "包seq" << sendpack.head.seq << "发送失败" << endl;
					}
					else
					{
						now = time(nullptr);
						curr_time = ctime(&now);
						cout << curr_time << "[System]:";
						cout << "成功发送包seq：" << sendpack.head.seq << "  datalength：" << sendpack.head.len << "  Check：" << sendpack.head.Check << "  flags：" << (u_short)sendpack.head.flags << endl;
					}
					sequence = i;
					sendwindow.sentbutnotchecked = sequence;
				}
				//当前不是最后一个包
				else
				{
					sendpack.head.flags = 0;
					sendpack.head.len = 2048;//数据部分长度
					sendpack.head.seq = i;//序列号
					sendpack.head.Check = 0;
					//消息复制时需要注意是i-1
					memcpy(&(sendpack.data), message + 2048 * (i - 1), sendpack.head.len);
					//设置校验和
					sendpack.head.Check = packetcheck((u_short*)&sendpack, sizeof(sendpack));
					//进行传输
					memcpy(sendbuffer, &(sendpack), sizeof(sendpack));
					sendto(client, sendbuffer, sizeof(sendpack), 0, (sockaddr*)&ServerAddr, addrlength);
					now = time(nullptr);
					curr_time = ctime(&now);
					cout << curr_time << "[System]:";
					cout << "成功发送包seq：" << sendpack.head.seq << "  datalength：" << sendpack.head.len << "  Check：" << sendpack.head.Check << "  flags：" << (u_short)sendpack.head.flags << endl;
					sendwindow.sentbutnotchecked = i;
					sequence = i + 1;
				}
			}
			//更新发送时间
			starttime = clock();
			//sequence = sendwindow.ceiling + 1;
		}
		//如果超时则重传当前窗口内所有包
		else if (clock() - starttime >= 2 * CLOCKS_PER_SEC)
		{
			//超时重传则进入慢启动阶段
			sendwindow.ssthresh = sendwindow.size / 2;
			sendwindow.size = 1;
			sendwindow.ceiling = sendwindow.floor + sendwindow.size - 1;
			if (sendwindow.ceiling >= packetnum)
			{
				sendwindow.ceiling = packetnum;
			}
			sendwindow.state = SLOW_START;
			sendwindow.dupACKcount = 0;
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:超时重传，进入慢启动阶段，当前门限值为：" << sendwindow.ssthresh << "。" << endl;
			for (int i = sendwindow.floor;i <= sendwindow.ceiling;i++)
			{
				//当前是最后一个包
				if (i == packetnum)
				{
					sendpack.head.flags = FINAL;
					sendpack.head.len = messagelength - 2048 * (i - 1);//数据部分长度
					sendpack.head.seq = i;//序列号
					sendpack.head.Check = 0;
					//消息复制 报错——内存太小
					memcpy(&(sendpack.data), message + 2048 * (i - 1), sendpack.head.len);
					//设置校验和
					sendpack.head.Check = packetcheck((u_short*)&sendpack, sizeof(sendpack));
					//进行传输
					memcpy(sendbuffer, &(sendpack), sizeof(sendpack));
					int flag = sendto(client, sendbuffer, sizeof(sendpack), 0, (sockaddr*)&ServerAddr, addrlength);
					if (flag == -1)
					{
						now = time(nullptr);
						curr_time = ctime(&now);
						cout << curr_time << "[System]:";
						cout << "包seq" << sendpack.head.seq << "发送失败" << endl;
					}
					else
					{
						now = time(nullptr);
						curr_time = ctime(&now);
						cout << curr_time << "[System]:";
						cout << "超时重传包seq：" << sendpack.head.seq << "  datalength：" << sendpack.head.len << "  Check：" << sendpack.head.Check << "  flags：" << (u_short)sendpack.head.flags << endl;
					}
				}
				else
				{
					sendpack.head.flags = 0;
					sendpack.head.len = 2048;//数据部分长度
					sendpack.head.seq = i;//序列号
					sendpack.head.Check = 0;
					//消息复制时需要注意是i-1
					memcpy(&(sendpack.data), message + 2048 * (i - 1), sendpack.head.len);
					//设置校验和
					sendpack.head.Check = packetcheck((u_short*)&sendpack, sizeof(sendpack));
					//进行传输
					memcpy(sendbuffer, &(sendpack), sizeof(sendpack));
					flag = sendto(client, sendbuffer, sizeof(sendpack), 0, (sockaddr*)&ServerAddr, addrlength);
					if (flag == -1)
					{
						now = time(nullptr);
						curr_time = ctime(&now);
						cout << curr_time << "[System]:";
						cout << "包seq" << sendpack.head.seq << "发送失败" << endl;
					}
					else
					{
						now = time(nullptr);
						curr_time = ctime(&now);
						cout << curr_time << "[System]:";
						cout << "超时重传包seq：" << sendpack.head.seq << "  datalength：" << sendpack.head.len << "  Check：" << sendpack.head.Check << "  flags：" << (u_short)sendpack.head.flags << endl;
					}
				}
			}
			sendwindow.sentbutnotchecked = sendwindow.ceiling;
			sequence = sendwindow.ceiling + 1;
			//更新发送时间
			starttime = clock();
		}
		else//其他情况下不需要做任何处理
		{
			continue;
		}
	}
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
	//为之后绑定套接字做准备
	SOCKADDR_IN ClientAddr;
	ClientAddr.sin_family = AF_INET;
	ClientAddr.sin_addr.s_addr = inet_addr("127.0.0.1");//IP地址
	ClientAddr.sin_port = htons(8888);//端口号
	//创建socket，指定地址类型为AF_INET，数据报套接字
	SOCKET client = socket(AF_INET, SOCK_DGRAM, 0);
	//进行连接
	int len = sizeof(ClientAddr);
	flag = connect(client, ClientAddr, len);
	if (flag != 0)//判断绑定
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
		closesocket(client);
		WSACleanup();
		return 0;
	}
	//数据传输
	string filename;
	now = time(nullptr);
	curr_time = ctime(&now);
	cout << curr_time << "[System]:";
	cout << "请输入文件名称" << endl;
	cin >> filename;
	//传送文件名
	now = time(nullptr);
	curr_time = ctime(&now);
	cout << curr_time << "[System]:";
	cout << "————开始传输文件名字————" << endl;
	sendmessage(filename.length(), (char*)(filename.c_str()), len, client, ClientAddr);
	now = time(nullptr);
	curr_time = ctime(&now);
	cout << curr_time << "[System]:";
	cout << "————文件名传输成功————" << endl;
	//这里空两个时钟周期是为了避免线程的混淆
	clock_t cur_time = clock();
	while (clock() - cur_time <= 2 * CLOCKS_PER_SEC)
	{

	}
	//恢复窗口
	sendwindow.size = 1;
	sendwindow.ceiling = 1;
	sendwindow.floor = 1;
	sendwindow.sentbutnotchecked = 0;
	sendwindow.size = 1;
	sendwindow.ssthresh = 14;
	finished = 0;
	now = time(nullptr);
	curr_time = ctime(&now);
	cout << curr_time << "[System]:";
	cout << "————开始传输文件内容————" << endl;
	//以二进制方式打开文件
	ifstream fin(filename.c_str(), ifstream::binary);
	char* message = new char[100000000];//会不会太大了
	int index = 0;
	//以字节为单位放入
	unsigned char temp = fin.get();
	while (fin)
	{
		message[index++] = temp;
		temp = fin.get();
	}
	fin.close();
	//传送文件信息
	clock_t start = clock();
	sendmessage(index, message, len, client, ClientAddr);
	clock_t end = clock();
	now = time(nullptr);
	curr_time = ctime(&now);
	cout << curr_time << "[System]:"<< "文件大小为：" << index << "bytes，总共传输时间为：" << ((double)(end - start)) / CLK_TCK << "s"<< "，平均吞吐率为：" << index * 8.0 * CLK_TCK / (end - start) << "bps" << endl;
	//断开连接
	clock_t starttime = clock();
	while (clock() - starttime <= 2 * CLOCKS_PER_SEC)
	{

	}
	flag = disconnect(client, ClientAddr, len);
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
	//关闭套接字
	closesocket(client);
	WSACleanup();

	return 0;
}