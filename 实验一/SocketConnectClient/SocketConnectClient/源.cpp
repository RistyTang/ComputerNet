#include<iostream>
#include<winsock2.h>
#include<cstring>
#include<ctime>
#pragma comment(lib,"ws2_32.lib")//socket��
using namespace std;
int main()
{
	WSADATA wsaData;
	//wVersionRequested��Windows Sockets API�ṩ�ĵ��÷���ʹ�õ���߰汾�š�
	//��λ�ֽ�ָ�����汾(����)�ţ���λ�ֽ�ָ�����汾�š�
	WORD mVersionRequested = MAKEWORD(2, 2);
	//�����׽��ֿ� flag��ʾ�Ƿ�ɹ�
	int flag=WSAStartup(mVersionRequested, &wsaData);
	//��ȡ��ǰʱ��
	time_t now = time(nullptr);
	char* curr_time = ctime(&now);
	if (flag == 0)
	{
		cout <<curr_time<< "�׽��ֿ���سɹ�" << endl;
	}
	else
	{
		cout <<curr_time<< "�׽��ֿ����ʧ��" << endl;
	}
	//����socket��ָ����ַ����ΪAF_INET����ʽ�׽��֣�TCPЭ��
	SOCKET client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//Ϊ֮�����Ӻʹ���������׼��
	SOCKADDR_IN ClientAddr;
	ClientAddr.sin_family = AF_INET;
	ClientAddr.sin_addr.s_addr = inet_addr("127.0.0.1");//IP��ַ
	ClientAddr.sin_port = htons(6666);//�˿ں�
	//��������
	flag=connect(client, (SOCKADDR*)&ClientAddr, sizeof(SOCKADDR));
	now = time(nullptr);
	curr_time = ctime(&now);
	if (flag == 0)
	{
		cout << curr_time << "�������ӳɹ�" << endl;
	}
	else
	{
		cout << curr_time << "��������ʧ��" << endl;
	}
	//����������Ϣ���߳�
	DWORD WINAPI recvMsgThread(LPVOID IpParameter);
	//����Ҫ�κξ��������ֱ�ӹر�
	CloseHandle(CreateThread(NULL, 0, recvMsgThread, (LPVOID)&client, 0, 0));

	//������Ϣ
	char clienttext[500] =  {};
	cout << "�����������������������쿪ʼ����������������" << endl;
	while (1)
	{
		//cin >> clienttext;
		cin.getline(clienttext, 499);
		if (!(strcmp(clienttext, "quit()")))//�˳�������Ϣ
		{
			cout << "���Ѿ��˳�����" << endl;
			break;
		}
		send(client, clienttext, sizeof(clienttext), 0);
		now = time(nullptr);
		curr_time = ctime(&now);
		cout << curr_time << "��Ϣ\""<<clienttext<<"\"���ͳɹ�" << endl;
		memset(clienttext, 0, sizeof(clienttext));
	}
	//�ر��׽���\�׽��ֿ�
	//send(client, "�������\n", sizeof("�������\n"), 0);
	closesocket(client);
	WSACleanup();
	return 0;
}
DWORD WINAPI recvMsgThread(LPVOID IpParameter)//������Ϣ���߳�
{
	SOCKET client = *(SOCKET*)IpParameter;//��ȡ�ͻ��˵�SOCKET����
	char servertext[1000];//���յ�����Ϣ
	string printtext;
	while (1)
	{
		//��ȡ��ǰʱ��
		time_t now = time(0);
		char* curr_time = ctime(&now);
		int recvlength=recv(client, servertext, sizeof(servertext), 0);//sizeof(servertext)
		printtext = servertext;
		if (recvlength<0)
		{
			cout << "�����Ѿ�������" << endl;
			break;
		}
		if (recvlength == 0||printtext=="")//û�յ���Ϣ��ʲôҲ����
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