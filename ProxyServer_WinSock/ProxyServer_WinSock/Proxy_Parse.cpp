﻿//#include "pch.h"
#include "Proxy_Parse.h"

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
bool IsHTTPs(char *request)
{
	int i = 0, n = strlen(request);
	while (i < n)
	{
		if (request[i] == ':' && request[i + 1] == '4' && request[i + 2] == '4'
			&& request[i + 3] == '3')
			return true;
		i++;
	}
	return false;
}
bool IsGETMethod(char *request)
{
	if (request[0] == 'G' && request[1] == 'E' && request[2] == 'T')
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