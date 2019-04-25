
#include"stdafx.h"
#include<WinSock2.h>
#include <ws2tcpip.h>
//#include<iostream>
#include"Proxy_Parse.h"
#pragma comment(lib,"Ws2_32.lib")
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "8888"

using namespace std;



UINT Proxy(LPVOID prams)
{
	cout << "Da co Client ket noi !!!" << endl << endl;
	SOCKET ClientSocket = (SOCKET)prams;
	char request[1000] = { 0 }, dname[1000], ip[16], body_res[5000] = { 0 }, header_res[1000] = { 0 };

	int bytes = recv(ClientSocket, request, sizeof request, 0);
	if (bytes > 0) {
		printf("Bytes received: %d\n", bytes);
		cout << request;
	}
	else if (bytes == 0)
	{
		cout << " No Request !!!\n";
		return 0;
	}
	
	if (!IsGETMethod(request))
	{
		closesocket(ClientSocket);
		return 0;
	}

	/*if (IsHTTPs(request))
	{
		closesocket(ClientSocket);
		cout << "is HTTPS" << endl;
		return 0;
	}*/

	//
	if (GetDomainName(request, dname) == false)
	{
		cout << "Get Domain Name False !!!";
	}
	// 
	//GET IP:
	hostent *remoteHost = gethostbyname(dname);
	in_addr addr;
	addr.s_addr = *(u_long *)remoteHost->h_addr_list[0];
	

	SOCKET ConnectSocket = INVALID_SOCKET;
	ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ConnectSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	sockaddr_in AddrIP;
	AddrIP.sin_family = AF_INET;
	AddrIP.sin_port = htons(80);
	AddrIP.sin_addr.s_addr = addr.s_addr;
	cout << "IPv4: "<<inet_ntoa(addr) << endl;
	
	int iResult = connect(ConnectSocket, (sockaddr *)&AddrIP, sizeof AddrIP);
	if (iResult == SOCKET_ERROR) {
		closesocket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET;
		cout << "Failed Connect with web server" << endl;
		return 0;
	}
	else cout << "Connect Success to Web" << endl << endl;

	bytes = send(ConnectSocket, request, bytes, 0);

	int id = 0, endhead = 0;
	while (endhead < 4)
	{
		bytes = recv(ConnectSocket, header_res + id, 1, 0);
		//head.push_back(header_res[id]);
		cout << header_res[id];
		if (header_res[id] == '\r' || header_res[id] == '\n')
			endhead++;
		else endhead = 0;
		id++;
	}
	bytes = send(ClientSocket, header_res, id, 0);
	cout << id << " Byte ---- Xong header roi nhe !" << endl << endl;

	id = 0;
	while ((bytes = recv(ConnectSocket,body_res,5000,0)) > 0)
	{
		
		//cout << "Tong byte nhan tu web lan thu " << id << " : " << bytes << endl;
		bytes = send(ClientSocket, body_res, bytes, 0);
		//cout << "Tong byte gui client la thu " << id << " : " << bytes << endl;
		memset(body_res, 0, sizeof body_res);
		id++;
	}

	cout << "Da Thuc Hien Xong !" << endl;

	closesocket(ConnectSocket);
	closesocket(ClientSocket);
}


int main()
{
	WSADATA wsaData;
	int iResult;
	
	SOCKET ListenSocket = INVALID_SOCKET;
	//SOCKET ClientSocket = INVALID_SOCKET;
	
	struct addrinfo *result = NULL;
	struct addrinfo hints;
	/*int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;*/

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}
	
	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	
	// Accept a client socket
	while (true) 
	{
		SOCKET ClientSocket = INVALID_SOCKET;
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		AfxBeginThread(Proxy, (LPVOID)ClientSocket);
		//Proxy((LPVOID)ClientSocket);
	}

	
	WSACleanup();
	system("pause");
	return iResult;
}
