// verify.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <Windows.h>
#include <vector>
#include <string>



int _tmain(int argc, _TCHAR* argv[])
{
    std::vector<int> vstr; 

    for(int i = 0;i < 10; i++){
        vstr.push_back(i);
    }

    int value = vstr.back();
	return 0;
}

