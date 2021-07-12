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
char userName[MAX_CLNT][NAME_SIZE];		// 유저 닉네임 저장
char userIP[MAX_CLNT][NAME_SIZE];		// 유저 IP주소 저장
char myName[NAME_SIZE];					// 임시 닉네임 저장

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
		
		recv(hClntSock, myName, NAME_SIZE, 0);		// 클라이언트의 닉네임을 받아서 저장
		WaitForSingleObject(hMutex, INFINITE);
		strcpy(userName[clntCnt], myName);			// 유저 닉네임 배열에 저장
		strcpy(userIP[clntCnt], inet_ntoa(clntAdr.sin_addr));	// 유저 IP주소 배열에 저장
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
	char msg[BUF_SIZE];			// 메시지
	char temp[BUF_SIZE];		// 임시 문자배열
	char action[BUF_SIZE];		// 명령어
	char del[] = " ";			// strtok 구분자
	char* token, * target;		// strtok 결과 저장

	while ((strLen = recv(hClntSock, msg, sizeof(msg), 0)) > 0) {
		strcpy(action, msg);
		token = strtok(action, del);
		token = strtok(NULL, del);
		
		if (!strcmp(action, "/list\n") || !strcmp(action, "/l\n")) {	/* 유저 목록 출력 */
			char list_msg[1000];		// 유저 목록 출력 메시지
			memset(list_msg, 0, sizeof(list_msg));

			sprintf(list_msg, "┌──────────────────┤ USER LIST├───────────────────┐ \n");
			strcat(list_msg, "├──────────────────────┬─────────┬────────────────┤ \n");
			sprintf(temp, "│ %20s │ %7s │ %14s │\n", "USER NAME", "SOCKET", "USER IP");
			strcat(list_msg, temp);
			strcat(list_msg, "├──────────────────────┼─────────┼────────────────┤ \n");
		
			WaitForSingleObject(hMutex, INFINITE);
			for (int i = 0; i < clntCnt; i++) {
				
				sprintf(temp, "│ %20s │ %7d │ %14s │\n", userName[i], clntSocks[i], userIP[i]);		// 유저 닉네임, 소켓, IP주소를 출력
				strcat(list_msg, temp);
			}
			ReleaseMutex(hMutex);
			strcat(list_msg, "└──────────────────────┴─────────┴────────────────┘ \n");

			send(hClntSock, list_msg, strlen(list_msg), 0);
		}
		else if (!strcmp(action, "/to")) {			/* 귓속말 기능 */
			target = token;
			token = strtok(NULL, del);

			SOCKET hTargetSock = NULL;				// 귓속말 상대 소켓
			
			for (i = 0; i < clntCnt; i++)			// 접속중인 유저들중에 지정한 유저를 검색
				if (!strcmp(userName[i], target))
					hTargetSock = clntSocks[i];

			if (hTargetSock == NULL) {				// 지정한 상대가 존재하지 않을 경우
				sprintf(msg, "Target doesn't exist\n");
				send(hClntSock, msg, strlen(msg), 0);
			}
			else {									// 지정한 상대가 존재할 경우
				time_t t = time(NULL);				// 현재 시간
				struct tm tm = *localtime(&t);		// 시간 출력을 위한 코드
				char tm_hour[3];
				char tm_min[3];
				// 10 미만이면 한자리 수로 출력되기 때문에 앞에 0을 붙인다.
				if (tm.tm_hour < 10)	sprintf(tm_hour, "0%d", tm.tm_hour);
				else					sprintf(tm_hour, "%d", tm.tm_hour);
				if (tm.tm_min < 10)		sprintf(tm_min, "0%d", tm.tm_min);
				else					sprintf(tm_min, "%d", tm.tm_min);

				sprintf(msg, "[%2s:%2s] %s_[whispher] %s", tm_hour, tm_min, userName[i], token);
				while ((token = strtok(NULL, del)) != NULL){	// 메시지 내용 옮기기
					strcat(msg, " ");
					strcat(msg, token);
				}
				send(hTargetSock, msg, strlen(msg), 0);			// 해당 유저에게 메시지 전송
			}
		}
		else if (!strcmp(action, "q\n")) {	/* 유저 퇴장 메시지 */
			char exitmsg[50];		// 유저 퇴장 알림 출력 메시지
			for (int i = 0; i < MAX_CLNT; i++) {
				if (clntSocks[i] == hClntSock) {	// 퇴장한 유저의 이름 추출
					sprintf(exitmsg, "%s 님이 퇴장하셨습니다.\n", userName[i]);
					SendMsg2(exitmsg, sizeof(exitmsg), hClntSock);
					break;
				}
			}
		}
		else if (!strcmp(action, "/h\n")) {		/* 도움말 출력 */
			char help_msg[1000];		// 도움말 출력 메시지
			memset(help_msg, 0, sizeof(help_msg));
			sprintf(help_msg, "┌──────────────────┤ HELP├───────────────────┐ \n");
			strcat(help_msg, "├────────────────┬───────────────────────────┤ \n");
			strcat(help_msg, "│ 유저 목록 출력 │ /l or /list               │ \n");
			strcat(help_msg, "│ 유저 차단      │ /b [username]             │ \n");
			strcat(help_msg, "│ 유저 차단 해제 │ /br [username]            │ \n");
			strcat(help_msg, "│ 차단 목록 출력 │ /bl or /blist             │ \n");
			strcat(help_msg, "│ 귓속말 전송    │ /to [username] message    │ \n");
			strcat(help_msg, "│ 화면 클리어    │ /cls                      │ \n");
			strcat(help_msg, "│ 종료           │ q                         │ \n");
			strcat(help_msg, "└────────────────┴───────────────────────────┘ \n");

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
				clntSocks[i] = clntSocks[i + 1];		// 소켓 배열 갱신
				strcpy(userName[i], userName[i + 1]);	// 유저 닉네임 배열 갱신
				strcpy(userIP[i], userIP[i + 1]);		// 유저 IP주소 배열 갱신
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
{	// 지정한 hClntSock과 같은 소켓을 사용하는 클라이언트에게는 메시지를 보내지 않음
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
