#include "Sql.h"
#include "MainLog.h"

extern void		PutLogList(char* cMsg);
extern class CMainLog * G_pMainLog;

CSql::CSql() {
	ZeroMemory(cDatabase, sizeof(cDatabase));
	ZeroMemory(cUsername, sizeof(cUsername));
	ZeroMemory(cPassword, sizeof(cPassword));
	ZeroMemory(cAddress, sizeof(cAddress));
}
CSql::~CSql() {

}
void CSql::Message(int iClientH, char* pData, DWORD dwMsgSize) {
	WORD* dwpMsgID;

	//if (G_pWorldLog->m_pClientList[iClientH] == NULL) return;
	dwpMsgID = (WORD*)(pData + DEF_INDEX2_MSGTYPE + 2);
	switch (*dwpMsgID) {
	default: //Insert, Update, Delete (unless defined above)
		//nothing right now. sql is based on WL request and no new features required on ML
		break;

	}
}

bool CSql::GetAccountInfo(int iClientH) {
	char* cp, cQuery[300], cTemp[400];
	int iRet, numfields, iTotalRows;
	DWORD* dwp;
	WORD* wp;
	MYSQL* conn;
	MYSQL_RES* res;
	MYSQL_ROW row;

	ZeroMemory(cQuery, sizeof(cQuery));
	ZeroMemory(cTemp, sizeof(cTemp));

	conn = mysql_init(NULL);
	if (!conn) {
		PutLogList("(!)(SQL) Mysql connection failed 1");
		return FALSE;
	}
	else if (!mysql_real_connect(conn, cAddress, cUsername, cPassword, cDatabase, 0, NULL, 0)) {
		ZeroMemory(cTemp, sizeof(cTemp));
		wsprintf(cTemp, "(!)(SQL) Mysql connection failed 2 (IP): %s, May be bad PW/US", cAddress);
		PutLogList(cTemp);
		mysql_close(conn);
		return false;
	}
	wsprintf(cQuery, "SELECT * FROM account WHERE account = '%s'", G_pMainLog->m_pAccountList[iClientH]->cAccountName);
	if (mysql_query(conn, cQuery) != 0) {
		ZeroMemory(cTemp, sizeof(cTemp));
		wsprintf(cTemp, "(!)(SQL) Mysql Account No Account Found (%s)", G_pMainLog->m_pAccountList[iClientH]->cAccountName);
		PutLogList(cTemp);
		mysql_close(conn);
		return false;
	}
	res = mysql_store_result(conn);
	iTotalRows = mysql_num_rows(res);
	numfields = mysql_num_fields(res);
	if (iTotalRows <= 0) {
		ZeroMemory(cTemp, sizeof(cTemp));
		wsprintf(cTemp, "(!)(SQL) Mysql Account Read Error (%s)", G_pMainLog->m_pAccountList[iClientH]->cAccountName);
		PutLogList(cTemp);
//		mysql_free_result(res);
		mysql_close(conn);
		return false;
	}
	if ((row = mysql_fetch_row(res)) != NULL) {
		try {
			memcpy(G_pMainLog->m_pAccountList[iClientH]->cEmail, row[1], strlen(row[1])); //Email
			G_pMainLog->m_pAccountList[iClientH]->iAccountid = atoi(row[2]); //ID
			G_pMainLog->m_pAccountList[iClientH]->iCredit = atoi(row[4]); //Credit
			if (atoi(row[5]) <= 1) G_pMainLog->m_pAccountList[iClientH]->bEmailConfirm = atoi(row[5]); //Email Active?
		}
		catch (std::exception const& e) {
			wsprintf(cTemp, "(!CRITICAL) GetAccountInfo (SQL) Read rows to memory #1 (%s)", e.what());
			PutLogList(cTemp);
		}

	}
	mysql_close(conn);
	return true;
}