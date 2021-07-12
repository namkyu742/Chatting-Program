#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <process.h> 
#include <time.h>

#define BUF_SIZE 100
#define NAME_SIZE 20
#define MAX_CLNT 30

unsigned WINAPI SendMsg(void* arg);
unsigned WINAPI RecvMsg(void* arg);
void ErrorHandling(char* msg);
void print_cls(void);

char userName[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];
char blackList[MAX_CLNT][NAME_SIZE];

int blaCnt = 0;

int main(void)
{
	WSADATA wsaData;
	SOCKET hSock;
	SOCKADDR_IN servAdr;
	HANDLE hSndThread, hRcvThread;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	printf("[Login]\n");
	printf(" �г��� : ");
	char tempname[20];
	scanf("%s", &tempname);
	sprintf(userName, "[%s]", tempname);
	hSock = socket(PF_INET, SOCK_STREAM, 0);

	//----------------------------------
	char addr[20];
	int portnum = 5000;
	printf(" IP�ּ� : ");
	scanf("%s", &addr);
	//	strcpy(addr, "192.168.0.18");
		//----------------------------------

	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = inet_addr(addr);
	servAdr.sin_port = htons(portnum);

	if (connect(hSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
		ErrorHandling("connect() error");

	send(hSock, userName, NAME_SIZE, 0);

	hSndThread = (HANDLE)_beginthreadex(NULL, 0, SendMsg, (void*)&hSock, 0, NULL);
	hRcvThread = (HANDLE)_beginthreadex(NULL, 0, RecvMsg, (void*)&hSock, 0, NULL);

	WaitForSingleObject(hSndThread, INFINITE);
	WaitForSingleObject(hRcvThread, INFINITE);
	closesocket(hSock);
	WSACleanup();

	return 0;
}

unsigned WINAPI SendMsg(void* arg)   // send thread main
{
	print_cls();					// ȭ�� ����
	SOCKET hSock = *((SOCKET*)arg);
	char nameMsg[NAME_SIZE + BUF_SIZE];
	char action[BUF_SIZE];
	char del[] = " ";		// strtok ������
	char* token;			// strtok ��� ����

	while (1) {
		fgets(msg, BUF_SIZE, stdin);	// �޽����� �Է� �޴´�.

		strcpy(action, msg);
		token = strtok(action, del);	// ��ɾ �����Ѵ�.

		if (!strcmp(action, "q\n") || !strcmp(action, "Q\n")) {
			send(hSock, msg, BUF_SIZE, 0);		// ����޽��� ���ۿ�
			closesocket(hSock);
			exit(0);
		}
		else if (!strcmp(action, "/list\n") || !strcmp(action, "/l\n") || !strcmp(action, "/to") || !strcmp(action, "/h\n")) {
			send(hSock, msg, BUF_SIZE, 0);	// ������ �ش� ��ɾ� ���� ��û
		}
		else if (!strcmp(action, "/b")) {		/* ���� ���� */
			token = strtok(NULL, del);
			token[strlen(token) - 1] = '\0';
			strcpy(blackList[blaCnt], token);		// ���� ��Ͽ� �г��� ���
			printf("[Blocked Completed] : %s\n", blackList[blaCnt]);
			blaCnt += 1;							// ���� ��� �ε��� ����
		}
		else if (!strcmp(action, "/br")) {		/* ���� ���� ���� */
			int find = 0;
			token = strtok(NULL, del);
			token[strlen(token) - 1] = '\0';

			for (int i = 0; i < blaCnt; i++) {	// ���ܸ�Ͽ� ������ �ִ��� �˻�
				if (!strcmp(token, blackList[i])) {	// �ִٸ� ��Ͽ��� �����ϰ� ����
					printf("[Unblocked Completed] : %s\n", blackList[i]);
					while (i < blaCnt) {
						strcpy(blackList[i], blackList[i + 1]);
						find = 1;
						i++;
					}
				}
			}
			if (find)
				blaCnt -= 1;						 // ���� ��� �ε��� ����
			else
				printf("Can not found\n");
		}
		else if (!strcmp(action, "/blist\n") || !strcmp(action, "/bl\n")) { /* ���� ���� ��� */
			printf("�������������������� BLACK LIST��������������������\n");
			printf("��������������������������������������������������������������\n");
			printf("�� %3s �� %21s ��\n", "No", "User Name");
			printf("��������������������������������������������������������������\n");
			if (blaCnt == 0)
				printf("��     ��        No Data        ��\n");
			else
				for (int i = 0; i < blaCnt; i++) {
					printf("�� %3d �� %21s ��\n", i + 1, blackList[i]);	// �г��� ���
				}
			printf("��������������������������������������������������������������\n");
		}
		else if (!strcmp(action, "/cls\n")) {		/* ȭ�� ���� */
			print_cls();
		}
		else if (!strcmp(action, "\n")) {
			/* �Է� ���� ���¿��� ���͸� ������ ����� ������ ���� */
			// �ƹ� ���� ������ Send ���� �ʴ´�.
		}
		else {							/* ������ �޽��� ���� */
			time_t t = time(NULL);				// ���� �ð�
			struct tm tm = *localtime(&t);		// �ð� ����� ���� �ڵ�
			char tm_hour[3];
			char tm_min[3];
			// 10 �̸��̸� ���ڸ� ���� ��µǱ� ������ �տ� 0�� ���δ�.
			if (tm.tm_hour < 10)	sprintf(tm_hour, "0%d", tm.tm_hour);
			else					sprintf(tm_hour, "%d", tm.tm_hour);
			if (tm.tm_min < 10)		sprintf(tm_min, "0%d", tm.tm_min);
			else					sprintf(tm_min, "%d", tm.tm_min);

			sprintf(nameMsg, "[%s:%s] %s %s", tm_hour, tm_min, userName, msg);
			send(hSock, nameMsg, strlen(nameMsg), 0);
		}
	}
	return 0;
}

unsigned WINAPI RecvMsg(void* arg)   // read thread main
{
	int hSock = *((SOCKET*)arg);
	char nameMsg[NAME_SIZE + BUF_SIZE];
	int strLen;

	while (1) {
		strLen = recv(hSock, nameMsg, NAME_SIZE + BUF_SIZE - 1, 0);
		nameMsg[strLen] = '\0';
		// -------------------------------------------------------------------------------
		/* ���� ó���� ���� �޽��� �߼����� �г����� �����ϰ� ���� ��ϰ� ���ϴ� �۾� */
		/* ���� ��Ͽ� �����Ѵٸ� �ش� �����κ��� �޽����� ������� �ʴ´�. */
		char temp[NAME_SIZE + BUF_SIZE];
		char del[] = " ";
		char* token;
		int block = 0;
		strcpy(temp, nameMsg);
		token = strtok(temp, del);
		token = strtok(NULL, del);
		if (strLen == 0)
			break;

		for (int i = 0; i < blaCnt; i++)
			if (!strcmp(token, blackList[i]))
				block = 1;

		if (!block)
			fputs(nameMsg, stdout);
		// -------------------------------------------------------------------------------
	}
	return 0;
}

void ErrorHandling(char* msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
void print_cls(void)
{
	system("cls");
	printf("������������������������������������������������������������������������������������������\n");
	printf(" [Chatting Program - Computer Network Team.9]\n");
	printf(" [���� ����� ���Ͻø� /h �� �Է��ϼ���.  ] \n");
	printf("������������������������������������������������������������������������������������������\n");
}