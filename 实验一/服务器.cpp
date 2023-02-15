#include<iostream>
#include<WinSock2.h>
#include<cstring>
#include<ctime>
#include<vector>
#include<string>
#pragma comment(lib,"ws2_32.lib")//socket��
using namespace std;
DWORD WINAPI servEventThread(LPVOID& IpParameter);//������������Ϣ�߳�
DWORD WINAPI ClientThread(LPVOID IpParameter);//�������˶�ͻ����̺߳�������
int curr_clients;//��ǰһ���ж��ٸ��ͻ���
SOCKET clientsarray[10];//һ��ʮ���ͻ�����
string sendtext;//������Ҫ�ַ�����Ϣ
int from_client;//�յ������ĸ��ͻ��˵���Ϣ
int main()
{
	WSADATA wsaData;
	//wVersionRequested��Windows Sockets API�ṩ�ĵ��÷���ʹ�õ���߰汾�š�
	//��λ�ֽ�ָ�����汾(����)�ţ���λ�ֽ�ָ�����汾�š�
	WORD mVersionRequested = MAKEWORD(2, 2);
	//�����׽��ֿ� flag��ʾ�Ƿ�ɹ�
	int flag = WSAStartup(mVersionRequested, &wsaData);
	time_t now = time(nullptr);
	char* curr_time = ctime(&now);
	if (flag == 0)
	{
		cout << curr_time << "�׽��ֿ���سɹ�" << endl;
	}
	else
	{
		cout << curr_time << "�׽��ֿ����ʧ��" << endl;
	}
	//����socket��ָ����ַ����ΪAF_INET����ʽ�׽��֣�TCPЭ��
	SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//Ϊ֮����׽�����׼��
	SOCKADDR_IN ServerAddr;
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");//IP��ַ
	ServerAddr.sin_port = htons(6666);//�˿ں�
	// ���׽��ֵ�һ��IP��ַ��һ���˿���
	flag = bind(server, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR));
	now = time(nullptr);
	curr_time = ctime(&now);
	if (flag == 0)//�жϰ�
	{
		cout << curr_time << "�󶨳ɹ�" << endl;
	}
	else
	{
		cout << curr_time << "��ʧ��" << endl;
	}

	//���׽�������Ϊ����ģʽ�ȴ���������
	flag = listen(server, 10);//ͬһ���������ʮ������

	now = time(nullptr);
	curr_time = ctime(&now);
	if (flag == 0)
	{
		cout << curr_time << "�����ɹ�" << endl;
	}
	else
	{
		cout << curr_time << "����ʧ��" << endl;
	}

	//�������󣬽����������󣬷���һ���µĶ�Ӧ�ڴ˴����ӵ��׽���
	int len = sizeof(SOCKADDR);

	curr_clients = 0;
	//���̸߳�����տͻ�������
	while (1)
	{
		SOCKET accepter = accept(server, (SOCKADDR*)&ServerAddr, &len);
		//�����߳����������ͻ���
		if (curr_clients<10)//���ܶ��ڼ�����Ŀ����
		{
			clientsarray[curr_clients] = accepter;//��¼��ͬ�ͻ���
			HANDLE hThread = CreateThread(NULL, 0, ClientThread, (LPVOID)&accepter, 0, 0);
			CloseHandle(hThread);//�رն��̵߳�����
			
		}
		//accepter = INVALID_SOCKET;//����һ���ر�һ��
	}
	//����Ҫ�������ֱ�ӹر�
	//CloseHandle(CreateThread(NULL, 0, servEventThread, (LPVOID)&accepter, 0, 0));


	//�ر��׽��֣��رռ��ص��׽��ֿ�(closesocket()/WSACleanup())

	closesocket(server);
	WSACleanup();

}
DWORD WINAPI servEventThread(LPVOID IpParameter)
{
	SOCKET accepter = *(SOCKET*)IpParameter;//��ȡ�������˵�SOCKET����
	time_t now = time(nullptr);
	char* curr_time = ctime(&now);
	char recvtext[500];
	//cout << "����˺���" << endl;
	for (int i = 0;i < curr_clients;i++)//�ж��������ĸ��ͻ���
	{
		if (accepter == clientsarray[i])
			from_client = i;
	}
	while (1)
	{
		int recvlength = recv(accepter, recvtext, 500, 0);
		for (int i = 0;i < curr_clients;i++)//�ж��������ĸ��ͻ���
		{
			if (accepter == clientsarray[i])
				from_client = i;
		}
		sendtext = curr_time;//��װʱ��
		sendtext.append("�ͻ���");
		sendtext.append(to_string(from_client + 1));
		sendtext.append("������Ϣ:\"");
		string temp=recvtext;
		sendtext.append(temp);
		sendtext.append("\"");//����Ϣ��װ���Ϊʱ��+�ͻ���i������Ϣ��--��
		if (recvlength <= 0)
		{
			cout << "�ͻ���"<<from_client+1<<"�Ѿ��˳����졣" << endl;
			curr_clients -= 1;
			cout << "��ǰ�ͻ�������Ϊ��" << curr_clients << endl;
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
			cout << "�յ����Կͻ���"<<from_client+1<<"����Ϣ��\"";
			cout << recvtext;
			cout << "\"" << endl;
			//�����������յ�����Ϣת���������û�
			string  message;//��������������ʾ����Ϣ
			for (int i = 0;i < curr_clients/*&&i!=from_client*/;i++)
			{
				//cout << "now from client" << from_client << endl;
				if (i != from_client) 
				{
					message = curr_time;
					message.append("���ͻ���");
					message.append(to_string(i + 1));
					message.append("�ŷ�����Ϣ\"");
					message.append(sendtext);
					message.append("\"�ɹ�");
					//strcpy(message, now_send_client.c_str());//
					send(clientsarray[i], sendtext.c_str(), 1000, 0);
					now = time(nullptr);
					curr_time = ctime(&now);
					cout << message << endl;
					//cout << sendtext << endl;
					message = "���ͻ���";//�൱���������
				}
			}
			sendtext = "";//�����Ϣ
			//cout << "sendtext=="<<sendtext << endl;
			//break;
			//memset(recvtext, 0, sizeof(recvtext));
		}
		
		//memset(recvtext, 0, sizeof(recvtext));
		temp = "";//�����Ϣ
	}
	return 0;
}
DWORD WINAPI ClientThread(LPVOID IpParameter)//����������ͻ��˽�����
{
	SOCKET accepter = *(SOCKET*)IpParameter;//��ȡ�������˵�SOCKET����
	//�ٿ����߳�������
	HANDLE hThread = CreateThread(NULL, 0, servEventThread, (LPVOID)&accepter, 0, 0);
	CloseHandle(hThread);
	//�÷��ص��׽��ֺͿͻ��˽���ͨ��(send()/recv())��
	cout << "�����������������������쿪ʼ����������������" << endl;
	curr_clients += 1;
	cout << "��ǰ�ͻ�������Ϊ��" << curr_clients << endl;
	
	//��ȡ��ǰʱ��
	//cout << "������" << endl;
	//string  message;//��������������ʾ����Ϣ
	//�磺���ͻ���x���͡�--����Ϣ�ɹ�
	/*
	while (1)
	{
		//cin>>sendtext;
		//��ȡ��ǰʱ��
		time_t now = time(nullptr);
		string curr_time = ctime(&now);
		if (!(strcmp(sendtext.c_str(), "quit()")))//�˳�������Ϣ
		{
			cout << "���Ѿ�ѡ��������졣" << endl;
			return 0;
		}
		else if (sendtext == "")
		{
			//cout << "������Ϣ����Ϊ�գ�" << endl;
			sendtext = "now no message";
			//break;
		}
		else if(sendtext!="now no message")
		{
			for (int i = 0;i < curr_clients&&i!=from_client;i++)
			{
				message = curr_time;
				message.append( "���ͻ���");
				message.append(to_string(i+1));
				message.append("������Ϣ\"");
				message.append(sendtext);
				message.append("\"�ɹ�");
				//strcpy(message, now_send_client.c_str());//
				send(clientsarray[i], sendtext.c_str(), sizeof(sendtext), 0);
				now = time(nullptr);
				curr_time = ctime(&now);
				cout << message<< endl;
				message = "���ͻ���";//�൱���������
			}
			sendtext = "now no message";//�����Ϣ
			break;
		}
	}*/
	return 0;
}

