#include "stdafx.h"
#include<string>
using namespace std;


bool GetDomainName(char *request, char *dname);
bool IsHTTPs(char *request);
bool IsGETMethod(char *request);
int GetContent_Length(string head);
