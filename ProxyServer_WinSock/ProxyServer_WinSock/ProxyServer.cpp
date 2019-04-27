
#include"stdafx.h"
#include<WinSock2.h>
#include <ws2tcpip.h>
#include<iostream>
#include"Proxy_Parse.h"
#pragma comment(lib,"Ws2_32.lib")
#define DEFAULT_BUFLEN 1460
#define DEFAULT_PORT "8888"

//using namespace std;



UINT Proxy(LPVOID prams)
{
	cout << "Da co Client ket noi !!! \n\n";
	SOCKET ClientSocket = (SOCKET)prams;
	char request[1000] = { 0 }, dname[100] = { 0 }, ip[16] = { 0 },
		body_res[DEFAULT_BUFLEN] = { 0 }, header_res[5000] = { 0 };

	int bytes = recv(ClientSocket, request, sizeof request, 0);
	if (bytes > 0) {
		cout << "Bytes received:" << bytes << request;
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

	//
	if (GetDomainName(request, dname) == false)
	{
		cout << "Get Domain Name False !!!";
		closesocket(ClientSocket);
		return 0;
	}
	else cout << dname << "\n";
	// 
	//GET IP:
	hostent *remoteHost = gethostbyname(dname);
	/*if (remoteHost->h_length == 0)
	{
		closesocket(ClientSocket);
		std::cout << "Khong the Get IP !!!" << endl << endl;
		return 0;
	}*/
	in_addr addr;
	addr.s_addr = *(u_long *)remoteHost->h_addr_list[0];
	

	SOCKET ConnectSocket = INVALID_SOCKET;
	ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ConnectSocket == INVALID_SOCKET) {
		cout << "socket failed with error: " << WSAGetLastError() << endl;
		WSACleanup();
		return 0;
	}
	sockaddr_in AddrIP;
	AddrIP.sin_family = AF_INET;
	AddrIP.sin_port = htons(80);
	AddrIP.sin_addr = addr;

	std::cout << "IPv4: " << inet_ntoa(addr) << endl << endl;
	
	int iResult = connect(ConnectSocket, (sockaddr *)&AddrIP, sizeof AddrIP);
	if (iResult == SOCKET_ERROR) {
		closesocket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET;
		cout << "Failed Connect with web server\n";
		return 0;
	}
	else cout << "Connect Success to Web\n\n";

	bytes = send(ConnectSocket, request, bytes, 0);

	int id = 0, endhead = 0;
	string head;
	while (endhead < 4)
	{
		bytes = recv(ConnectSocket, header_res + id, 1, 0);
		head.push_back(header_res[id]);
		cout << header_res[id];
		if (header_res[id] == '\r' || header_res[id] == '\n')
			endhead++;
		else endhead = 0;
		id++;
	}
	bytes = send(ClientSocket, header_res, id, 0);
	cout << id << " Byte ---- Xong header roi nhe !\n\n";
	int ctlength = GetContent_Length(head);
	cout << "Content-Length: " << ctlength << endl;
	int bytes_rev = 0, sum_bytes = 0;
	do
	{
		bytes_rev = recv(ConnectSocket, body_res, DEFAULT_BUFLEN, 0);
		sum_bytes += bytes_rev;
		ctlength -= bytes_rev;
		if (bytes_rev > 0) {
			cout << "Bytes received: " << bytes_rev << endl;

			// Echo the buffer back to the sender
			int bytes_send = send(ClientSocket, body_res, bytes_rev, 0);
			if (bytes_send == SOCKET_ERROR) {
				cout << "send failed with error: " << WSAGetLastError();
				closesocket(ConnectSocket);
				closesocket(ClientSocket);
				return 0;
			}
			if (ctlength > 0)
				continue;
			if (bytes_rev < DEFAULT_BUFLEN)
			{
				cout << sum_bytes << endl;
				closesocket(ConnectSocket);
				closesocket(ClientSocket);
				return 0;
			}
		}
		else if (bytes_rev == 0)
		{
			cout << "Connection closing...\n";
			closesocket(ConnectSocket);
			closesocket(ClientSocket);
			return 0;
		}
		else {
			cout << "recv failed with error: " << WSAGetLastError();
			closesocket(ConnectSocket);
			closesocket(ClientSocket);
			//WSACleanup();
			return 0;
		}
	} while (bytes_rev > 0);
	cout << sum_bytes << endl;
	cout << "Da Thuc Hien Xong !\n\n";

	closesocket(ConnectSocket);
	closesocket(ClientSocket);
	return 1;
}


int main()
{
	WSADATA wsaData;
	int iResult;
	
	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;
	
	struct addrinfo *result = NULL;
	struct addrinfo hints;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		cout << "WSAStartup failed with error: " << iResult;
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
		cout << "getaddrinfo failed with error: " << iResult;
		WSACleanup();
		return 1;
	}
	
	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		cout << "socket failed with error: " << WSAGetLastError();
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		cout << "bind failed with error: " << WSAGetLastError();
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		cout << "listen failed with error: " << WSAGetLastError();
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	
	while (true) 
	{
		//SOCKET ClientSocket = INVALID_SOCKET;
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			cout << "accept failed with error: " << WSAGetLastError();
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
