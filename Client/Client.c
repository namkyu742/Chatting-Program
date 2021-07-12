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
	printf(" 닉네임 : ");
	char tempname[20];
	scanf("%s", &tempname);
	sprintf(userName, "[%s]", tempname);
	hSock = socket(PF_INET, SOCK_STREAM, 0);

	//----------------------------------
	char addr[20];
	int portnum = 5000;
	printf(" IP주소 : ");
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
	print_cls();					// 화면 비우기
	SOCKET hSock = *((SOCKET*)arg);
	char nameMsg[NAME_SIZE + BUF_SIZE];
	char action[BUF_SIZE];
	char del[] = " ";		// strtok 구분자
	char* token;			// strtok 결과 저장

	while (1) {
		fgets(msg, BUF_SIZE, stdin);	// 메시지를 입력 받는다.

		strcpy(action, msg);
		token = strtok(action, del);	// 명령어를 추출한다.

		if (!strcmp(action, "q\n") || !strcmp(action, "Q\n")) {
			send(hSock, msg, BUF_SIZE, 0);		// 종료메시지 전송용
			closesocket(hSock);
			exit(0);
		}
		else if (!strcmp(action, "/list\n") || !strcmp(action, "/l\n") || !strcmp(action, "/to") || !strcmp(action, "/h\n")) {
			send(hSock, msg, BUF_SIZE, 0);	// 서버에 해당 명령어 실행 요청
		}
		else if (!strcmp(action, "/b")) {		/* 유저 차단 */
			token = strtok(NULL, del);
			token[strlen(token) - 1] = '\0';
			strcpy(blackList[blaCnt], token);		// 차단 목록에 닉네임 등록
			printf("[Blocked Completed] : %s\n", blackList[blaCnt]);
			blaCnt += 1;							// 차단 목록 인덱스 증가
		}
		else if (!strcmp(action, "/br")) {		/* 유저 차단 해제 */
			int find = 0;
			token = strtok(NULL, del);
			token[strlen(token) - 1] = '\0';

			for (int i = 0; i < blaCnt; i++) {	// 차단목록에 유저가 있는지 검사
				if (!strcmp(token, blackList[i])) {	// 있다면 목록에서 제외하고 갱신
					printf("[Unblocked Completed] : %s\n", blackList[i]);
					while (i < blaCnt) {
						strcpy(blackList[i], blackList[i + 1]);
						find = 1;
						i++;
					}
				}
			}
			if (find)
				blaCnt -= 1;						 // 차단 목록 인덱스 감소
			else
				printf("Can not found\n");
		}
		else if (!strcmp(action, "/blist\n") || !strcmp(action, "/bl\n")) { /* 유저 차단 목록 */
			printf("┌────────┤ BLACK LIST├────────┐\n");
			printf("├─────┬───────────────────────┤\n");
			printf("│ %3s │ %21s │\n", "No", "User Name");
			printf("├─────┼───────────────────────┤\n");
			if (blaCnt == 0)
				printf("│     │        No Data        │\n");
			else
				for (int i = 0; i < blaCnt; i++) {
					printf("│ %3d │ %21s │\n", i + 1, blackList[i]);	// 닉네임 출력
				}
			printf("└─────┴───────────────────────┘\n");
		}
		else if (!strcmp(action, "/cls\n")) {		/* 화면 비우기 */
			print_cls();
		}
		else if (!strcmp(action, "\n")) {
			/* 입력 없는 상태에서 엔터만 눌렀을 경우의 전송을 방지 */
			// 아무 내용 없으면 Send 하지 않는다.
		}
		else {							/* 서버에 메시지 전송 */
			time_t t = time(NULL);				// 현재 시간
			struct tm tm = *localtime(&t);		// 시간 출력을 위한 코드
			char tm_hour[3];
			char tm_min[3];
			// 10 미만이면 한자리 수로 출력되기 때문에 앞에 0을 붙인다.
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
		/* 차단 처리를 위해 메시지 발송인의 닉네임을 추출하고 차단 목록과 비교하는 작업 */
		/* 차단 목록에 존재한다면 해당 유저로부터 메시지를 출력하지 않는다. */
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
	printf("─────────────────────────────────────────────\n");
	printf(" [Chatting Program - Computer Network Team.9]\n");
	printf(" [도움말 출력을 원하시면 /h 를 입력하세요.  ] \n");
	printf("─────────────────────────────────────────────\n");
}