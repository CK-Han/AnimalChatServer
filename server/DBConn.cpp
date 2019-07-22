#include "DBConn.h"

DBConn::DBConn()
{
}


DBConn::~DBConn()
{
	this->DisconnectDataSource();
}

bool DBConn::Initialize()
{
	this->AllocateHandles();
	this->ConnectDataSource();

	return true;
}

bool DBConn::CheckSQLSuccess(SQLRETURN& ret)
{
	bool isSuccess = ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO;
	if (isSuccess == false) {
		SQLGetDiagRec(SQL_HANDLE_DBC, hdbc, ++rec, state, &native, message, sizeof(message), &length);

		std::wcout << state << ", " << rec << ", " << native << ", " << message << std::endl;
	}

	return isSuccess;
}

void DBConn::AllocateHandles()
{
	auto ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
	if (this->CheckSQLSuccess(ret) == false) {
		std::cout << "SQLAllocHandle() ENV error\n";
		return;
	}

	ret = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
	if (this->CheckSQLSuccess(ret) == false) {
		std::cout << "SQLSetEnvAttr() error\n";
		return;
	}

	ret = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
	if (this->CheckSQLSuccess(ret) == false) {
		std::cout << "SQLAllocHandle() DBC error\n";
		return;
	}
}

void DBConn::ConnectDataSource()
{
	auto ret = SQLConnect(hdbc, (SQLWCHAR*)L"TEST_CGHAN", SQL_NTS, (SQLWCHAR*)L"root", SQL_NTS, (SQLWCHAR*)L"doublecg1!", SQL_NTS);
	if (this->CheckSQLSuccess(ret) == false) {
		std::cout << "SQLConnect() error\n";
		return;
	}
}

void DBConn::ExecuteStatementDirect(SQLTCHAR* query)
{
	auto ret = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	if (this->CheckSQLSuccess(ret) == false) {
		std::cout << "SQLAllocHandle() STMT error\n";
		return;
	}

	ret = SQLExecDirect(hstmt, query, SQL_NTS);
	if (this->CheckSQLSuccess(ret) == false) {
		std::cout << "SQLExecDirect() error\n";
		return;
	}
}

void DBConn::PrepareStatement(SQLTCHAR* query)
{
	auto ret = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	if (this->CheckSQLSuccess(ret) == false) {
		std::cout << "SQLAllocHandle() STMT error\n";
		return;
	}

	ret = SQLPrepare(hstmt, query, SQL_NTS);
	if (this->CheckSQLSuccess(ret) == false) {
		std::cout << "SQLPrepare() error\n";
		return;
	}
}

void DBConn::ExecuteStatement()
{
	auto ret = SQLExecute(hstmt);
	if (this->CheckSQLSuccess(ret) == false) {
		std::cout << "SQLExecute() error\n";
		return;
	}
}

DBConn::DBQueryResult DBConn::FetchResult()
{
	// long long rid;
	TCHAR rid[128];
	TCHAR id[128];
	TCHAR password[128];
	TCHAR name[128];
	TCHAR date_add[128];
	TCHAR date_mod[128];
	TCHAR float_test[128];

	SQLLEN a, b, c, d, e, f, g;

	//SQLBindCol(hstmt, 1, SQL_C_UBIGINT, &rid, sizeof(rid), &a);
	SQLBindCol(hstmt, 1, SQL_C_TCHAR, &rid, sizeof(rid), &a);
	SQLBindCol(hstmt, 2, SQL_C_TCHAR, &id, sizeof(id), &b);
	SQLBindCol(hstmt, 3, SQL_C_TCHAR, &password, sizeof(password), &c);
	SQLBindCol(hstmt, 4, SQL_C_TCHAR, &name, sizeof(name), &d);
	SQLBindCol(hstmt, 5, SQL_C_TCHAR, &date_add, sizeof(date_add), &e);
	SQLBindCol(hstmt, 6, SQL_C_TCHAR, &date_mod, sizeof(date_mod), &f);
	SQLBindCol(hstmt, 7, SQL_C_TCHAR, &float_test, sizeof(float_test), &g);

	SQLRETURN ret;
	while ( (ret = SQLFetch(hstmt)) != SQL_NO_DATA ) {
		std::wcout << rid << ", " << id << ", " << name << ", " << password << ", " << date_add << ", " << date_mod << std::endl;
	}

	SQLFreeStmt(hstmt, SQL_UNBIND);

	return DBQueryResult{};
}

void DBConn::DisconnectDataSource()
{
	if (hstmt) {
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
		hstmt = NULL;
	}

	SQLDisconnect(hdbc);
	if (hdbc) {
		SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
		hdbc = NULL;
	}

	if (henv) {
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
		henv = NULL;
	}
}
