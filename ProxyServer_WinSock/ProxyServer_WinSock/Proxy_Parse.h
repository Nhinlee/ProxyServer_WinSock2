#include "stdafx.h"
#define DEFAULT_BUFLEN 1460
#define DEFAULT_PORT "8888"

using namespace std;


bool GetDomainName(char *request, char *dname);
bool IsGETMethod(char *request);
bool IsPOSTMethod(char *request);
int GetContent_Length(string head);
UINT Proxy(LPVOID prams);