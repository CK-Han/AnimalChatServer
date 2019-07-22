#include <iostream>

#include "framework.h"
#include "DBConn.h"

int main()
{
	std::cout << "hi" << std::endl;

	Framework::GetInstance();

	//DBConn::GetInstance();

	std::cout << "press input any key to shutdown\n";
	char ch;
	std::cin >> ch;

	return 0;
}