#pragma once
#if !defined(AFX_SQL_H__A9554BE2_A96B_11D2_B143_00001C7030A7__INCLUDED_)
#define AFX_SQL_H__A9554BE2_A96B_11D2_B143_00001C7030A7__INCLUDED_

#include <windows.h>
#include "mysql/mysql.h"
#include "NetMessages.h"

class CSql
{
public:
	char cUsername[20];
	char cPassword[64];
	char cDatabase[22];
	char cAddress[16];

	CSql();
	virtual ~CSql();
	void Message(int iClientH, char* pData, DWORD dwMsgSize);
	bool GetAccountInfo(int iClientH);
};
#endif
