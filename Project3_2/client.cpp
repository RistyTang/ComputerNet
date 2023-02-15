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
struct sendwindowstruct//���ʹ���
{
	const unsigned int size = 20;//���ڴ�С=20
	unsigned int ceiling;//����
	unsigned int floor;//���ޣ��ѷ��͵����seq
	//unsigned int base;//�ѷ��ʹ�ȷ�ϵ�����ķ���
	sendwindowstruct()
	{
		ceiling = 20;//��1�ſ�ʼ���
		floor = 1;//��1��ʼ���
	}
};
//ȫ�ֱ���window
sendwindowstruct sendwindow;
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
//�����ļ�
int sendmessage(int messagelength, char* message, int addrlength, SOCKET& client, SOCKADDR_IN& ServerAddr)
{
	//����Ϊ������ģʽ��������֮����г�ʱ�ж�
	int iMode = 1; //0������
	ioctlsocket(client, FIONBIO, (u_long FAR*) & iMode);//����������
	//����Ҫ�����ٸ���
	int packetnum = messagelength / 2048;
	if (messagelength % 2048)//����ȡ��
	{
		packetnum += 1;
	}
	now = time(nullptr);
	curr_time = ctime(&now);
	cout << curr_time << "[System]:����" << messagelength << "�ֽڣ���˹�Ҫ����" << packetnum << "����" << endl;
	int sequence = 1;//��¼��һ��Ҫ���͵����ݰ������к�
	clock_t starttime;
	UDPpacket sendpack;//���Ͱ�
	char* sendbuffer = new char[sizeof(sendpack)];

	//1.���͵�ǰ���������еİ������һ�����ݰ����⣩
	//�ڷ��͵�һ�����ݰ���ʼ��ʱ
	int flag;
	while (sequence <= sendwindow.ceiling && sequence != packetnum)
	{
		sendpack.head.flags = 0;//flag=0��ʾ�������һ�����ݰ�
		sendpack.head.len = 2048;//���ݲ��ֳ���
		sendpack.head.seq = sequence;//���к�
		sendpack.head.Check = 0;
		//��Ϣ����
		memcpy(&(sendpack.data), message + 2048 * (sequence - 1), sendpack.head.len);
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
			cout << "�ɹ����Ͱ�seq��" << sendpack.head.seq << "  datalength��" << sendpack.head.len << "  Check��" << sendpack.head.Check << "  flags��" << (u_short)sendpack.head.flags << endl;
		}
		if (sequence == sendwindow.floor)
		{
			//���͵�ǰ�����ڵ�һ����ʱ��ʼ��ʱ
			starttime = clock();
		}
		sequence += 1;
	}
	//2.�ն��ٷ����٣�ͬʱ����ǰ�ƣ����һ�������⣩
	while (sequence < packetnum)
	{
		UDPpacket recvpacket;//���Է��������ص����ݰ�
		//���ջ�����
		char* recvbuffer = new char[sizeof(recvpacket)];
		//�յ�ACK��Ϊ�ź�
		while (recvfrom(client, recvbuffer, sizeof(recvpacket), 0, (sockaddr*)&ServerAddr, &addrlength) <= 0)
		{
			if (clock() - starttime >= 2 * CLOCKS_PER_SEC)
			{
				starttime = clock();//��������ʱ��
				//�ش����д����ڵİ�
				for (int i = sendwindow.floor;i <= sendwindow.ceiling;i++)
				{
					sendpack.head.flags = 0;
					sendpack.head.len = 2048;//���ݲ��ֳ���
					sendpack.head.seq = i;//���к�
					sendpack.head.Check = 0;
					//��Ϣ����ʱ��Ҫע����i-1������
					memcpy(&(sendpack.data), message + 2048 * (i - 1), sendpack.head.len);
					//����У���
					sendpack.head.Check = packetcheck((u_short*)&sendpack, sizeof(sendpack));
					//���д���
					memcpy(sendbuffer, &(sendpack), sizeof(sendpack));
					sendto(client, sendbuffer, sizeof(sendpack), 0, (sockaddr*)&ServerAddr, addrlength);
					now = time(nullptr);
					curr_time = ctime(&now);
					cout << curr_time << "[System]:";
					cout << "��seq" << sendpack.head.seq << "��ʱ���·��ͳɹ�" << endl;
				}
			}
		}
		//�յ�������Ҫ�жϰ�����ȷ��
		memcpy(&recvpacket, recvbuffer, sizeof(recvpacket));
		//cout << "�ͻ����յ���ACK��headΪ��check" << recvpacket.head.Check << "  len:" << recvpacket.head.len << "  flags:" << (u_short)recvpacket.head.flags << "  seq:" << recvpacket.head.seq << endl;
		//����У���
		u_short res = packetcheck((u_short*)&recvpacket, sizeof(recvpacket));
		//����յ���ACK�����>=��ǰ���ڵײ��Ļ�
		if (res == 0 && recvpacket.head.seq >= sendwindow.floor)
		{
			//��������ǰ��
			sendwindow.floor = recvpacket.head.seq + 1;
			sendwindow.ceiling = sendwindow.floor + sendwindow.size - 1;
			if (sendwindow.ceiling >= packetnum)
			{
				sendwindow.ceiling = packetnum;
			}
			//���Լ�������
			for (int i = sequence;i <= sendwindow.ceiling;i++)
			{
				sendpack.head.flags = 0;
				sendpack.head.len = 2048;//���ݲ��ֳ���
				sendpack.head.seq = sequence;//���к�
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
				sequence += 1;
			}
			//���·���ʱ��
			starttime = clock();
		}
		else//��������²���Ҫ���κδ���
		{
			continue;
		}
	}
	//3.�������һ����
	if (1)
	{
		sendpack.head.flags = FINAL;
		sendpack.head.len = messagelength - 2048 * (sequence - 1);//���ݲ��ֳ���
		sendpack.head.seq = sequence;//���к�
		sendpack.head.Check = 0;
		//��Ϣ���� �������ڴ�̫С
		memcpy(&(sendpack.data), message + 2048 * (sequence - 1), sendpack.head.len);
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
		//��ʱ����Ҫ�յ����Է�������FINAL_ACK����ʾ��������
		starttime = clock();//����ʱ��
	}
	//4.�������һ��յ�����size��С��ACK��
	while (1)
	{
		UDPpacket recvpacket;//���Է��������ص����ݰ�
		//���ջ�����
		char* recvbuffer = new char[sizeof(recvpacket)];
		while (recvfrom(client, recvbuffer, sizeof(recvpacket), 0, (sockaddr*)&ServerAddr, &addrlength) <= 0)
		{
			if (clock() - starttime >= 2 * CLOCKS_PER_SEC)
			{
				starttime = clock();//��������ʱ��
				//�ش����д����ڵİ�
				for (int i = sendwindow.floor;i <= sendwindow.ceiling;i++)
				{
					sendpack.head.flags = 0;
					sendpack.head.len = 2048;//���ݲ��ֳ���
					sendpack.head.seq = i;//���к�
					sendpack.head.Check = 0;
					//��Ϣ����ʱ��Ҫע����i-1������
					memcpy(&(sendpack.data), message + 2048 * (i - 1), sendpack.head.len);
					//����У���
					sendpack.head.Check = packetcheck((u_short*)&sendpack, sizeof(sendpack));
					//���д���
					memcpy(sendbuffer, &(sendpack), sizeof(sendpack));
					sendto(client, sendbuffer, sizeof(sendpack), 0, (sockaddr*)&ServerAddr, addrlength);
					now = time(nullptr);
					curr_time = ctime(&now);
					cout << curr_time << "[System]:";
					cout << "��seq" << sendpack.head.seq << "��ʱ���·��ͳɹ�" << endl;
				}
			}
		}
		//�յ��˰����ж�һ���������seq
		memcpy(&(recvpacket), recvbuffer, sizeof(recvpacket));
		u_short res = packetcheck((u_short*)&recvpacket, sizeof(recvpacket));
		//�������һ������ACK�������ǰ��򵽴��ACK
		//cout << "�ͻ����յ��İ�headΪ��check" << recvpacket.head.Check << "len:" << recvpacket.head.len << "flags:" << (u_short)recvpacket.head.flags << "seq:" << recvpacket.head.seq << endl;
		//cout << "checkres=" << res << endl;
		if (res == 0 && recvpacket.head.seq >= sendwindow.floor && recvpacket.head.flags != FINAL_ACK)
		{
			//ֻ��Ҫ�����ڵײ�ǰ��
			sendwindow.floor = recvpacket.head.seq + 1;
			starttime = clock();
		}
		//�����һ������ACK��ֱ�ӽ���
		else if (res == 0 && recvpacket.head.flags == FINAL_ACK)
		{
			now = time(nullptr);
			curr_time = ctime(&now);
			cout << curr_time << "[System]:";
			//�ļ�ȫ����ȷ���ͣ�����
			cout << "�ļ��Ѿ�ȫ����ȷ�������" << endl;
			//�ȴ�2MSL����ֹ�ͻ��ְ�����
			while (clock() - starttime <= 2 * CLOCKS_PER_SEC)
			{

			}
			return 1;
		}
		else//����ɶҲ���ɣ�ֱ����ʱ�ط���
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
	cout << curr_time << "[System]:";
	cout << "�ļ���СΪ��" << index << "bytes���ܹ�����ʱ��Ϊ��" << ((double)(end - start)) / CLK_TCK << "s";
	cout << "��ƽ��������Ϊ��" << index * 8.0 * CLK_TCK / (end - start) << "bps" << endl;
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
	closesocket(client);
	WSACleanup();

	return 0;
}