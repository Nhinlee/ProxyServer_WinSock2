//#include "pch.h"
#include "Proxy_Parse.h"
#include<fstream>	
#include<time.h>
//data:
map<string, int> blacklist;
string fbd403;
struct HEADER_IN_CACHE {
	bool Exist;
	string filename;
	string date;
	int size;
};
//Function:
bool GetDomainName(char *request, char *dname)
{
	int n = strlen(request), pos = 0, i = 0;
	n -= 6;
	for (; i < n; i++)
	{
		if (request[i] == 'H' && request[i + 1] == 'o' && request[i + 2] == 's' && request[i + 3] == 't'
			&& request[i + 4] == ':')
		{
			pos = i + 6;
			break;
		}
	}
	if (i == n)
		return false;
	i = 0;
	while (pos < n && request[pos] != '\r'/*&& request[pos] != ':'*/)// Nếu là cổng 443 (HTTPs) thì phải chạy tới : và dừng lại.
	{
		dname[i] = request[pos];
		pos++;
		i++;
	}
	dname[i] = '\0';
	return true;

}

bool IsGETMethod(char *request)
{
	if (strlen(request) < 3)
		return false;
	if (request[0] == 'G' && request[1] == 'E' && request[2] == 'T')
		return true;
	return false;
}

bool IsPOSTMethod(char *request)
{
	if (strlen(request) < 4)
		return false;
	if (request[0] == 'P' && request[1] == 'O' && request[2] == 'S' && request[3] == 'T')
		return true;
	return false;
}

int GetContent_Length(string head)
{
	int pos = head.find("Content-Length: ");
	int sum = 0;
	if (pos == -1)
		return -1;
	else {
		string temp;
		pos += 16;
		while (head[pos] != '\r')
		{
			sum += head[pos] - '0';
			sum *= 10;
			pos++;
		}
		sum /= 10;
		return sum;
	}
}

bool UpdateBlacklist(string input)
{
	ifstream inp(input);
	if (inp.fail())
		return false;
	string host;
	while (!inp.eof())
	{
		inp >> host;
		blacklist[host]++;
		if (host.find("www.") == -1)
		{
			host = "www." + host;
			blacklist[host]++;
		}
	}
	return true;
}

char* strtochar(string s)
{
	int n = s.length();
	char* a = new char[n + 1];
	for (int i = 0; i < n; i++)
		a[i] = s[i];
	a[n] = '\0';
	return a;
}

void UpdateDate(string &res)
{
	time_t t;
	tm *pt;
	time(&t);
	pt = gmtime(&t);
	int pos = res.find("Date: ");
	char* temp = asctime(pt);
	temp[strlen(temp) - 1] = '\0';
	res.insert(pos + 6, " GMT");
	res.insert(pos + 6, temp);
}

bool Update403(string input)
{
	ifstream inp(input);
	if (inp.fail())
		return false;
	char c;
	while (!inp.eof())
	{
		c = inp.get();
		if (c == -1)
			break;
		if (c == '\n')
			fbd403.push_back('\r');
		fbd403.push_back(c);
	}
	UpdateDate(fbd403);
	return true;
}

UINT Proxy(LPVOID prams)
{
	cout << "Da co Client ket noi !!! \n\n";
	// Initialize buffer memory and ClientSocket:
	SOCKET ClientSocket = (SOCKET)prams;
	// Buffer:
	char request[5000] = { 0 }, dname[100] = { 0 }, ip[16] = { 0 },
		body_res[DEFAULT_BUFLEN] = { 0 }, header_res[5000] = { 0 };

	// Recieve Request from Client( Browser).
	int bytes = recv(ClientSocket, request, sizeof request, 0);
	if (bytes > 0) {
		cout << "Bytes received:" << bytes << endl << request;
	}
	else if (bytes == 0)
	{
		cout << " No Request !!!\n";
		closesocket(ClientSocket);
		return 0;
	}

	//If Request is GET or POST method, It's continue processes
	if (!IsGETMethod(request) && !IsPOSTMethod(request))
	{
		closesocket(ClientSocket);
		return 0;
	}

	//Get File Name:
	string filename = GetFileName(request);

	//Get Host Name:
	if (GetDomainName(request, dname) == false)
	{
		cout << "Get Domain Name False !!!";
		closesocket(ClientSocket);
		return 0;
	}
	else cout << dname << "\n";

	
	//Refuse with blacklist:
	if (blacklist[dname])
	{
		//Update 403 request:
		Update403("403.conf");
		char* res = strtochar(fbd403);
		bytes = send(ClientSocket, res, (int)strlen(res), 0);
		fbd403.clear();
		delete res;
		cout << "-------------Web in black list----------------" << endl;
		closesocket(ClientSocket);
		return 0;
	}

	//GET IP from Host name of web server:
	hostent *remoteHost = gethostbyname(dname);
	/*if (remoteHost->h_length == 0)
	{
		closesocket(ClientSocket);
		std::cout << "Khong the Get IP !!!" << endl << endl;
		return 0;
	}*/
	in_addr addr;
	addr.s_addr = *(u_long *)remoteHost->h_addr_list[0];

	//Create SOCKET connect to web server with ip and post 80 default:
	SOCKET ConnectSocket = INVALID_SOCKET;
	ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ConnectSocket == INVALID_SOCKET) {
		cout << "socket failed with error: " << WSAGetLastError() << endl;
		WSACleanup();
		return 0;
	}
	//Create struct sockaddr_in to save infor of web server:
	sockaddr_in AddrIP;
	AddrIP.sin_family = AF_INET;
	AddrIP.sin_port = htons(80);
	AddrIP.sin_addr = addr;

	//Connect web server:
	int iResult = connect(ConnectSocket, (sockaddr *)&AddrIP, sizeof AddrIP);
	if (iResult == SOCKET_ERROR) {
		closesocket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET;
		cout << "Failed Connect with web server\n";
		return 0;
	}
	else cout << "Connect Success to Web\n\n";

	//Send request from client to web server with bytes exactly, that we save it before:
	bytes = send(ConnectSocket, request, bytes, 0);

	// Recieve Header Respones:
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
	//

	//Send Header Respones for client( Browser) with id (bytes) exactly:
	bytes = send(ClientSocket, header_res, id, 0);
	cout << id << " Byte ---- Xong header roi nhe !\n\n";

	//Get Content-Length of body res:
	int ctlength = GetContent_Length(head);
	cout << "Content-Length: " << ctlength << endl;

	//
	string headerdate = Get_Last_Modified(header_res);
	HEADER_IN_CACHE temp;
	temp = { false,"","",0 };
	temp = Find_In_Cache(filename);

	if (temp.Exist && headerdate == temp.date)
	{
		ifstream inp(temp.filename+".conf");
		char c;
		while (!inp.eof())
		{
			c = inp.get();
			bytes = send(ClientSocket, &c, 1, 0);
		}
		closesocket(ClientSocket);
		closesocket(ConnectSocket);
		return 1;
	}
	
	//
	//Get body response from web server:
	int bytes_rev = 0, sum_bytes = 0;
	ofstream out(filename + ".conf");
	do
	{
		bytes_rev = recv(ConnectSocket, body_res, DEFAULT_BUFLEN, 0);
		sum_bytes += bytes_rev;
		ctlength -= bytes_rev;
		if (bytes_rev > 0) {
			cout << "Bytes received: " << bytes_rev << endl;
			//
			string body = chartostr(body_res);
			out << body;
			//
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

	temp.date = headerdate;
	temp.Exist = true;
	temp.filename = filename;
	temp.size = sum_bytes;
	BackUpHeader(temp);

	cout << sum_bytes << endl;
	cout << "Da Thuc Hien Xong !\n\n";

	//Close SOCKET and return function:
	closesocket(ConnectSocket);
	closesocket(ClientSocket);
	return 1;
}