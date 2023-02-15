#include<iostream>
#include<WinSock2.h>
#include<fstream>
#include<string>
#include<time.h>
#include<Windows.h>
#pragma comment(lib,"ws2_32.lib")//socket��
using namespace std;
const unsigned char SYN = 0x01;//SYN=1 ACK=0
const unsigned char ACK = 0x02;//SYN=0 ACK=1
const unsigned char SYN_ACK = 0x03;//SYN=1 ACK=1
const unsigned char FIN = 0x04;//FYN=1
const unsigned char FIN_ACK = 0x05;//FYN=1 ACK=1
const unsigned char FINAL_ACK = 0x06;//�ɹ����յ����һ����
const unsigned char FINAL = 0x07;//��ʶΪ���һ����
//������RENO�㷨�漰������״̬
const unsigned char SLOW_START = 0x08;//������
const unsigned char CONGESTION_AVOID = 0x09;//ӵ������
const unsigned char QUICK_RECOVERY = 0x10;//���ٻָ�
struct sendwindowstruct//���ʹ���
{
	unsigned int size;//���ڴ�С
	unsigned int ceiling;//����
	unsigned int floor;//���ޣ��ѷ��͵����seq
	unsigned int sentbutnotchecked;//��¼�ѷ��͵�δȷ�ϵ����seq
	unsigned int ssthresh;//����ʼ����
	unsigned int curACKseq;
	unsigned int dupACKcount;//�ظ�ACK�ĸ���
	unsigned char state;//��ǰ״̬
	sendwindowstruct()
	{
		size = 1;//���ڴ�С��ʼ��Ϊ1
		floor = 1;//��1��ʼ���
		ceiling = 1;//����
		sentbutnotchecked = 0;//��ʼʱû�а���ȷ��
		ssthresh = 14;
		state = SLOW_START;//��ʼ״̬Ϊ������״̬
	}
};
struct UDPpackethead
{
	unsigned int seq;//���к� 16λ
	u_short Check;//У�� 16λ
	u_short len;//���ݲ����ܳ���
	unsigned char flags;//��־λ 
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
	char data[2048];//ÿ��data��������
};
//ȫ�ֱ���
time_t now;
char* curr_time = ctime(&now);
sendwindowstruct sendwindow;
int packetnum;
bool finished = 0;//��Ϣ�������
//����У��� ��ppt
u_short packetcheck(u_short* packet, int packelength)
{
	//UDP����͵ļ��㷽���ǣ�
	//��ÿ16λ��͵ó�һ��32λ������
	//������32λ��������16λ��Ϊ0�����16λ�ӵ�16λ�ٵõ�һ��32λ������
	//�ظ���2��ֱ����16λΪ0������16λȡ�����õ�У��͡�
	//register�ؼ�����������������ܵĽ���������CPU�ڲ��Ĵ�����
	//������ͨ���ڴ�Ѱַ���������Ч�ʡ�
	//u_long32λ������ʹ��unsigned int ֻ��16λ
	register u_long sum = 0;
	int count = (packelength + 1) / 2;//�����ֽڵļ���
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
//���ֽ������ӽ׶�
//�ͻ��˷��������������ݱ�SYN������һ������
//��������ͻ��˷���һ��SYN_ACK�����ڶ�������
//�ͻ��˷���ACK��������������
int connect(SOCKET& client, SOCKADDR_IN& ClientAddr, int& len)
{
	//����Ϊ������ģʽ�����⿨��recvfrom
	int iMode = 1; //0������
	ioctlsocket(client, FIONBIO, (u_long FAR*) & iMode);//����������
	//��һ�����֣��ͻ��˷���SYN
	UDPpacket pack1;
	pack1.head.flags = SYN;
	//pack1.head.Check = packetcheck((u_short*)&(pack1.head), sizeof(pack1.head));
	pack1.head.Check = packetcheck((u_short*)&(pack1), sizeof(pack1));
	//cout << "�ͻ���headΪ��check" << pack1.head.Check << "len:" << pack1.head.len << "flags:" << (u_short)pack1.head.flags << "seq:" << pack1.head.seq << endl;
	//���ͻ�����
	char* buffer1 = new char[sizeof(pack1)];
	memcpy(buffer1, &pack1, sizeof(pack1));
	//s���ѽ������ӵ�socket,�������UDPЭ�����辭��connect������
	//msg�����������ݵĻ�����
	//len������������
	//flags�����÷�ʽ��־λ, һ��Ϊ0, �ı�flags������ı�sendto���͵���ʽ��
	//addr������ѡ��ָ�룬ָ��Ŀ���׽��ֵĵ�ַ
	//tolen��addr��ָ��ַ�ĳ���
	int flag = sendto(client, buffer1, sizeof(pack1), 0, (sockaddr*)&ClientAddr, len);
	//��һ�����ַ���ʱ��
	clock_t pack1starttime = clock();
	if (flag == -1)
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "��������������һ�����ַ���ʧ�ܡ�������" << endl;
	}
	else
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "��������������һ�����ַ��ͳɹ���������" << endl;
	}
	//�ڶ������֣��ȴ�������������SYN_ACK
	UDPpacket pack2;
	//������
	char* buffer2 = new char[sizeof(pack2)];
	//һֱû���յ�����˵��֮ǰ�İ��������ˣ�Ҫ���·��͵�һ������
	while (1)
	{
		//�����ʱ���ش�
		if (clock() - pack1starttime >= 2 * CLOCKS_PER_SEC)
		{
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			cout << "����������һ�����ְ���ʱ�ش���������" << endl;
			sendto(client, buffer1, sizeof(pack1), 0, (sockaddr*)&ClientAddr, len);
			pack1starttime = clock();
		}
		flag = recvfrom(client, buffer2, sizeof(pack2), 0, (sockaddr*)&ClientAddr, &len);
		if (flag <= 0)//û�յ����Ļ��ͼ���recvfrom
		{
			continue;
		}
		//�յ��˰�
		memcpy(&pack2, buffer2, sizeof(pack2));
		int res = packetcheck((u_short*)&pack2, sizeof(pack2));
		//�ǵڶ�������
		if (pack2.head.flags == SYN_ACK && res == 0)
		{
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			cout << "�������������ڶ������ֽ��ճɹ���������" << endl;
			break;
		}
		//����һֱ���ڵȴ�״̬
	}
	//���������� �ͻ��˷���ACK
	UDPpacket pack3;
	pack3.head.flags = ACK;
	pack3.head.Check = packetcheck((u_short*)&pack3, sizeof(pack3));
	//���ͻ�����
	char* buffer3 = new char[sizeof(pack3)];
	memcpy(buffer3, &pack3, sizeof(pack3));
	flag = sendto(client, buffer3, sizeof(pack3), 0, (sockaddr*)&ClientAddr, len);
	if (flag == -1)
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "���������������������ַ���ʧ�ܡ�������" << endl;
		return 0;
	}
	else
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "���������������������ַ��ͳɹ���������" << endl;
	}
	clock_t checktime = clock();//��¼���͵���������ʱ��
	while ((clock() - checktime) <= 2 * CLOCKS_PER_SEC)
	{
		//�ְѵڶ�������������
		//�������Ӧ�þͽ�������
		if (recvfrom(client, buffer2, sizeof(pack2), 0, (sockaddr*)&ClientAddr, &len) <= 0)
		{
			continue;
		}
		//����յ��˰�����ô��˵��������������û�յ����ش�
		sendto(client, buffer3, sizeof(pack3), 0, (sockaddr*)&ClientAddr, len);
		checktime = clock();
	}
	iMode = 0; //0������
	ioctlsocket(client, FIONBIO, (u_long FAR*) & iMode);//�ָ�������ģʽ
	//cout << "�ͻ���headΪ��check" << pack3.head.Check << "len:" << pack3.head.len << "flags:" << (u_short)pack3.head.flags << "seq:" << pack3.head.seq << endl;
	return 1;
}
//����
//�ͻ��˷���FIN
//����������FIN_ACK
//����������FIN
//�ͻ��˷���FIN_ACK
int disconnect(SOCKET& client, SOCKADDR_IN& ClientAddr, int len)
{
	//����Ϊ������ģʽ�����⿨��recvfrom
	int iMode = 1; //0������
	ioctlsocket(client, FIONBIO, (u_long FAR*) & iMode);//����������
	//��һ�λ��֣��ͻ��˷���FIN
	UDPpacket pack1;
	pack1.head.flags = FIN;
	pack1.head.Check = packetcheck((u_short*)&pack1, sizeof(pack1));
	//���ͻ�����
	char* buffer1 = new char[sizeof(pack1)];
	memcpy(buffer1, &pack1, sizeof(pack1));
	int flag = sendto(client, buffer1, sizeof(pack1), 0, (sockaddr*)&ClientAddr, len);
	if (flag == -1)
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "��������������һ�λ��ַ���ʧ�ܡ�������" << endl;
	}
	else
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "��������������һ�λ��ַ��ͳɹ���������" << endl;
	}
	//��¼��һ�λ��ַ���ʱ��
	clock_t starttime = clock();
	//�ڶ��λ��֣����շ�����������FIN_ACK
	UDPpacket pack2;
	//������
	char* buffer2 = new char[sizeof(pack2)];
	//���յڶ��λ���������Ϣ
	while (1)
	{
		if (clock() - starttime >= 2 * CLOCKS_PER_SEC)
		{
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			//��ʱ�ˣ��ش�
			cout << "��ʱ�ط���һ�λ���" << endl;
			sendto(client, buffer1, sizeof(pack1), 0, (sockaddr*)&ClientAddr, len);
			starttime = clock();
		}
		flag = recvfrom(client, buffer2, sizeof(pack2), 0, (sockaddr*)&ClientAddr, &len);
		if (flag <= 0)
		{
			continue;
		}
		//���ճɹ�
		memcpy(&(pack2), buffer2, sizeof(pack2));
		u_short res = packetcheck((u_short*)&pack2, sizeof(pack2));
		//�ǵڶ��λ���
		if (pack2.head.flags == FIN_ACK && res == 0)
		{
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			cout << "�������������ڶ��λ��ֽ��ճɹ���������" << endl;
			break;
		}
		//�����İ�����ȷ��һֱ�ȴ�
	}
	starttime = clock();
	//�ȴ�����ʱ�����ڣ���serverͬ��
	while (clock() - starttime <= CLOCKS_PER_SEC)
	{

	}
	//���յ����λ�����Ϣ��ͷ��ΪACK
	UDPpacket pack3;
	//������
	char* buffer3 = new char[sizeof(pack3)];
	//���յ����λ���������Ϣ
	while (1)
	{
		flag = recvfrom(client, buffer3, sizeof(pack3), 0, (sockaddr*)&ClientAddr, &len);
		if (flag <= 0)
		{
			continue;
		}
		//���ճɹ�
		memcpy(&(pack3), buffer3, sizeof(pack3));
		u_short res = packetcheck((u_short*)&pack3, sizeof(pack3));
		//�ǵ����λ���
		if (pack3.head.flags == FIN && res == 0)
		{
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			cout << "�����������������λ��ֽ��ճɹ���������" << endl;
			break;
		}
		//����һֱ���ڵȴ�״̬
	}
	//���Ĵλ��֣��ͻ��˷���FIN_ACK
	UDPpacket pack4;
	pack4.head.flags = FIN_ACK;
	pack4.head.Check = packetcheck((u_short*)&pack4, sizeof(pack4));
	//���ͻ�����
	char* buffer4 = new char[sizeof(pack4)];
	memcpy(buffer4, &pack4, sizeof(pack4));
	flag = sendto(client, buffer4, sizeof(pack4), 0, (sockaddr*)&ClientAddr, len);
	if (flag == -1)
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "���������������Ĵλ��ַ���ʧ�ܡ�������" << endl;
	}
	else
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "���������������Ĵλ��ַ��ͳɹ���������" << endl;
	}
	starttime = clock();//���Ĵλ��ַ���ʱ��
	while (clock() - starttime <= 2 * CLOCKS_PER_SEC)
	{
		//û�յ�
		if (recvfrom(client, buffer3, sizeof(pack1), 0, (sockaddr*)&ClientAddr, &len) <= 0)
		{
			continue;
		}
		else//�յ��ˣ�˵�����Ĵλ���û����ȥ����Ҫ���·���
		{
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			cout << "��ʱ�ط����Ĵλ���" << endl;
			sendto(client, buffer4, sizeof(pack4), 0, (sockaddr*)&ClientAddr, len);
			//���·���ʱ��
			starttime = clock();
		}
	}
	return 1;
}
//������Ϣ���߳�
DWORD WINAPI recvMsgThread(LPVOID IpParameter)
{
	SOCKET client = *(SOCKET*)IpParameter;//��ȡ�ͻ��˵�SOCKET����
	SOCKADDR_IN ClientAddr = *(SOCKADDR_IN*)IpParameter;//��ȡclientaddr
	//����Ϊ������ģʽ��������֮����г�ʱ�ж�
	int iMode = 1; //0������
	ioctlsocket(client, FIONBIO, (u_long FAR*) & iMode);//����������
	UDPpacket recvpacket;//���Է��������ص����ݰ�
	//���ջ�����
	char* recvbuffer = new char[sizeof(recvpacket)];
	int addrlength = sizeof(ClientAddr);
	while (1)
	{
		while (recvfrom(client, recvbuffer, sizeof(recvpacket), 0, (sockaddr*)&ClientAddr, &addrlength) <= 0)
		{

		}
		//�յ�������Ҫ�жϰ�����ȷ��
		memcpy(&recvpacket, recvbuffer, sizeof(recvpacket));
		//cout << "�ͻ����յ���ACK��headΪ��check" << recvpacket.head.Check << "  len:" << recvpacket.head.len << "  flags:" << (u_short)recvpacket.head.flags << "  seq:" << recvpacket.head.seq << endl;
		//����У���
		u_short res = packetcheck((u_short*)&recvpacket, sizeof(recvpacket));
		//�ۻ�ȷ��˼�룺�յ��µ�ACK��ʱ���ж�����������
		//1.У������ȷ
		//2.�յ���ACK�����Ӧ��>=��ǰ���ڵײ�
		if (res == 0 && recvpacket.head.flags == FINAL_ACK)
		{
			finished = 1;//ȫ���������
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			//�ļ�ȫ����ȷ���ͣ�����
			cout << "�ļ��Ѿ�ȫ����ȷ�������" << endl;
			//�ȴ�2MSL����ֹ�ͻ��ְ�����
			clock_t starttime = clock();
			while (clock() - starttime <= 2 * CLOCKS_PER_SEC)
			{

			}
			return 1;//�ر��߳�
		}
		else if (res == 0 && recvpacket.head.seq >= sendwindow.floor)
		{
			//�������׶��յ��µ�ACK��cwnd����
			if (sendwindow.state == SLOW_START)
			{
				//��Ӧ����ǰ��
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
				cout << curr_time << "[System]:��ǰΪ�������׶Σ����ڴ�СΪ��" << sendwindow.size << "��" << endl;
				if (sendwindow.size >= sendwindow.ssthresh)
				{
					sendwindow.state = CONGESTION_AVOID;
					now = time(nullptr);
					curr_time = ctime(&now);
					cout << curr_time << "[System]:����ӵ������׶Ρ�" << endl;
				}
			}
			//ӵ������׶��յ��µ�ACK����������
			if (sendwindow.state == CONGESTION_AVOID)
			{
				//��Ӧ����ǰ��
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
				cout << curr_time << "[System]:��ǰΪӵ������׶Σ����ڴ�СΪ��" << sendwindow.size << "��" << endl;
			}
			//���ٻָ��׶��յ��µ�ACK������ӵ������
			if (sendwindow.state == QUICK_RECOVERY)
			{
				//��Ӧ����ǰ��
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
				cout << curr_time << "[System]:��ǰΪ���ٻָ��׶Σ����ڴ�СΪ��" << sendwindow.size << "��" << endl;
				cout << curr_time << "[System]:�յ��µ�ACK������ӵ������׶Ρ�" << endl;
			}
		}
		else
		{
			//�յ��ظ�ACK
			if ((res == 0) && (recvpacket.head.seq == sendwindow.curACKseq))
			{
				sendwindow.dupACKcount += 1;
			}
			//һ���յ������ظ�ACK�ͽ�����ٻָ��׶�
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
				cout << curr_time << "[System]:������ٻָ��׶Σ���ǰ���ڴ�СΪ��" << sendwindow.size << "��" << endl;
			}
			continue;
		}
	}
}
//�����ļ�
int sendmessage(int messagelength, char* message, int addrlength, SOCKET& client, SOCKADDR_IN& ServerAddr)
{
	//����Ϊ������ģʽ��������֮����г�ʱ�ж�
	int iMode = 1; //0������
	ioctlsocket(client, FIONBIO, (u_long FAR*) & iMode);//����������
	//����Ҫ�����ٸ���
	packetnum = messagelength / 2048;
	if (messagelength % 2048)//����ȡ��
	{
		packetnum += 1;
	}
	//���ʹ��ڵ����޲��ܴ�����Ҫ�������ݰ�����
	if (packetnum < sendwindow.ceiling)
	{
		sendwindow.ceiling = packetnum;
	}
	now = time(nullptr);
	curr_time = ctime(&now);
	cout << curr_time << "[System]:����" << messagelength << "�ֽڣ���˹�Ҫ����" << packetnum << "����" << endl;
	//�����������ݰ���ȷ�ϵ��߳�
	DWORD WINAPI recvMsgThread(LPVOID IpParameter);
	//����Ҫ�κξ��������ֱ�ӹر�
	CloseHandle(CreateThread(NULL, 0, recvMsgThread, (LPVOID)&client, 0, 0));
	int sequence = 1;//��¼��һ��Ҫ���͵����ݰ������к�
	clock_t starttime;
	UDPpacket sendpack;//���Ͱ�
	char* sendbuffer = new char[sizeof(sendpack)];
	int flag;
	starttime = clock();
	//����sentbutnotchecked��ceiling�жϷ�������
	while (sequence <= packetnum)
	{
		//��ǰ�ļ��������
		if (finished == 1)
		{
			finished = 0;
			return 1;
		}
		//�����ƶ�����sendwindow.sentbutnotcheck<sendwindow.ceiling
		else if (sendwindow.sentbutnotchecked < sendwindow.ceiling)
		{
			//��������δ���͵����ݰ����ͳ�ȥ
			for (int i = sendwindow.sentbutnotchecked + 1;i <= sendwindow.ceiling;i++)
			{
				//��ǰ�����һ����
				if (i == packetnum)
				{
					sendpack.head.flags = FINAL;
					sendpack.head.len = messagelength - 2048 * (i - 1);//���ݲ��ֳ���
					sendpack.head.seq = sequence;//���к�
					sendpack.head.Check = 0;
					//��Ϣ���� �������ڴ�̫С
					memcpy(&(sendpack.data), message + 2048 * (i - 1), sendpack.head.len);
					//����У���
					sendpack.head.Check = packetcheck((u_short*)&sendpack, sizeof(sendpack));
					//���д���
					memcpy(sendbuffer, &(sendpack), sizeof(sendpack));
					int flag = sendto(client, sendbuffer, sizeof(sendpack), 0, (sockaddr*)&ServerAddr, addrlength);
					if (flag == -1)
					{
						now = time(nullptr);
						curr_time = ctime(&now);
						cout << curr_time << "[System]:";
						cout << "��seq" << sendpack.head.seq << "����ʧ��" << endl;
					}
					else
					{
						now = time(nullptr);
						curr_time = ctime(&now);
						cout << curr_time << "[System]:";
						cout << "�ɹ����Ͱ�seq��" << sendpack.head.seq << "  datalength��" << sendpack.head.len << "  Check��" << sendpack.head.Check << "  flags��" << (u_short)sendpack.head.flags << endl;
					}
					sequence = i;
					sendwindow.sentbutnotchecked = sequence;
				}
				//��ǰ�������һ����
				else
				{
					sendpack.head.flags = 0;
					sendpack.head.len = 2048;//���ݲ��ֳ���
					sendpack.head.seq = i;//���к�
					sendpack.head.Check = 0;
					//��Ϣ����ʱ��Ҫע����i-1
					memcpy(&(sendpack.data), message + 2048 * (i - 1), sendpack.head.len);
					//����У���
					sendpack.head.Check = packetcheck((u_short*)&sendpack, sizeof(sendpack));
					//���д���
					memcpy(sendbuffer, &(sendpack), sizeof(sendpack));
					sendto(client, sendbuffer, sizeof(sendpack), 0, (sockaddr*)&ServerAddr, addrlength);
					now = time(nullptr);
					curr_time = ctime(&now);
					cout << curr_time << "[System]:";
					cout << "�ɹ����Ͱ�seq��" << sendpack.head.seq << "  datalength��" << sendpack.head.len << "  Check��" << sendpack.head.Check << "  flags��" << (u_short)sendpack.head.flags << endl;
					sendwindow.sentbutnotchecked = i;
					sequence = i + 1;
				}
			}
			//���·���ʱ��
			starttime = clock();
			//sequence = sendwindow.ceiling + 1;
		}
		//�����ʱ���ش���ǰ���������а�
		else if (clock() - starttime >= 2 * CLOCKS_PER_SEC)
		{
			//��ʱ�ش�������������׶�
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
			cout << curr_time << "[System]:��ʱ�ش��������������׶Σ���ǰ����ֵΪ��" << sendwindow.ssthresh << "��" << endl;
			for (int i = sendwindow.floor;i <= sendwindow.ceiling;i++)
			{
				//��ǰ�����һ����
				if (i == packetnum)
				{
					sendpack.head.flags = FINAL;
					sendpack.head.len = messagelength - 2048 * (i - 1);//���ݲ��ֳ���
					sendpack.head.seq = i;//���к�
					sendpack.head.Check = 0;
					//��Ϣ���� �������ڴ�̫С
					memcpy(&(sendpack.data), message + 2048 * (i - 1), sendpack.head.len);
					//����У���
					sendpack.head.Check = packetcheck((u_short*)&sendpack, sizeof(sendpack));
					//���д���
					memcpy(sendbuffer, &(sendpack), sizeof(sendpack));
					int flag = sendto(client, sendbuffer, sizeof(sendpack), 0, (sockaddr*)&ServerAddr, addrlength);
					if (flag == -1)
					{
						now = time(nullptr);
						curr_time = ctime(&now);
						cout << curr_time << "[System]:";
						cout << "��seq" << sendpack.head.seq << "����ʧ��" << endl;
					}
					else
					{
						now = time(nullptr);
						curr_time = ctime(&now);
						cout << curr_time << "[System]:";
						cout << "��ʱ�ش���seq��" << sendpack.head.seq << "  datalength��" << sendpack.head.len << "  Check��" << sendpack.head.Check << "  flags��" << (u_short)sendpack.head.flags << endl;
					}
				}
				else
				{
					sendpack.head.flags = 0;
					sendpack.head.len = 2048;//���ݲ��ֳ���
					sendpack.head.seq = i;//���к�
					sendpack.head.Check = 0;
					//��Ϣ����ʱ��Ҫע����i-1
					memcpy(&(sendpack.data), message + 2048 * (i - 1), sendpack.head.len);
					//����У���
					sendpack.head.Check = packetcheck((u_short*)&sendpack, sizeof(sendpack));
					//���д���
					memcpy(sendbuffer, &(sendpack), sizeof(sendpack));
					flag = sendto(client, sendbuffer, sizeof(sendpack), 0, (sockaddr*)&ServerAddr, addrlength);
					if (flag == -1)
					{
						now = time(nullptr);
						curr_time = ctime(&now);
						cout << curr_time << "[System]:";
						cout << "��seq" << sendpack.head.seq << "����ʧ��" << endl;
					}
					else
					{
						now = time(nullptr);
						curr_time = ctime(&now);
						cout << curr_time << "[System]:";
						cout << "��ʱ�ش���seq��" << sendpack.head.seq << "  datalength��" << sendpack.head.len << "  Check��" << sendpack.head.Check << "  flags��" << (u_short)sendpack.head.flags << endl;
					}
				}
			}
			sendwindow.sentbutnotchecked = sendwindow.ceiling;
			sequence = sendwindow.ceiling + 1;
			//���·���ʱ��
			starttime = clock();
		}
		else//��������²���Ҫ���κδ���
		{
			continue;
		}
	}
}
int main()
{
	WSADATA wsaData;
	//wVersionRequested��Windows Sockets API�ṩ�ĵ��÷���ʹ�õ���߰汾�š�
	//��λ�ֽ�ָ�����汾(����)�ţ���λ�ֽ�ָ�����汾�š�
	WORD mVersionRequested = MAKEWORD(2, 2);
	//�����׽��ֿ� flag��ʾ�Ƿ�ɹ�
	int flag = WSAStartup(mVersionRequested, &wsaData);
	if (flag == 0)
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "�׽��ֿ���سɹ�" << endl;
	}
	else
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "�׽��ֿ����ʧ��" << endl;
	}
	//Ϊ֮����׽�����׼��
	SOCKADDR_IN ClientAddr;
	ClientAddr.sin_family = AF_INET;
	ClientAddr.sin_addr.s_addr = inet_addr("127.0.0.1");//IP��ַ
	ClientAddr.sin_port = htons(8888);//�˿ں�
	//����socket��ָ����ַ����ΪAF_INET�����ݱ��׽���
	SOCKET client = socket(AF_INET, SOCK_DGRAM, 0);
	//��������
	int len = sizeof(ClientAddr);
	flag = connect(client, ClientAddr, len);
	if (flag != 0)//�жϰ�
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "�������ӳɹ�" << endl;
	}
	else
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "��������ʧ��" << endl;
		closesocket(client);
		WSACleanup();
		return 0;
	}
	//���ݴ���
	string filename;
	now = time(nullptr);
	curr_time = ctime(&now);
	cout << curr_time << "[System]:";
	cout << "�������ļ�����" << endl;
	cin >> filename;
	//�����ļ���
	now = time(nullptr);
	curr_time = ctime(&now);
	cout << curr_time << "[System]:";
	cout << "����������ʼ�����ļ����֡�������" << endl;
	sendmessage(filename.length(), (char*)(filename.c_str()), len, client, ClientAddr);
	now = time(nullptr);
	curr_time = ctime(&now);
	cout << curr_time << "[System]:";
	cout << "���������ļ�������ɹ���������" << endl;
	//���������ʱ��������Ϊ�˱����̵߳Ļ���
	clock_t cur_time = clock();
	while (clock() - cur_time <= 2 * CLOCKS_PER_SEC)
	{

	}
	//�ָ�����
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
	cout << "����������ʼ�����ļ����ݡ�������" << endl;
	//�Զ����Ʒ�ʽ���ļ�
	ifstream fin(filename.c_str(), ifstream::binary);
	char* message = new char[100000000];//�᲻��̫����
	int index = 0;
	//���ֽ�Ϊ��λ����
	unsigned char temp = fin.get();
	while (fin)
	{
		message[index++] = temp;
		temp = fin.get();
	}
	fin.close();
	//�����ļ���Ϣ
	clock_t start = clock();
	sendmessage(index, message, len, client, ClientAddr);
	clock_t end = clock();
	now = time(nullptr);
	curr_time = ctime(&now);
	cout << curr_time << "[System]:"<< "�ļ���СΪ��" << index << "bytes���ܹ�����ʱ��Ϊ��" << ((double)(end - start)) / CLK_TCK << "s"<< "��ƽ��������Ϊ��" << index * 8.0 * CLK_TCK / (end - start) << "bps" << endl;
	//�Ͽ�����
	clock_t starttime = clock();
	while (clock() - starttime <= 2 * CLOCKS_PER_SEC)
	{

	}
	flag = disconnect(client, ClientAddr, len);
	if (flag != 0)//�жϰ�
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "�Ͽ����ӳɹ�" << endl;
	}
	else
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "�Ͽ�����ʧ��" << endl;
	}
	//�ر��׽���
	closesocket(client);
	WSACleanup();

	return 0;
}