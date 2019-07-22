#pragma once

#include "utils.h"
#include <Windows.h>
#include <sqlext.h>
#include <vector>
#include <string>

class DBConn
	: public TemplateSingleton<DBConn>
{
public:
	struct DBParameter
	{
		std::vector<std::pair<std::wstring /*param*/, std::wstring /*data*/>> Params;
	};

	struct DBQueryResult
	{
		std::vector< std::wstring /*column_name*/> Columns;
		std::vector< std::vector<std::wstring /*datas*/> > Datas;
	};

public:
	DBConn();
	virtual ~DBConn();

	bool Initialize();
	DBQueryResult GetQueryResult(const std::wstring& query, const DBParameter& params);

private:
	bool CheckSQLSuccess(SQLRETURN&);

	void AllocateHandles();	// �ڵ� ���� �ʱ�ȭ
	void ConnectDataSource(); // DBMS ����
	void ExecuteStatementDirect(SQLTCHAR* query); // ���� �ٷ� ����
	void PrepareStatement(SQLTCHAR* query); // ������ �غ� ����
	void ExecuteStatement(); // �غ�� ������ ����
	DBQueryResult FetchResult(); // ��� Fetch
	void DisconnectDataSource(); // �ڵ� release

private:
	SQLHENV henv{ NULL };
	SQLHDBC hdbc{ NULL };
	SQLHSTMT hstmt{ NULL };

	SQLSMALLINT length;
	SQLSMALLINT rec;
	SQLINTEGER native;
	SQLTCHAR state[7];
	SQLTCHAR message[256];
};

