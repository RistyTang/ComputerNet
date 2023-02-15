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
time_t now;
char* curr_time = ctime(&now);

struct UDPpackethead
{
	unsigned int seq;//���к� 32λ
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
//���ֽ������ӽ׶�
//�ͻ��˷��������������ݱ�SYN������һ������
//��������ͻ��˷���һ��SYN_ACK�����ڶ�������
//�ͻ��˷���ACK��������������
int connect(SOCKET& server, SOCKADDR_IN& ServerAddr, int& len)
{
	//����Ϊ������ģʽ�����⿨��recvfrom
	int iMode = 1; //0������
	ioctlsocket(server, FIONBIO, (u_long FAR*) & iMode);//����������
	UDPpacket pack1;
	//������
	char* buffer1 = new char[sizeof(pack1)];
	//���յ�һ������������Ϣ
	now = time(nullptr);
	curr_time = ctime(&now);
	cout << curr_time << "[System]:";
	cout << "�������ӽ׶�" << endl;
	int flag;
	while (1)
	{
		//û�յ�����һֱѭ��
		flag = recvfrom(server, buffer1, sizeof(pack1), 0, (sockaddr*)&ServerAddr, &len);
		if (flag <= 0)
		{
			continue;
		}
		//���ճɹ�
		memcpy(&(pack1), buffer1, sizeof(pack1.head));
		//cout << "�������յ�headΪ��check" << pack1.head.Check << "len:" << pack1.head.len << "flags:" << (u_short)pack1.head.flags << "seq:" << pack1.head.seq << endl;
		u_short res = packetcheck((u_short*)&pack1, sizeof(pack1));
		//ֻ�е��յ������ݰ���ȷʱ�����ǳɹ�
		if (pack1.head.flags == SYN && res == 0)
		{
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			cout << "��������������һ�����ֽ��ճɹ���������" << endl;
			break;
		}
		//��������İ�����ȷ�Ļ����������ѭ�����ȴ��ͻ������·���
	}
	//�ڶ������֣�����������SYN_ACK
	UDPpacket pack2;
	pack2.head.flags = SYN_ACK;
	pack2.head.Check = packetcheck((u_short*)&pack2, sizeof(pack2));
	//���ͻ�����
	char* buffer2 = new char[sizeof(pack2)];
	memcpy(buffer2, &pack2, sizeof(pack2));
	flag = sendto(server, buffer2, sizeof(pack2), 0, (sockaddr*)&ServerAddr, len);
	//��ʱ�ش�����
	clock_t starttime = clock();//�ڶ������ַ���ʱ��
	if (flag == -1)//�ڶ������ַ���ʧ�ܣ��ش�
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "�������������ڶ������ַ���ʧ�ܡ�������" << endl;
	}
	else
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "�������������ڶ������ַ��ͳɹ���������" << endl;
	}
	//���������� ����������
	UDPpacket pack3;
	//������
	char* buffer3 = new char[sizeof(pack3)];
	//���һֱû���յ����Ļ���˵����һ������Ҫ�ش�
	while (1)
	{
		//��ʱ�ش�
		if (clock_t() - starttime >= CLOCKS_PER_SEC)
		{
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			cout << "�������������ڶ������ְ������ش���������" << endl;
			sendto(server, buffer2, sizeof(pack2), 0, (sockaddr*)&ServerAddr, len);
			starttime = clock();//���·���ʱ��
		}
		//û�յ�����һֱѭ��
		flag = recvfrom(server, buffer3, sizeof(pack3), 0, (sockaddr*)&ServerAddr, &len);
		if (flag <= 0)
		{
			continue;
		}
		//�����ΰ����ճɹ�
		memcpy(&(pack3), buffer3, sizeof(pack3));
		//cout << "�������յ�headΪ��check" << pack3.head.Check << "len:" << pack3.head.len << "flags:" << (u_short)pack3.head.flags << "seq:" << pack3.head.seq << endl;
		u_short res = packetcheck((u_short*)&pack3, sizeof(pack3));
		if (pack3.head.flags == ACK && res == 0)
		{
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			cout << "���������������������ֽ��ճɹ���������" << endl;
			break;//����ѭ��
		}
		//����Ļ���һֱ�ȵ��ܽ�����ȷ�İ�
	}
	//������ʱ������
	starttime = clock();
	while ((clock() - starttime) <= 2 * CLOCKS_PER_SEC)
	{
	}
	iMode = 0; //0������
	ioctlsocket(server, FIONBIO, (u_long FAR*) & iMode);//�ָ�������ģʽ
	return 1;
}
//����
//�ͻ��˷���FIN
//����������FIN_ACK
//����������FIN
//�ͻ��˷���FIN_ACK
int disconnect(SOCKET& server, SOCKADDR_IN& ServerAddr, int& len)
{
	//����Ϊ������ģʽ�����⿨��recvfrom
	int iMode = 1; //0������
	ioctlsocket(server, FIONBIO, (u_long FAR*) & iMode);//����������
	UDPpacket pack1;
	//������
	char* buffer1 = new char[sizeof(pack1)];
	int flag;
	//���յ�һ�λ���������Ϣ
	while (1)
	{
		flag = recvfrom(server, buffer1, sizeof(pack1), 0, (sockaddr*)&ServerAddr, &len);
		//û���յ���Ϣ��һֱѭ��
		if (flag <= 0)
		{
			continue;
		}
		//���ճɹ�
		memcpy(&(pack1), buffer1, sizeof(pack1));
		u_short res = packetcheck((u_short*)&pack1, sizeof(pack1));
		//�ǵ�һ������
		if (pack1.head.flags == FIN && res == 0)
		{
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			cout << "��������������һ�λ��ֽ��ճɹ���������" << endl;
			break;
		}
		//�����һֱ�ȴ�������ȷ�İ�
	}
	//�ڶ��λ��֣�����������FIN_ACK
	UDPpacket pack2;
	pack2.head.flags = FIN_ACK;
	pack2.head.Check = packetcheck((u_short*)&pack2, sizeof(pack2));
	//���ͻ�����
	char* buffer2 = new char[sizeof(pack2)];
	memcpy(buffer2, &pack2, sizeof(pack2));
	flag = sendto(server, buffer2, sizeof(pack2), 0, (sockaddr*)&ServerAddr, len);
	if (flag == -1)
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "�������������ڶ��λ��ַ���ʧ�ܡ�������" << endl;
	}
	else
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "�������������ڶ��λ��ַ��ͳɹ���������" << endl;
	}
	clock_t starttime = clock();//��¼�ڶ��λ��ַ���ʱ��
	while (clock() - starttime <= 2 * CLOCKS_PER_SEC)
	{
		//û�յ�
		flag = recvfrom(server, buffer1, sizeof(pack1), 0, (sockaddr*)&ServerAddr, &len);
		if (flag <= 0)
		{
			continue;
		}
		else//�յ��ˣ�˵���ڶ��λ���û����ȥ����Ҫ���·���
		{
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			cout << "��ʱ�ط��ڶ��λ���" << endl;
			sendto(server, buffer2, sizeof(pack2), 0, (sockaddr*)&ServerAddr, len);
			//���·���ʱ��
			starttime = clock();
		}
	}

	//�����λ��֣�������Ҫ����FIN
	UDPpacket pack3;
	pack3.head.flags = FIN;
	pack3.head.Check = packetcheck((u_short*)&pack3, sizeof(pack3));
	//���ͻ�����
	char* buffer3 = new char[sizeof(pack3)];
	memcpy(buffer3, &pack3, sizeof(pack3));
	flag = sendto(server, buffer3, sizeof(pack3), 0, (sockaddr*)&ServerAddr, len);
	if (flag == -1)
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "�����������������λ��ַ���ʧ�ܡ�������" << endl;
	}
	else
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "�����������������λ��ַ��ͳɹ���������" << endl;
	}
	starttime = clock();//�����λ��ַ���ʱ��
	//���յ��Ĵλ���������Ϣ
	UDPpacket pack4;
	//������
	char* buffer4 = new char[sizeof(pack4)];
	while (1)
	{
		if (clock() - starttime >= 2 * CLOCKS_PER_SEC)
		{
			starttime = clock();
			//���·��͵����λ���
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			cout << "��ʱ�ط������λ���" << endl;
			sendto(server, buffer3, sizeof(pack3), 0, (sockaddr*)&ServerAddr, len);
		}
		flag = recvfrom(server, buffer4, sizeof(pack4), 0, (sockaddr*)&ServerAddr, &len);
		if (flag <= 0)
		{
			continue;
		}
		//���ճɹ�
		memcpy(&(pack4), buffer4, sizeof(pack4));
		u_short res = packetcheck((u_short*)&pack4, sizeof(pack4));
		//�ǵ��Ĵλ���
		if (pack4.head.flags == FIN_ACK && res == 0)
		{
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			cout << "���������������Ĵλ��ֽ��ճɹ���������" << endl;
			break;
		}
		//����Ļ�һֱѭ���ȴ�
	}
	return 1;
}

//������Ϣ
u_long ReceiveMessage(char* message, int& addrlength, SOCKET& server, SOCKADDR_IN& ClientAddr)
{
	unsigned int filelength = 0;//��¼�ļ�����
	UDPpacket recvpacket, sendpacket;
	//���ջ�����
	char* recvbuffer = new char[sizeof(recvpacket)];
	//���ͻ�����
	char* sendbuffer = new char[sizeof(sendpacket)];
	unsigned int Ssequence = 0;//��ǰ������յ������ŷ���
	while (1)
	{
		//���ձ��ĳ���
		int recvlength = recvfrom(server, recvbuffer, sizeof(recvpacket), 0, (sockaddr*)&ClientAddr, &addrlength);
		//cout << "recvlength=" << recvlength << endl;
		memcpy(&recvpacket, recvbuffer, sizeof(recvpacket));
		u_short res = packetcheck((u_short*)&recvpacket, sizeof(recvpacket));
		//u_short res = packetcheck((u_short*)&recvpacket.head, sizeof(recvpacket.head));
		//cout << "�������յ�headΪ��check" << recvpacket.head.Check << "len:" << recvpacket.head.len << "flags:" << (u_short)recvpacket.head.flags << "seq:" << recvpacket.head.seq << endl;
		//cout << "У����Ϊ��" << res << endl;
		if (res != 0)//������
		{
			cout << "��Ϣ����У�������" << endl;
			sendpacket.head.flags = ACK;
			//��ʱ���ص�sequence��ȻΪ��һ�δ��͵�ֵ
			//��ʾ�ͻ���֮ǰ�����İ�����
			sendpacket.head.Check = 0;
			sendpacket.head.seq = Ssequence;
			//����У���
			sendpacket.head.Check = packetcheck((u_short*)&sendpacket, sizeof(sendpacket));
			memcpy(sendbuffer, &sendpacket, sizeof(sendpacket));
			//�ڴ��ش�
			int flag = sendto(server, sendbuffer, sizeof(sendpacket), 0, (sockaddr*)&ClientAddr, addrlength);
			if (flag == -1)
			{
				cout << "����ʧ��" << endl;
			}
			continue;
		}
		else//����У�����ǶԵ�
		{
			//�ж�һ�����ǲ������һ����
			if ((recvpacket.head.flags == FINAL) && (Ssequence == recvpacket.head.seq - 1))
			{
				//�Ž���Ϣ����
				memcpy(message + filelength, recvpacket.data, recvpacket.head.len);
				filelength += recvpacket.head.len;
				now = time(nullptr);
				curr_time = ctime(&now);
				cout << curr_time << "[System]:";
				cout << "�����������һ����seq:" << recvpacket.head.seq << "�Ѿ�������ϡ�������" << endl;
				//Ssequence += 1;
				Ssequence = recvpacket.head.seq;
				sendpacket.head.flags = FINAL_ACK;
				sendpacket.head.Check = 0;
				sendpacket.head.seq = Ssequence;
				sendpacket.head.Check = packetcheck((u_short*)&sendpacket, sizeof(sendpacket));
				//cout << "���������͵�ȷ�ϰ�headΪ��check" << sendpacket.head.Check << "len:" << sendpacket.head.len << "flags:" << (u_short)sendpacket.head.flags << "seq:" << sendpacket.head.seq << endl;
				memcpy(sendbuffer, &sendpacket, sizeof(sendpacket));
				int flag = sendto(server, sendbuffer, sizeof(sendpacket), 0, (sockaddr*)&ClientAddr, addrlength);
				clock_t starttime = clock();//���һ��ACK�ķ���ʱ��
				now = time(nullptr);
				curr_time = ctime(&now);
				cout << curr_time << "[System]:";
				cout << "�������һ��ȷ�ϰ�seq:" << sendpacket.head.seq << "  datalength:" << sendpacket.head.len << "  Check:" << sendpacket.head.Check << "  flags:" << (u_short)sendpacket.head.flags << endl;
				//Ϊ�˷�ֹ���һ������ACK���Ͷ�ʧ����Ҫ��2MSL
				//����Ϊ������ģʽ�����⿨��recvfrom
				int iMode = 1; //0������
				ioctlsocket(server, FIONBIO, (u_long FAR*) & iMode);//����������
				while (clock() - starttime <= 2 * CLOCKS_PER_SEC)
				{
					if (recvfrom(server, recvbuffer, sizeof(recvpacket), 0, (sockaddr*)&ClientAddr, &addrlength) <= 0)
					{
						//һֱû���յ�����˵��������ȷ������ACK
						continue;
					}
					//������Ҫ�ش����һ������ACK
					else
					{
						now = time(nullptr);
						curr_time = ctime(&now);
						cout << curr_time << "[System]:";
						cout << "�ش����һ��ȷ�ϰ�seq:" << sendpacket.head.seq << "  datalength:" << sendpacket.head.len << "  Check:" << sendpacket.head.Check << "  flags:" << (u_short)sendpacket.head.flags << endl;
						sendto(server, sendbuffer, sizeof(sendpacket), 0, (sockaddr*)&ClientAddr, addrlength);
						starttime = clock();
					}
				}
				//�ָ�����ģʽ
				iMode = 0; //0������
				ioctlsocket(server, FIONBIO, (u_long FAR*) & iMode);//����������
				return filelength;
			}
			else//���滹�б�İ�
			{
				//����յ��İ�������Ҫ�İ��Ļ�
				if (Ssequence != recvpacket.head.seq - 1)
				{
					//˵���������⣬����ACK
					sendpacket.head.flags = ACK;
					sendpacket.head.len = 0;
					sendpacket.head.Check = 0;
					//��ʱ��ȻӦ�÷���֮ǰ��seq
					sendpacket.head.seq = Ssequence;
					//����У���
					sendpacket.head.Check = packetcheck((u_short*)&sendpacket, sizeof(sendpacket));
					memcpy(sendbuffer, &sendpacket, sizeof(sendpacket));
					//�ط��ð���ACK
					int flag = sendto(server, sendbuffer, sizeof(sendpacket), 0, (sockaddr*)&ClientAddr, addrlength);
					if (flag == -1)
					{
						//int i=WSAGetLastError();
						//cout << "wrong code:" << i << endl;
						//�������10038
						now = time(nullptr);
						curr_time = ctime(&now);
						cout << curr_time << "[System]:";
						//�ط����ݰ�
						cout << "���·���ȷ�ϰ�seq:" << sendpacket.head.seq << "  datalength:" << sendpacket.head.len << "  Check:" << sendpacket.head.Check << "  flags:" << (u_short)sendpacket.head.flags << "ʧ��" << endl;
						continue;//���������ݰ�
					}
					else
					{
						now = time(nullptr);
						curr_time = ctime(&now);
						cout << curr_time << "[System]:";
						cout << "���·���ȷ�ϰ�seq:" << sendpacket.head.seq << "  datalength:" << sendpacket.head.len << "  Check:" << sendpacket.head.Check << "  flags:" << (u_short)sendpacket.head.flags << "�ɹ�" << endl;
						continue;
					}

				}
				else//�յ������ݰ��Ǵ�ʱ��������Ҫ��
				{
					//�Ž���Ϣ����
					memcpy(message + filelength, recvpacket.data, recvpacket.head.len);
					filelength += recvpacket.head.len;
					//����������ACK
					Ssequence = recvpacket.head.seq;
					sendpacket.head.flags = ACK;
					sendpacket.head.seq = Ssequence;
					sendpacket.head.Check = 0;
					sendpacket.head.Check = packetcheck((u_short*)&sendpacket, sizeof(sendpacket));
					memcpy(sendbuffer, &sendpacket, sizeof(sendpacket));
					int flag = sendto(server, sendbuffer, sizeof(sendpacket), 0, (SOCKADDR*)&ClientAddr, addrlength);
					//Ϊʲô����������ʧ�ܣ�����buffer̫С�ˡ���
					if (flag == -1)
					{
						now = time(nullptr);
						curr_time = ctime(&now);
						cout << curr_time << "[System]:";
						//cout << "��ȷ���հ���" << recvpacket.head.seq << endl;
						cout << "������������ACK����ʧ�ܡ�������" << endl;
					}
					else
					{
						now = time(nullptr);
						curr_time = ctime(&now);
						cout << curr_time << "[System]:";
						//cout << "��ȷ���հ���" << recvpacket.head.seq << endl;
						cout << "����ȷ�ϰ�seq:" << sendpacket.head.seq << "  datalength:" << sendpacket.head.len << "  Check:" << sendpacket.head.Check << "  flags:" << (u_short)sendpacket.head.flags << endl;
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
	//����socket��ָ����ַ����ΪAF_INET�����ݱ��׽���
	SOCKET server = socket(AF_INET, SOCK_DGRAM, 0);
	//Ϊ֮����׽�����׼��
	SOCKADDR_IN ServerAddr;
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");//IP��ַ
	ServerAddr.sin_port = htons(7777);//�˿ں�
	// ���׽��ֵ�һ��IP��ַ��һ���˿���
	flag = bind(server, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr));
	if (flag == 0)//�жϰ�
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "�󶨳ɹ�" << endl;
	}
	else
	{
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "[System]:";
		cout << "��ʧ��" << endl;
		closesocket(server);
		WSACleanup();
		return 0;
	}
	//�������ֽ�������
	int len = sizeof(ServerAddr);
	flag = connect(server, ServerAddr, len);
	if (flag == 1)//�жϰ�
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
		closesocket(server);
		WSACleanup();
		return 0;
	}
	int addrlen = sizeof(ServerAddr);
	//���ݽ���
	int namelength = 0;
	int datalength = 0;
	char recvname[100];//�ļ�������
	char recvdata[100000000];//�ļ���������
	now = time(nullptr);
	curr_time = ctime(&now);
	cout << curr_time << "[System]:";
	cout << "����������ʼ�����ļ�����������" << endl;
	namelength = ReceiveMessage(recvname, addrlen, server, ServerAddr);
	now = time(nullptr);
	curr_time = ctime(&now);
	cout << curr_time << "[System]:";
	cout << "���������ļ����ƽ��ճɹ���������" << endl;
	now = time(nullptr);
	curr_time = ctime(&now);
	cout << curr_time << "[System]:";
	cout << "����������ʼ�����ļ����ݡ�������" << endl;
	datalength = ReceiveMessage(recvdata, addrlen, server, ServerAddr);
	now = time(nullptr);
	curr_time = ctime(&now);
	cout << curr_time << "[System]:";
	cout << "���������ļ����ݽ��ճɹ���������" << endl;
	recvname[namelength] = '\0';
	recvdata[datalength] = '\0';
	string filename(recvname);
	now = time(nullptr);
	curr_time = ctime(&now);
	cout << curr_time << "[System]:";
	cout << "�ļ���Ϊ��\"" << filename << "\"" << endl;
	//�洢���յ�������
	ofstream out(filename.c_str(), ofstream::binary);
	for (int i = 0; i < datalength; i++) {
		out << recvdata[i];
	}
	out.close();
	now = time(nullptr);
	curr_time = ctime(&now);
	cout << curr_time << "[System]:";
	cout << "�ļ�\"" << filename << "\"" << "�Ѿ��յ���д��" << endl;
	//����
	flag = disconnect(server, ServerAddr, addrlen);
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
	closesocket(server);
	WSACleanup();
	return 0;
}