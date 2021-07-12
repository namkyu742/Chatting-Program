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

unsigned WINAPI HandleClnt(void* arg);
void SendMsg2(char* msg, int len, SOCKET hClntSock);
void ErrorHandling(char* msg);

int clntCnt = 0;
SOCKET clntSocks[MAX_CLNT];
HANDLE hMutex;
char userName[MAX_CLNT][NAME_SIZE];		// ���� �г��� ����
char userIP[MAX_CLNT][NAME_SIZE];		// ���� IP�ּ� ����
char myName[NAME_SIZE];					// �ӽ� �г��� ����

int main(void)
{
	WSADATA wsaData;
	SOCKET hServSock, hClntSock;
	SOCKADDR_IN servAdr, clntAdr;
	HANDLE  hThread;
	int clntAdrSz;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	hMutex = CreateMutex(NULL, FALSE, NULL);
	hServSock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(atoi("5000"));

	if (bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
		ErrorHandling("bind() error");
	if (listen(hServSock, 5) == SOCKET_ERROR)
		ErrorHandling("listen() error");

	printf("SERVER START\n\n");
	while (1) {
		clntAdrSz = sizeof(clntAdr);
		hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &clntAdrSz);
		
		recv(hClntSock, myName, NAME_SIZE, 0);		// Ŭ���̾�Ʈ�� �г����� �޾Ƽ� ����
		WaitForSingleObject(hMutex, INFINITE);
		strcpy(userName[clntCnt], myName);			// ���� �г��� �迭�� ����
		strcpy(userIP[clntCnt], inet_ntoa(clntAdr.sin_addr));	// ���� IP�ּ� �迭�� ����
		clntSocks[clntCnt++] = hClntSock;
		ReleaseMutex(hMutex);

		hThread = (HANDLE)_beginthreadex(NULL, 0, HandleClnt, (void*)&hClntSock, 0, NULL);
		printf("Connected client    : %s %s\n", inet_ntoa(clntAdr.sin_addr, myName));
	}
	closesocket(hServSock);
	WSACleanup();
	return 0;
}

unsigned WINAPI HandleClnt(void* arg)
{
	SOCKET hClntSock = *((SOCKET*)arg);
	int strLen = 0, i;
	char msg[BUF_SIZE];			// �޽���
	char temp[BUF_SIZE];		// �ӽ� ���ڹ迭
	char action[BUF_SIZE];		// ��ɾ�
	char del[] = " ";			// strtok ������
	char* token, * target;		// strtok ��� ����

	while ((strLen = recv(hClntSock, msg, sizeof(msg), 0)) > 0) {
		strcpy(action, msg);
		token = strtok(action, del);
		token = strtok(NULL, del);
		
		if (!strcmp(action, "/list\n") || !strcmp(action, "/l\n")) {	/* ���� ��� ��� */
			char list_msg[1000];		// ���� ��� ��� �޽���
			memset(list_msg, 0, sizeof(list_msg));

			sprintf(list_msg, "���������������������������������������� USER LIST������������������������������������������ \n");
			strcat(list_msg, "������������������������������������������������������������������������������������������������������ \n");
			sprintf(temp, "�� %20s �� %7s �� %14s ��\n", "USER NAME", "SOCKET", "USER IP");
			strcat(list_msg, temp);
			strcat(list_msg, "������������������������������������������������������������������������������������������������������ \n");
		
			WaitForSingleObject(hMutex, INFINITE);
			for (int i = 0; i < clntCnt; i++) {
				
				sprintf(temp, "�� %20s �� %7d �� %14s ��\n", userName[i], clntSocks[i], userIP[i]);		// ���� �г���, ����, IP�ּҸ� ���
				strcat(list_msg, temp);
			}
			ReleaseMutex(hMutex);
			strcat(list_msg, "������������������������������������������������������������������������������������������������������ \n");

			send(hClntSock, list_msg, strlen(list_msg), 0);
		}
		else if (!strcmp(action, "/to")) {			/* �ӼӸ� ��� */
			target = token;
			token = strtok(NULL, del);

			SOCKET hTargetSock = NULL;				// �ӼӸ� ��� ����
			
			for (i = 0; i < clntCnt; i++)			// �������� �������߿� ������ ������ �˻�
				if (!strcmp(userName[i], target))
					hTargetSock = clntSocks[i];

			if (hTargetSock == NULL) {				// ������ ��밡 �������� ���� ���
				sprintf(msg, "Target doesn't exist\n");
				send(hClntSock, msg, strlen(msg), 0);
			}
			else {									// ������ ��밡 ������ ���
				time_t t = time(NULL);				// ���� �ð�
				struct tm tm = *localtime(&t);		// �ð� ����� ���� �ڵ�
				char tm_hour[3];
				char tm_min[3];
				// 10 �̸��̸� ���ڸ� ���� ��µǱ� ������ �տ� 0�� ���δ�.
				if (tm.tm_hour < 10)	sprintf(tm_hour, "0%d", tm.tm_hour);
				else					sprintf(tm_hour, "%d", tm.tm_hour);
				if (tm.tm_min < 10)		sprintf(tm_min, "0%d", tm.tm_min);
				else					sprintf(tm_min, "%d", tm.tm_min);

				sprintf(msg, "[%2s:%2s] %s_[whispher] %s", tm_hour, tm_min, userName[i], token);
				while ((token = strtok(NULL, del)) != NULL){	// �޽��� ���� �ű��
					strcat(msg, " ");
					strcat(msg, token);
				}
				send(hTargetSock, msg, strlen(msg), 0);			// �ش� �������� �޽��� ����
			}
		}
		else if (!strcmp(action, "q\n")) {	/* ���� ���� �޽��� */
			char exitmsg[50];		// ���� ���� �˸� ��� �޽���
			for (int i = 0; i < MAX_CLNT; i++) {
				if (clntSocks[i] == hClntSock) {	// ������ ������ �̸� ����
					sprintf(exitmsg, "%s ���� �����ϼ̽��ϴ�.\n", userName[i]);
					SendMsg2(exitmsg, sizeof(exitmsg), hClntSock);
					break;
				}
			}
		}
		else if (!strcmp(action, "/h\n")) {		/* ���� ��� */
			char help_msg[1000];		// ���� ��� �޽���
			memset(help_msg, 0, sizeof(help_msg));
			sprintf(help_msg, "���������������������������������������� HELP������������������������������������������ \n");
			strcat(help_msg, "�������������������������������������������������������������������������������������������� \n");
			strcat(help_msg, "�� ���� ��� ��� �� /l or /list               �� \n");
			strcat(help_msg, "�� ���� ����      �� /b [username]             �� \n");
			strcat(help_msg, "�� ���� ���� ���� �� /br [username]            �� \n");
			strcat(help_msg, "�� ���� ��� ��� �� /bl or /blist             �� \n");
			strcat(help_msg, "�� �ӼӸ� ����    �� /to [username] message    �� \n");
			strcat(help_msg, "�� ȭ�� Ŭ����    �� /cls                      �� \n");
			strcat(help_msg, "�� ����           �� q                         �� \n");
			strcat(help_msg, "�������������������������������������������������������������������������������������������� \n");

			send(hClntSock, help_msg, strlen(help_msg), 0);
		}
		else {
			SendMsg2(msg, strLen, hClntSock);
		}
	}

	WaitForSingleObject(hMutex, INFINITE);
	for (i = 0; i < clntCnt; i++)  {		// remove disconnected client
		if (hClntSock == clntSocks[i]) {
			printf("disconnected client : %s %s\n", userIP[i], userName[i]);
			while (i < clntCnt) {
				clntSocks[i] = clntSocks[i + 1];		// ���� �迭 ����
				strcpy(userName[i], userName[i + 1]);	// ���� �г��� �迭 ����
				strcpy(userIP[i], userIP[i + 1]);		// ���� IP�ּ� �迭 ����
				i++;
			}
			break;
		}
	}
	clntCnt--;
	ReleaseMutex(hMutex);
	closesocket(hClntSock);
	return 0;
}

void SendMsg2(char* msg, int len, SOCKET hClntSock)		// send to all(without self)
{	// ������ hClntSock�� ���� ������ ����ϴ� Ŭ���̾�Ʈ���Դ� �޽����� ������ ����
	WaitForSingleObject(hMutex, INFINITE);
	for (int i = 0; i < clntCnt; i++)
		if(hClntSock!=clntSocks[i])
			send(clntSocks[i], msg, len, 0);

	ReleaseMutex(hMutex);
}

void ErrorHandling(char* msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
