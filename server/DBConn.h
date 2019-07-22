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

	void AllocateHandles();	// 핸들 변수 초기화
	void ConnectDataSource(); // DBMS 연결
	void ExecuteStatementDirect(SQLTCHAR* query); // 쿼리 바로 실행
	void PrepareStatement(SQLTCHAR* query); // 쿼리문 준비 설정
	void ExecuteStatement(); // 준비된 쿼리문 실행
	DBQueryResult FetchResult(); // 결과 Fetch
	void DisconnectDataSource(); // 핸들 release

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

