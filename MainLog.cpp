#include "MainLog.h"
#include <stdlib.h>
#include <stdio.h>
#include <direct.h>


extern void		PutLogList(char * cMsg);
extern void 	MysqlUser(); //dialogbox
extern char		G_cTxt[500];
extern char		G_cData50000[50000];

BOOL CMainLog::bAccept(class XSocket * pXSock)
{
 register int i, x;
 int sTotalConnection;
 class XSocket * pTmpSock;
 BOOL bFlag;
 sTotalConnection = 0;

	bFlag = FALSE;
	for (i = 1; i < DEF_MAXCLIENTSOCK; i++) { //loop 1
		if (m_pClientList[i] == NULL) { //if client null
			m_pClientList[i] = new class CClient(m_hWnd);
			
			//accept
			pXSock->bAccept(m_pClientList[i]->m_pXSock, WM_ONCLIENTSOCKETEVENT + i); 
			m_pClientList[i]->m_pXSock->bInitBufferSize(DEF_MSGBUFFERSIZE);
			ZeroMemory(m_pClientList[i]->m_cIPaddress, sizeof(m_pClientList[i]->m_cIPaddress));
			m_pClientList[i]->m_pXSock->iGetPeerAddress(m_pClientList[i]->m_cIPaddress);
			
			//cant accept mor then 20 connections from 1 ip
			for (x = 1; x < DEF_MAXCLIENTSOCK; x++)
			if(m_pClientList[x] != NULL)
			if(strcmp(m_pClientList[x]->m_cIPaddress, m_pClientList[i]->m_cIPaddress) == 0) sTotalConnection++;

			if(sTotalConnection > 20) { //delete the client from connection
				delete m_pClientList[i];
				m_pClientList[i] = NULL;
				pTmpSock = new class XSocket(m_hWnd, DEF_SERVERSOCKETBLOCKLIMIT);
				pXSock->bAccept(pTmpSock, NULL); 
				delete pTmpSock;
				return FALSE;
			}
			//wsprintf(G_cTxt,"Client Accepted(%d)", i);
			//PutLogList(G_cTxt);
			return TRUE;
		} //end client == null
	} //end loop 1
	pTmpSock = new class XSocket(m_hWnd, DEF_SERVERSOCKETBLOCKLIMIT);
	pXSock->bAccept(pTmpSock, NULL); 
	delete pTmpSock;
	return FALSE;
}

CMainLog::CMainLog(HWND hWnd)
{
int i;

	for (i = 0; i < DEF_MAXCLIENTSOCK; i++) m_pClientList[i] = NULL;
	for (i = 0; i < DEF_MSGQUEUESIZE; i++) m_pMsgQueue[i] = NULL;
	for (i = 0; i < DEF_MAXGAME; i++) m_pGameList[i] = NULL;
	for (i = 0; i < DEF_MAXMAPS; i++) m_pMapList[i] = NULL;
	for (i = 0; i < DEF_MAXACCOUNTS; i++) m_pAccountList[i] = NULL;
	for (i = 0; i < DEF_MAXCHARACTER; i++) m_pCharList[i] = NULL;

	m_hWnd = hWnd;
	ZeroMemory(m_cMainServerAddress, sizeof(m_cMainServerAddress));
	m_iQueueHead = 0;
	m_iQueueTail = 0;
	m_iMainServerInternalPort = 0;
	m_iValidAccounts = m_iTotalWorld = m_iTotalGame = 0;
	bShutDown = FALSE;
}

CMainLog::~CMainLog(void)
{
	int i;
	for (i = 0; i < DEF_MAXCLIENTSOCK; i++)
		if (m_pClientList[i] != NULL) delete m_pClientList[i];
	for (i = 0; i < DEF_MSGQUEUESIZE; i++) 
		if(m_pMsgQueue[i] != NULL) delete m_pMsgQueue[i];
	for (i = 0; i < DEF_MAXGAME; i++)
		if(m_pGameList[i] != NULL) delete m_pGameList[i];
	for (i = 0; i < DEF_MAXMAPS; i++)
		if(m_pMapList[i] == NULL) delete m_pMapList[i];
	for (i = 0; i < DEF_MAXACCOUNTS; i++)
		if(m_pAccountList[i] == NULL) delete m_pAccountList[i];
	for (i = 0; i < DEF_MAXCHARACTER; i++)
		if(m_pCharList[i] == NULL) delete m_pCharList[i];
}

bool CMainLog::bInit()
{
if (bReadServerConfigFile("HMLServer.cfg") == FALSE) return FALSE;
MysqlUser(); //Username password for sql
	return TRUE;
}

void CMainLog::MsgProcess()
{
 char   * pData=NULL, *cp, cFrom, cKey, cTemp[50];
 DWORD    dwMsgSize, * dwpMsgID;
  WORD   * wpMsgType;
 int     i, iClientH;
 char cDump[1000];
 BOOL bFlag;
 bFlag = FALSE;
 ZeroMemory(cDump, sizeof(cDump));
 ZeroMemory(cTemp, sizeof(cTemp));

	ZeroMemory(m_pMsgBuffer, DEF_MSGBUFFERSIZE+1);
	pData = (char *)m_pMsgBuffer;
	while (bGetMsgQuene(&cFrom, pData, &dwMsgSize, &iClientH, &cKey) == TRUE) {
		dwpMsgID   = (DWORD *)(pData + DEF_INDEX4_MSGID);
		//wsprintf(G_cTxt, "(X) Recieved Message (0x%.8X)", *dwpMsgID);
		//PutLogList(G_cTxt);
		if (dwMsgSize > DEF_MSGBUFFERSIZE) dwMsgSize = DEF_MSGBUFFERSIZE; //marleythe9 fix size;
		switch (*dwpMsgID) {
			
			//----------From Client-------------//
			case MSGID_REQUEST_LOGIN: //client request login
				RequestLogin(iClientH, pData);
			break;
			case MSGID_REQUEST_CREATENEWCHARACTER: //message from client
					ResponseCharacter(iClientH, pData, 0);
			break;
			case MSGID_REQUEST_DELETECHARACTER:
					DeleteCharacter(iClientH, pData, 0);
				break;
			case MSGID_REQUEST_CHANGEPASSWORD:
					ChangePassword(iClientH, pData);
				break;
			case MSGID_REQUEST_ENTERGAME:
					RequestEnterGame(iClientH, pData, 0);
				break;
			//---------From WLserver-------------//
			case MSGID_REGISTER_WORLDSERVERSOCKET: //last message for activation
				wpMsgType  = (WORD *)(pData + DEF_INDEX2_MSGTYPE);
				switch(*wpMsgType) {
					case DEF_MSGTYPE_CONFIRM:
						RegisterSocketWL(iClientH, pData, TRUE);
					break;
					case DEF_MSGTYPE_REJECT:
						RegisterSocketWL(iClientH, pData, FALSE);
					break;
				}
			break;
			case MSGID_WORLDSERVERACTIVATED:
				if(m_pClientList[iClientH]->bIsworld == FALSE) return;
				ActiveateWorld(iClientH);
				break;

			case MSGID_REQUEST_REMOVEGAMESERVER: //remove game server
				if(m_pClientList[iClientH]->bIsActiveWL == FALSE) return;
				wpMsgType  = (WORD *)(pData + DEF_INDEX2_MSGTYPE);
				switch(*wpMsgType) {
				case DEF_MSGTYPE_CONFIRM:
			    RegisterGameServer(iClientH, pData, FALSE);
				break;
				}
				break;

			case MSGID_REGISTER_WORLDSERVER_GAMESERVER: //ad game server
				if(m_pClientList[iClientH]->bIsActiveWL == FALSE) return;
				wpMsgType  = (WORD *)(pData + DEF_INDEX2_MSGTYPE);
				switch(*wpMsgType) {
					case DEF_MSGTYPE_CONFIRM:
					RegisterGameServer(iClientH, pData, TRUE);
					break;
				}
				break;

			case MSGID_RESPONSE_CHARINFOLIST: //get all character info
					if(m_pClientList[iClientH]->bIsActiveWL == FALSE) return;
					TotalChar(iClientH, pData);
				break;

			case MSGID_RESPONSE_CHARACTERLOG:
				if(m_pClientList[iClientH]->bIsActiveWL == FALSE) return;
				wpMsgType  = (WORD *)(pData + DEF_INDEX2_MSGTYPE);
				switch(*wpMsgType) {
				case DEF_LOGRESMSGTYPE_NEWCHARACTERDELETED:
					DeleteCharacter(iClientH, pData, 1);
					break;
				case DEF_LOGRESMSGTYPE_NEWCHARACTERCREATED:
					ResponseCharacter(iClientH, pData, 1);
					break;
				case DEF_LOGRESMSGTYPE_NEWCHARACTERFAILED:
						cp = (char *)(pData + DEF_INDEX2_MSGTYPE + 2);

						memcpy(cTemp, cp, 10);
						cp += 10;

					for (i = 0; i < DEF_MAXACCOUNTS; i++)
						if((m_pAccountList[i] != NULL) && (strcmp(m_pAccountList[i]->cAccountName, cTemp) == 0)) {
							if(m_pClientList[m_pAccountList[i]->iClientH] != NULL)
							SendEventToClient(DEF_LOGRESMSGTYPE_NEWCHARACTERFAILED, DEF_LOGRESMSGTYPE_NEWCHARACTERFAILED, 0, 0, m_pAccountList[i]->iClientH);
					}
					break;
				case DEF_LOGRESMSGTYPE_ALREADYEXISTINGCHARACTER:
						cp = (char *)(pData + DEF_INDEX2_MSGTYPE + 2);

						memcpy(cTemp, cp, 10);
						cp += 10;

					for (i = 0; i < DEF_MAXACCOUNTS; i++)
						if((m_pAccountList[i] != NULL) && (strcmp(m_pAccountList[i]->cAccountName, cTemp) == 0)) {
							if(m_pClientList[m_pAccountList[i]->iClientH] != NULL)
							SendEventToClient(DEF_LOGRESMSGTYPE_ALREADYEXISTINGCHARACTER, DEF_LOGRESMSGTYPE_ALREADYEXISTINGCHARACTER, 0, 0, m_pAccountList[i]->iClientH);
					}
					break;
				}
				break;
			   case MSGID_REQUEST_CLEARACCOUNTSTATUS:
						if(m_pClientList[iClientH]->bIsActiveWL == FALSE) return;
						cp = (char *)(pData + DEF_INDEX2_MSGTYPE + 2);

						memcpy(cTemp, cp, 10);
						cp += 10;

					if(strlen(cTemp) <= 0) return;
					for (i = 0; i < DEF_MAXACCOUNTS; i++)
						if((m_pAccountList[i] != NULL) && (strcmp(m_pAccountList[i]->cAccountName, cTemp) == 0)&&(strcmp(m_pAccountList[i]->cWorldName, m_pClientList[iClientH]->m_cWorldName)==0)) {
							wsprintf(G_cTxt, "(!)Clear Account (%s)", cTemp);
							PutLogList(G_cTxt);
							if(m_pAccountList[i]->bAcctive == TRUE) m_iValidAccounts--;
							m_pAccountList[i]->bAcctive = FALSE;
							DeleteAccount(i);
							//SendEventToWLS(MSGID_REQUEST_SETACCOUNTINITSTATUS, DEF_MSGTYPE_CONFIRM, cTemp, 10, iClientH);	
						}
						ZeroMemory(cTemp, sizeof(cTemp));
				break;

			//--------Gateserver--------//
			   case MSGID_SENDSERVERSHUTDOWNMSG:
						if(m_pClientList[iClientH]->bIsActiveWL == FALSE) return;
						bShutDown = TRUE;
						m_pClientList[iClientH]->bIsShutdown = TRUE;
				   break;
			   case MSGID_REQUEST_ADDREMOVEMAP:
				   if(m_pClientList[iClientH]->bIsActiveWL == FALSE) return;
					AddRemoveMaps(iClientH, pData);
				   break;
			default:
				wsprintf(cDump, "Unknown message received! (0x%.8X)", *dwpMsgID);
				PutLogList(cDump);
				//PutPacketLogData(*dwpMsgID, pData, dwMsgSize);
				break;
		}
		break;
	}
	UpdateScreen();
}

void CMainLog::ActiveateWorld(int iClientH)
{
	int i;
	for (i = 1; i < DEF_MAXCLIENTSOCK; i++) //activate all sockets undert this WLserver
			if((m_pClientList[i] != NULL) && (strcmp(m_pClientList[i]->m_cWorldName, m_pClientList[iClientH]->m_cWorldName) == 0))
				m_pClientList[i]->bIsActiveWL = TRUE; //WLserver Activated

				m_iTotalWorld++;
				PutLogList(" ");
				wsprintf(G_cTxt, "(!) WorldServer(%s) Activated", m_pClientList[iClientH]->m_cWorldName);
				PutLogList(G_cTxt);
}

void CMainLog::AddRemoveMaps(int iClientH, char * pData)
{
	int i, iGame;
	char cMapName[16], cGameServer[11];
	char cmode;
	char * cp;
	cmode = -1;
	ZeroMemory(cMapName, sizeof(cMapName));
	ZeroMemory(cGameServer, sizeof(cGameServer));

	if (m_pClientList[iClientH] == NULL) return;
	cp = (char *)(pData + DEF_INDEX2_MSGTYPE + 2);
	memcpy(cMapName, cp, 15);
	cp += 15;
	memcpy(cGameServer, cp, 11);
	cp += 11;
	cmode = *cp;
	cp++;

	for (i = 0; i < DEF_MAXGAME; i++)
		if((m_pGameList[i] != NULL)&&(memcmp(m_pGameList[i]->m_cGameName, cGameServer, 11) == 0)) iGame = i;

	if(m_pGameList[iGame] == NULL) return;

	if(cmode == 0)
	for (i = 0; i < DEF_MAXMAPS; i++)
		if(m_pMapList[i] == NULL) {
			m_pMapList[i] = new class CMap(iGame, cMapName);
			m_pGameList[iGame]->m_iTotalMaps++;
			wsprintf(G_cTxt, "(!) Map(%s) registration: WS1-%s MapNum(%d)", cMapName, m_pGameList[iGame]->m_cGameName, m_pGameList[iGame]->m_iTotalMaps);
			PutLogList(G_cTxt);
			return;
		}
	if(cmode == 1)
	for (i = 0; i < DEF_MAXMAPS; i++)
		if((m_pMapList[i] != NULL)&&(memcmp(m_pMapList[i]->m_cMapName, cMapName, 15) == 0)) {
			m_pGameList[m_pMapList[i]->iIndex]->m_iTotalMaps--;
			wsprintf(G_cTxt, "(!) Map(%s) removed: WS1-%s MapNum(%d)", cMapName, m_pGameList[m_pMapList[i]->iIndex]->m_cGameName, m_pGameList[m_pMapList[i]->iIndex]->m_iTotalMaps);
			PutLogList(G_cTxt);
			delete m_pMapList[i];
			m_pMapList[i] = NULL;
		}
}

BOOL CMainLog::bGetMsgQuene(char * pFrom, char * pData, DWORD * pMsgSize, int * pIndex, char * pKey)
{

	if (m_pMsgQueue[m_iQueueHead] == NULL) return FALSE;

	m_pMsgQueue[m_iQueueHead]->Get(pFrom, pData, pMsgSize, pIndex, pKey);

	delete m_pMsgQueue[m_iQueueHead];
	m_pMsgQueue[m_iQueueHead] = NULL;

	m_iQueueHead++;
	if (m_iQueueHead >= DEF_MSGQUEUESIZE) m_iQueueHead = 0;

	return TRUE;
}
BOOL CMainLog::bPutMsgQuene(char cFrom, char * pData, DWORD dwMsgSize, int iIndex, char cKey)
{
	if (m_pMsgQueue[m_iQueueTail] != NULL) return FALSE;

	m_pMsgQueue[m_iQueueTail] = new class CMsg;
	if (m_pMsgQueue[m_iQueueTail] == NULL) return FALSE;

	if (m_pMsgQueue[m_iQueueTail]->bPut(cFrom, pData, dwMsgSize, iIndex, cKey) == FALSE) return FALSE;

	m_iQueueTail++;
	if (m_iQueueTail >= DEF_MSGQUEUESIZE) m_iQueueTail = 0;
	
	return TRUE;
}
void CMainLog::OnTimer()
{
DWORD dwTime;
dwTime = timeGetTime();
MsgProcess();
}

//send message to Client
void CMainLog::SendEventToClient(DWORD dwMsgID, WORD wMsgType, char * pData, DWORD dwMsgSize, int iWorldH)
{
 int iRet;
 DWORD * dwp;
 char * cp;
 WORD * wp;
 int m_iCurWorldLogSockIndex;

	if(iWorldH == -1) return;
	else m_iCurWorldLogSockIndex = iWorldH;

	ZeroMemory(G_cData50000, sizeof(G_cData50000));
	
	dwp  = (DWORD *)(G_cData50000 + DEF_INDEX4_MSGID);
	*dwp = dwMsgID;
	wp   = (WORD *)(G_cData50000 + DEF_INDEX2_MSGTYPE);
	*wp  = wMsgType;

	cp = (char *)(G_cData50000 + DEF_INDEX2_MSGTYPE + 2);

	memcpy((char *)cp, pData, dwMsgSize);

	if ((iWorldH != -1)) { //if WLserver registered
		//wsprintf(G_cTxt, "(X) Send Message (%x)", dwMsgID);
		//PutLogList(G_cTxt);
		switch(dwMsgID) {
			case MSGID_RESPONSE_REGISTER_WORLDSERVERSOCKET:
				iRet = m_pClientList[m_iCurWorldLogSockIndex]->m_pXSock->iSendMsg(G_cData50000, dwMsgSize+6, DEF_USE_ENCRYPTION);
				break;
			default:
				iRet = m_pClientList[m_iCurWorldLogSockIndex]->m_pXSock->iSendMsg(G_cData50000, dwMsgSize+6, DEF_USE_ENCRYPTION); //send anyway..
				break;
		}
		switch (iRet) {
		case DEF_XSOCKEVENT_QUENEFULL:
		case DEF_XSOCKEVENT_SOCKETERROR:
		case DEF_XSOCKEVENT_CRITICALERROR:
		case DEF_XSOCKEVENT_SOCKETCLOSED:
			delete m_pClientList[m_iCurWorldLogSockIndex];
			m_pClientList[m_iCurWorldLogSockIndex] = NULL;
			wsprintf(G_cTxt, "(!) Connection Lost (%d) - (%s)", m_iCurWorldLogSockIndex, m_pClientList[m_iCurWorldLogSockIndex]->m_cWorldName);
			PutLogList(G_cTxt);
		}
	}
}

//send message to WLserver
void CMainLog::SendEventToWLS(DWORD dwMsgID, WORD wMsgType, char * pData, DWORD dwMsgSize, int iWorldH)
{
 int iRet;
 DWORD * dwp;
 char * cp;
 WORD * wp;
 int m_iCurWorldLogSockIndex;

	if(iWorldH == -1) return;
	else m_iCurWorldLogSockIndex = iWorldH;

	ZeroMemory(G_cData50000, sizeof(G_cData50000));
	
	dwp  = (DWORD *)(G_cData50000 + DEF_INDEX4_MSGID);
	*dwp = dwMsgID;
	wp   = (WORD *)(G_cData50000 + DEF_INDEX2_MSGTYPE);
	*wp  = wMsgType;

	cp = (char *)(G_cData50000 + DEF_INDEX2_MSGTYPE + 2);

	memcpy((char *)cp, pData, dwMsgSize);

	if ((iWorldH != -1)) { //if WLserver registered
		//wsprintf(G_cTxt, "(X) Send Message (%x)", dwMsgID);
		//PutLogList(G_cTxt);
		switch(dwMsgID) {
			case MSGID_RESPONSE_REGISTER_WORLDSERVERSOCKET:
				iRet = m_pClientList[m_iCurWorldLogSockIndex]->m_pXSock->iSendMsg(G_cData50000, dwMsgSize+6, DEF_USE_ENCRYPTION);
				break;
			default:
				iRet = m_pClientList[m_iCurWorldLogSockIndex]->m_pXSock->iSendMsg(G_cData50000, dwMsgSize+6, DEF_USE_ENCRYPTION); //send anyway..
				break;
		}
		switch (iRet) {
		case DEF_XSOCKEVENT_QUENEFULL:
		case DEF_XSOCKEVENT_SOCKETERROR:
		case DEF_XSOCKEVENT_CRITICALERROR:
		case DEF_XSOCKEVENT_SOCKETCLOSED:
			delete m_pClientList[m_iCurWorldLogSockIndex];
			m_pClientList[m_iCurWorldLogSockIndex] = NULL;
			wsprintf(G_cTxt, "(!) Connection Lost (%d) - (%s)", m_iCurWorldLogSockIndex, m_pClientList[m_iCurWorldLogSockIndex]->m_cWorldName);
			PutLogList(G_cTxt);
		}
	}
}


void CMainLog::OnClientSubLogSocketEvent(UINT message, WPARAM wParam, LPARAM lParam) //From client
{
 register int iClientH, iRet, iTmp, i, iTotalSock;
 char  * pData, cKey;
 DWORD  dwMsgSize;
 iTotalSock = 0;

	iTmp = WM_ONCLIENTSOCKETEVENT;
	iClientH = message - iTmp;
	if (m_pClientList[iClientH] == NULL) return;
	iRet = m_pClientList[iClientH]->m_pXSock->iOnSocketEvent(wParam, lParam);
	switch (iRet) {

	case DEF_XSOCKEVENT_CONNECTIONESTABLISH:

		break;

	case DEF_XSOCKEVENT_READCOMPLETE:
		pData = m_pClientList[iClientH]->m_pXSock->pGetRcvDataPointer(&dwMsgSize, &cKey);
		if (bPutMsgQuene(DEF_MSGFROM_LOGSERVER, pData, dwMsgSize, iClientH, cKey) == FALSE) {
			PutLogList("@@@@@@ CRITICAL ERROR in LOGSERVER MsgQuene!!! @@@@@@");
		}
		break;

	case DEF_XSOCKEVENT_BLOCK:

		break;

	case DEF_XSOCKEVENT_CONFIRMCODENOTMATCH:

		break;

	case DEF_XSOCKEVENT_MSGSIZETOOLARGE:
	case DEF_XSOCKEVENT_SOCKETERROR:
	case DEF_XSOCKEVENT_SOCKETCLOSED:
	case DEF_XSOCKEVENT_QUENEFULL:
		//delete world socket
		if(m_pClientList[iClientH]->bIsworld == TRUE) { // this is world server connected 2828 public port & autherized
			wsprintf(G_cTxt, "(X) World-Server-socket(%d) error!", iClientH);
			PutLogList(G_cTxt);
			 PutLogList(" ");

			 for (i = 1; i < DEF_MAXCLIENTSOCK; i++)
				if((m_pClientList[i] != NULL)&&(memcmp(m_pClientList[i]->m_cWorldName, m_pClientList[iClientH]->m_cWorldName, 30) == 0)) iTotalSock++;
			
			if(iTotalSock <= 1) {
			 wsprintf(G_cTxt, "(X) World-Server(%s) Deleted!", m_pClientList[iClientH]->m_cWorldName);
			 PutLogList(G_cTxt);
			 m_iTotalWorld--;
			}
			 delete m_pClientList[iClientH];
			m_pClientList[iClientH] = NULL;
			 return;
		}
		//if normal client
		DeleteAccount(NULL, m_pClientList[iClientH]->m_cAccountName);
		//DeleteAccount(-1, m_pClientList[iClientH]->m_cAccountName); //delete account name
		delete m_pClientList[iClientH];
		m_pClientList[iClientH] = NULL;
		break;
	}
}

void CMainLog::RegisterSocketWL(int iClientH, char *pData, bool bRegister)
{
char cData[100];
DWORD * dwp;
char *cp;
BOOL bFlag;
int i;

bFlag = FALSE;
ZeroMemory(cData, sizeof(cData));

	if(m_pClientList[iClientH] == NULL) return; //WL socket connected
	 if(bRegister == FALSE) {
		delete m_pClientList[iClientH];
		m_pClientList[iClientH] = NULL;
		wsprintf(G_cTxt, "(!) Socket Register Failed (%d)", iClientH);
		PutLogList(G_cTxt);
		 return;
	 }

	 //if wlserver IP allowed
	for (i = 0; i < DEF_MAXHWLSERVERLIST; i++)
		if (strcmp(m_sWLServerList[i << 4], m_pClientList[iClientH]->m_cIPaddress) == 0) bFlag = TRUE;

		if(bFlag == FALSE) { //not allowed ip
				wsprintf(G_cTxt, "(X)(%d) Not Allowed World-Server(%s) Deleted!", iClientH, m_pClientList[iClientH]->m_cWorldName);
				PutLogList(G_cTxt);
				delete m_pClientList[iClientH];
				m_pClientList[iClientH] = NULL;
				return;
			}
	
	cp = (char *)(pData + DEF_INDEX2_MSGTYPE + 2);

	memcpy(m_pClientList[iClientH]->m_cWorldName, cp, 30);
	cp += 30;

	memcpy(m_pClientList[iClientH]->m_cWorldServerAddress, cp, 16);
	cp += 16;

	dwp = (DWORD *)cp;
	m_pClientList[iClientH]->m_iWorldPort = *dwp;
	cp += 4;

	m_pClientList[iClientH]->bIsworld = TRUE;
	wsprintf(G_cTxt, "(O) Client (%d) is socket to World Server(%s)", iClientH, m_pClientList[iClientH]->m_cWorldName);
	PutLogList(G_cTxt);

	SendEventToWLS(MSGID_RESPONSE_REGISTER_WORLDSERVERSOCKET, DEF_MSGTYPE_CONFIRM, cData, 0, iClientH);
}
void CMainLog::RegisterGameServer(int iClientH, char *pData, bool bRegister)
{
 char cData[100], m_cWorldName[31], m_cGameName[11], m_cGameAddress[16], m_cGameAddressLan[16];
 DWORD * dwp;
 char *cp, *cMapNames;
 int m_iGamePort, m_iTotalMaps, i, x;
 BOOL bwlregister;
 bwlregister = FALSE;
 ZeroMemory(cData, sizeof(cData));
 ZeroMemory(m_cWorldName, sizeof(m_cWorldName));
 ZeroMemory(m_cGameName, sizeof(m_cGameName));
 ZeroMemory(m_cGameAddress, sizeof(m_cGameAddress));
 ZeroMemory(m_cGameAddressLan, sizeof(m_cGameAddressLan));

	if(m_pClientList[iClientH] == NULL) return;
	switch(bRegister) {
	case TRUE:
 	cp = (char *)(pData + DEF_INDEX2_MSGTYPE + 2);

	memcpy(m_cWorldName, cp, 30);
	cp += 30;

	memcpy(m_cGameName, cp, 10);
	cp += 10;

	memcpy(m_cGameAddress, cp, 16);
	cp += 16;

	memcpy(m_cGameAddressLan, cp, 16);
	cp += 16;

	dwp = (DWORD *)cp;
	m_iGamePort = *dwp;
	cp += 4;

	dwp = (DWORD *)cp;
	m_iTotalMaps = *dwp;
	cp += 4;

	for (i = 1; i < DEF_MAXCLIENTSOCK; i++)
	if((m_pClientList[i] != NULL) && (strcmp(m_pClientList[i]->m_cWorldName, m_cWorldName) == 0)) bwlregister = TRUE;
	if(bwlregister == TRUE) {
	wsprintf(G_cTxt, "(OOO) %s-Game Server(%s:%s:%d) registration success", m_cWorldName, m_cGameName, m_cGameAddress, m_iGamePort);
	PutLogList(G_cTxt);
	for (i = 0; i < DEF_MAXGAME; i++)
		if (m_pGameList[i] == NULL) {
			cMapNames = (char *)cp;
			m_pGameList[i] = new class CGame(iClientH, m_cGameName, m_cGameAddress, m_cGameAddressLan, m_iGamePort, m_cWorldName, m_iTotalMaps);
			for (x = 0; x < m_iTotalMaps; x++) {
				if (bClientRegisterMaps(i, cp) == 0) {
				}
				cp += 11;
			}
			m_iTotalGame++;
			wsprintf(G_cTxt, "(OOO) %s-Game Server(%s:%s:%d) registration success", m_cWorldName, m_cGameName, m_cGameAddress, m_iGamePort);
			PutLogList(G_cTxt);
			return;
		} //if game server == nULL
	}// WLserver is registered
	break;

	case FALSE:
	cp = (char *)(pData + DEF_INDEX2_MSGTYPE + 2);

	memcpy(m_cWorldName, cp, 30);
	cp += 30;

	memcpy(m_cGameName, cp, 10);
	cp += 10;
	
	for (i = 0; i < DEF_MAXGAME; i++)
		if ((m_pGameList[i] != NULL)&&(strcmp(m_pGameList[i]->m_cWorldName, m_cWorldName) == 0)&&(strcmp(m_pGameList[i]->m_cGameName, m_cGameName) == 0)) {
		PutLogList(" ");
		wsprintf(G_cTxt, "Delete Game Server(%s) in World Server(%s)", m_pGameList[i]->m_cGameName, m_cWorldName);
		PutLogList(G_cTxt);
		//delete maps on game server
		for (x = 0; x < DEF_MAXMAPS; x++)
		if((m_pMapList[x] != NULL) && (m_pMapList[x]->iIndex == i)) {
		wsprintf(G_cTxt, "Delete Map(%s) in Game Server(%s)", m_pMapList[x]->m_cMapName, m_pGameList[i]->m_cGameName);
		PutLogList(G_cTxt);
		delete m_pMapList[x];
		m_pMapList[x] = NULL;
		}
		delete m_pGameList[i];
		m_pGameList[i] = NULL;
		m_iTotalGame--;
		}
	break;
	}
}
BOOL CMainLog::bClientRegisterMaps(int iClientH, char *pData)
{
 int i;

	for (i = 0; i < DEF_MAXMAPS; i++) {
		if (m_pMapList[i] != NULL) {
			if (memcmp(m_pMapList[i]->m_cMapName, pData, 10) == 0) {
				wsprintf(G_cTxt, "(X) CRITICAL ERROR! Map(%s) duplicated!", pData);
				PutLogList(G_cTxt);
				return FALSE;
			}
		}
	}
	for (i = 0; i < DEF_MAXMAPS; i++) {
		if (m_pMapList[i] == NULL) {
			m_pMapList[i] = new class CMap(iClientH, pData);
			wsprintf(G_cTxt, "(!) Map(%s) registration: WS1-%s MapNum(%d)", pData, m_pGameList[iClientH]->m_cGameName, i);
			PutLogList(G_cTxt);
			return TRUE;			
		}
	}
	wsprintf(G_cTxt, "(X) No more map space left. Map(%s) cannot be added.", pData);
	PutLogList(G_cTxt);
	return FALSE;
}

void CMainLog::SendClientResponse(int iMessage, int iClientH)
{
	char cData[200];
	ZeroMemory(cData, sizeof(cData));
	switch(iMessage) {
		case 1:
			SendEventToClient(DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, cData, 0, iClientH);
			break;
		case 2:
			SendEventToClient(DEF_LOGRESMSGTYPE_PASSWORDMISMATCH, DEF_LOGRESMSGTYPE_PASSWORDMISMATCH, cData, 0, iClientH);
			break;
		case 3:
			SendEventToClient(DEF_LOGRESMSGTYPE_NOTEXISTINGWORLDSERVER, DEF_LOGRESMSGTYPE_NOTEXISTINGWORLDSERVER, cData, 0, iClientH);
			break;
		case 4: //Not Active Account
			SendEventToClient(DEF_LOGRESMSGTYPE_EMAILNOTACTIVE, DEF_LOGRESMSGTYPE_EMAILNOTACTIVE, cData, 0, iClientH);
			break;
	}
}

void CMainLog::RequestLogin(int iClientH, char *pData)
{
 int  iMessage, iAccount, i;
 char cData[200], cTotalChar;
 DWORD *dwp;
 SYSTEMTIME SysTime;
 bool bPasswordCorrect = false;
 std::string sMD5;
 char *cp, *cp2, cAccountName[11], cAccountPass[11], cWorldName[30], cTemp[50];
 ZeroMemory(cAccountName, sizeof(cAccountName));
 ZeroMemory(cAccountPass, sizeof(cAccountPass));
 ZeroMemory(cWorldName, sizeof(cWorldName));
 ZeroMemory(cTemp, sizeof(cTemp));
 ZeroMemory(cData, sizeof(cData));
 cTotalChar = 0;
 iAccount = -1;

		if(m_pClientList[iClientH] == NULL) return;
		//if trouble transfer tcp back to data.
		try {
			cp = (char*)(pData + DEF_INDEX2_MSGTYPE + 2);

			memcpy(cAccountName, cp, 10);
			cp += 10;
			memcpy(cAccountPass, cp, 10);
			cp += 10;
			memcpy(cWorldName, cp, 30);
			cp += 30;

		}
		catch (std::exception const& e) {
			wsprintf(G_cTxt, "(!CRITICAL) Crash error in RequestLogin #1 (%s)", e.what());
			PutLogList(G_cTxt);
		}


		iMessage = GetAccountInfo(iClientH, cAccountName, cAccountPass, cWorldName, &iAccount);
		if(m_pAccountList[iAccount] == NULL) iMessage = 1;
		if(iMessage > 0) { //error messages
		 SendClientResponse(iMessage, iClientH);
		 return;
		}
		//Get SQLData
		m_pSql.GetAccountInfo(iAccount);
		//if Account not Active
		if (m_pAccountList[iAccount]->bEmailConfirm == false) {
			SendClientResponse(4, iClientH);
			return;
		}
#ifdef DEF_MD5PASS
		sMD5 = m_pMD5.getHashFromString(cAccountPass, DEF_MD5SALT);
		if (sMD5 == m_pAccountList[iAccount]->cPassword) bPasswordCorrect = true;
#else
		memcpy(cTemp, cAccountPass, 11);
		if (strcmp(m_pAccountList[iAccount]->cPassword, cTemp) == 0) bPasswordCorrect = true;
#endif
		if (bPasswordCorrect == false) {
				wsprintf(G_cTxt, "(!) Account(%s) Password Wrong(%s)(%s)", cAccountName, m_pAccountList[iAccount]->cPassword, cTemp);
				PutLogList(G_cTxt);
				iMessage = 2;
				SendClientResponse(iMessage, iClientH);
				return;
		}
		ZeroMemory(cTemp, sizeof(cTemp));

		GetLocalTime(&SysTime);
		m_pAccountList[iAccount]->dAccountID = (int)(SysTime.wYear + SysTime.wMonth + SysTime.wDay + SysTime.wHour + SysTime.wMinute + timeGetTime());

		for (i = 0; i < DEF_MAXCHARACTER; i++)
			if((m_pCharList[i] != NULL) && (m_pCharList[i]->iTracker == iAccount)) cTotalChar++;

		//wsprintf(G_cTxt, "TestTotal(%d)", cTotalChar);
		//PutLogList(G_cTxt);

		cp2 = (char *)(cData);
		memcpy(cp2, cAccountName, 10);
		cp2 += 10;

		dwp = (DWORD *)cp2;
		*dwp = m_pAccountList[iAccount]->dAccountID;
		cp2 += 4;

		cp2 += 17;

		*cp2 = cTotalChar;
		cp2 ++;

		dwp = (DWORD *)cp2; ///valid time
		*dwp = 11758870; // is number of valid time
		cp2 += 4;

		dwp = (DWORD *)cp2; //ip
		*dwp = 11758874; // number to check valid time....
		cp2 += 4;

		for (i = 0; i < DEF_MAXCHARACTER; i++)
			if((m_pCharList[i] != NULL) && (m_pCharList[i]->iTracker == iAccount)) {
			memcpy(cp2, m_pCharList[i]->cCharacterName, 10);
			cp2 += 10;
		}

		for (i = 0; i < DEF_MAXCLIENTSOCK; i++)
			if((m_pClientList[i] != NULL) && (strcmp(m_pClientList[i]->m_cWorldName, cWorldName) == 0)&& (m_pClientList[i]->bIsActiveWL == TRUE)) {
			SendEventToWLS(MSGID_REQUEST_CHARINFOLIST, DEF_MSGTYPE_CONFIRM, cData, 49+(cTotalChar*10), i);
			break;
			}
}

void CMainLog::TotalChar(int iClientH, char *pData)
{
 char *cp, *cp2, cAccountName[11], cData[2000], cTotalChar; //cp2 out going message
 DWORD *dwp, dAccountid, dMsg, dMsgType;
 int i, iAccount, iTracker;
 short iUperVersion, iLowerVersion;
 ZeroMemory(cAccountName, sizeof(cAccountName));
 ZeroMemory(cData, sizeof(cData));
 iUperVersion = DEF_UPERVERSION;
 iLowerVersion = DEF_LOWERVERSION;
 cTotalChar = 0;
 iAccount = iTracker = dAccountid = 0;
 if(m_pClientList[iClientH] == NULL) return;

	   cp = (char *)(pData + DEF_INDEX2_MSGTYPE + 2); //incomeing message

	   memcpy(cAccountName, cp, 10);
	   cp += 10;

	   dwp = (DWORD *)cp;
	   dAccountid = *dwp;
	   cp += 4;

		dwp = (DWORD *)cp;
		dMsg = *dwp;
		cp += 4;

		dwp = (DWORD *)cp;
		dMsgType = *dwp;
		cp += 2;

		switch(dMsg) { //relay message from WL to Client
			
		case MSGID_RESPONSE_CHARACTERLOG:
			switch(dMsgType) {
			case DEF_MSGTYPE_CONFIRM:
				for (i = 0; i < DEF_MAXACCOUNTS; i++)
					  if((m_pAccountList[i] != NULL) && (strcmp(m_pAccountList[i]->cAccountName, cAccountName) == 0)) {
					   iTracker = m_pAccountList[i]->iClientH;
					   iAccount= i;
					  }
					   if((m_pAccountList[iAccount] == NULL)||(m_pClientList[iTracker] == NULL)) return;
					   //m_pAccountList[iAccount]->iClientH = NULL; //set iclient to null becouse this is only to be sure he didnt dc on logon
					   if(m_pAccountList[iAccount]->dAccountID != dAccountid) return;

				for (i = 0; i < DEF_MAXCHARACTER; i++)
					if((m_pCharList[i] != NULL) && (m_pCharList[i]->iTracker == iAccount)) cTotalChar++;

				cp2 = (char *)(cData); //outgoing messag

				dwp = (DWORD *)cp2;
				*dwp = iUperVersion;
				cp2 += 2;

				dwp = (DWORD *)cp2;
				*dwp = iLowerVersion;
				cp2 += 2;

				//used for account status
				cp2++;

				dwp = (DWORD *)cp2;
				*dwp = m_pAccountList[iAccount]->m_iAccntYear;
				cp2 += 2;

				dwp = (DWORD *)cp2;
				*dwp = m_pAccountList[iAccount]->m_iAccntMonth;
				cp2 += 2;

				dwp = (DWORD *)cp2;
				*dwp = m_pAccountList[iAccount]->m_iAccntDay;
				cp2 += 2;

				dwp = (DWORD *)cp2;
				*dwp = m_pAccountList[iAccount]->m_iPassYear;
				cp2 += 2;

				dwp = (DWORD *)cp2;
				*dwp = m_pAccountList[iAccount]->m_iPassMonth;
				cp2 += 2;

				dwp = (DWORD *)cp2;
				*dwp = m_pAccountList[iAccount]->m_iPassDay;
				cp2 += 2;

			     *cp2 = cTotalChar;
			     cp2++;
				cp = (char *)(pData + 44); //incomeing
				memcpy(cp2, cp, 8+(65*cTotalChar)); // get message between total*
				cp2 += 8+(65*cTotalChar);

				SendEventToClient(DEF_LOGRESMSGTYPE_CONFIRM, DEF_LOGRESMSGTYPE_CONFIRM, cData, 26+(65*cTotalChar), iTracker);
			break;
			}
			break;
		}
}
void CMainLog::ResponseCharacter(int iClientH, char *pData, char cMode)
{
 char *cp, *cp2, cAccountName[11], cAccountPass[11], cWorldName[30], cData[2000], cTotalChar; //cp2 out going message
 DWORD *dwp, dwAccountid;
 int i, iMessage, iAccount, iTracker;
 char cTemp[50];
 SYSTEMTIME SysTime;
 bool bPasswordCorrect = false;
 std::string sMD5;
  ZeroMemory(cAccountName, sizeof(cAccountName));
   ZeroMemory(cAccountPass, sizeof(cAccountPass));
    ZeroMemory(cWorldName, sizeof(cWorldName));
	ZeroMemory(cTemp, sizeof(cTemp));
	ZeroMemory(cData, sizeof(cData));
	cTotalChar = 0;
	iTracker = -1;
	iAccount = -1;

 	if(m_pClientList[iClientH] == NULL) return;

	switch(cMode) {
		case 0: //message from client to WL
		 cp = (char *)(pData + DEF_INDEX2_MSGTYPE + 2 + 10); //incomeing message
		
		 memcpy(cAccountName, cp, 10);
		 cp += 10;
		 memcpy(cAccountPass, cp, 10);
		 cp += 10;
		 memcpy(cWorldName, cp, 30);
		 cp += 30;

		iMessage = GetAccountInfo(iClientH, cAccountName, cAccountPass, cWorldName, &iAccount);

		if(iMessage > 0) {
		SendEventToClient(DEF_LOGRESMSGTYPE_NEWCHARACTERFAILED, DEF_LOGRESMSGTYPE_NEWCHARACTERFAILED, 0, 0, iClientH);
		return;
		}

#ifdef DEF_MD5PASS
		sMD5 = m_pMD5.getHashFromString(cAccountPass, DEF_MD5SALT);
		if (sMD5 == m_pAccountList[iAccount]->cPassword) bPasswordCorrect = true;
#else
		memcpy(cTemp, cAccountPass, 11);
		if (strcmp(m_pAccountList[iAccount]->cPassword, cTemp) == 0) bPasswordCorrect = true;
#endif
		if (bPasswordCorrect == false) {
		SendEventToClient(DEF_LOGRESMSGTYPE_NEWCHARACTERFAILED, DEF_LOGRESMSGTYPE_NEWCHARACTERFAILED, 0, 0, iClientH);
		return;
		}

		for (i = 0; i < DEF_MAXCHARACTER; i++)
		if((m_pCharList[i] != NULL) && (m_pCharList[i]->iTracker == iAccount)) cTotalChar++;

		GetLocalTime(&SysTime);
		m_pAccountList[iAccount]->dAccountID = (int)(SysTime.wYear + SysTime.wMonth + SysTime.wDay + SysTime.wHour + SysTime.wMinute + timeGetTime()); 

		cp2 = (char *)(cData); //outgoing messag
		cp = (char *)(pData + 6); //incomeing
		memcpy(cp2, cp, 71); // get message between total*
		cp2 += 71;

		dwp = (DWORD *)cp2;
		*dwp = m_pAccountList[iAccount]->dAccountID;
	    cp2 += 4;

	    *cp2 = cTotalChar;
	    cp2++;

		for (i = 0; i < DEF_MAXCHARACTER; i++)
			if((m_pCharList[i] != NULL) && (m_pCharList[i]->iTracker == iAccount)) {
			memcpy(cp2, m_pCharList[i]->cCharacterName, 11);
			cp2 += 11;
		}

		for (i = 0; i < DEF_MAXCLIENTSOCK; i++) //send to a working WLserver socket
			if((m_pClientList[i] != NULL) && (strcmp(m_pClientList[i]->m_cWorldName, cWorldName) == 0)&& (m_pClientList[i]->bIsActiveWL == TRUE)) {
			SendEventToWLS(MSGID_REQUEST_CREATENEWCHARACTER, DEF_MSGTYPE_CONFIRM, cData, 76+(cTotalChar * 11), i);
			break;
		}
		break;
		case 1: //message from WL to Client
			cp = (char *)(pData + DEF_INDEX2_MSGTYPE + 2); //incomeing message
			
			memcpy(cAccountName, cp, 10);
			cp += 10;

		    dwp = (DWORD *)cp;
			dwAccountid = *dwp;
			cp += 4;

			for (i = 0; i < DEF_MAXACCOUNTS; i++)
				if((m_pAccountList[i] != NULL) && (strcmp(m_pAccountList[i]->cAccountName, cAccountName) == 0)&&(m_pClientList[m_pAccountList[i]->iClientH] != NULL)) {
				  iTracker = m_pAccountList[i]->iClientH;
				  iAccount = i;
				}

				if(m_pAccountList[iAccount] == NULL) {
					SendEventToClient(DEF_LOGRESMSGTYPE_NEWCHARACTERFAILED, DEF_LOGRESMSGTYPE_NEWCHARACTERFAILED, 0, 0, iTracker);
					PutLogList("Not exist account");
					return;
				}
				  //m_pAccountList[i]->iClientH = NULL; //set iclient to null becouse this is only to be sure he didnt dc on logon
				if(m_pAccountList[iAccount]->dAccountID != dwAccountid) {
					SendEventToClient(DEF_LOGRESMSGTYPE_NEWCHARACTERFAILED, DEF_LOGRESMSGTYPE_NEWCHARACTERFAILED, 0, 0, iTracker);
					PutLogList("AccountID Wrong");
					return;
				}

			cp = (char *)(pData + 26); //incomeing
			memcpy(cAccountPass, cp, 10); //new char name
			cp += 10;
			cTotalChar = (char)*cp;

			cp = (char *)(pData + 26); //incomeing
			cp2 = (char *)(cData); //outgoing messag
			memcpy(cp2, cp, 11+(cTotalChar*65)); // get message between total*
			cp2 += 11+(cTotalChar*65);

			if(SaveAccountInfo(iAccount, NULL, cAccountPass, m_pAccountList[iAccount]->cWorldName, 1, iTracker) == FALSE) {

			}

			SendEventToClient(DEF_LOGRESMSGTYPE_NEWCHARACTERCREATED, DEF_LOGRESMSGTYPE_NEWCHARACTERCREATED, cData, 11+(cTotalChar*65), iTracker);
			break;
	}
}
void CMainLog::DeleteCharacter(int iClientH, char *pData, char cMode)
{
 char *cp, *cp2, cAccountName[11], cAccountPass[11], cWorldName[30], cData[500], cTotalChar, cCharName[11]; //cp2 out going message
 DWORD *dwp, dwAccountid;
 int i, iMessage, iAccount, iTracker;
 char cTemp[50];
 SYSTEMTIME SysTime;
 bool bPasswordCorrect = false;
 std::string sMD5;
  ZeroMemory(cAccountName, sizeof(cAccountName));
   ZeroMemory(cAccountPass, sizeof(cAccountPass));
    ZeroMemory(cWorldName, sizeof(cWorldName));
	ZeroMemory(cCharName, sizeof(cCharName));
	ZeroMemory(cTemp, sizeof(cTemp));
	ZeroMemory(cData, sizeof(cData));
	iAccount = -1;
	iTracker = -1;
	cTotalChar = 0;

	if(m_pClientList[iClientH] == NULL) return;

	switch(cMode) {
		case 0: //message from client to WL
		cp = (char *)(pData + DEF_INDEX2_MSGTYPE + 2); //incomeing message
		
		memcpy(cCharName, cp, 10);
		cp += 10;
		memcpy(cAccountName, cp, 10);
		cp += 10;
		memcpy(cAccountPass, cp, 10);
		cp += 10;
		memcpy(cWorldName, cp, 30);
		cp += 30;

		iMessage = GetAccountInfo(iClientH, cAccountName, cAccountPass, cWorldName, &iAccount);
		if(iMessage > 0) {
		SendEventToClient(DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, 0, 0, iClientH);
		return;
		}
#ifdef DEF_MD5PASS
		sMD5 = m_pMD5.getHashFromString(cAccountPass, DEF_MD5SALT);
		if (sMD5 == m_pAccountList[iAccount]->cPassword) bPasswordCorrect = true;
#else
		memcpy(cTemp, cAccountPass, 11);
		if (strcmp(m_pAccountList[iAccount]->cPassword, cTemp) == 0) bPasswordCorrect = true;
#endif
		if (bPasswordCorrect == false) {
		SendEventToClient(DEF_LOGRESMSGTYPE_PASSWORDMISMATCH, DEF_LOGRESMSGTYPE_PASSWORDMISMATCH, 0, 0, iClientH);
		return;
		}
		//delete charcter in account file
		if(SaveAccountInfo(iAccount, NULL, m_pAccountList[iAccount]->cWorldName, cCharName, 3, iClientH) == FALSE) {
			SendEventToClient(DEF_LOGRESMSGTYPE_NOTEXISTINGCHARACTER, DEF_LOGRESMSGTYPE_NOTEXISTINGCHARACTER, 0, 0, iClientH);
			return;
		}
		for (i = 0; i < DEF_MAXCHARACTER; i++)
			if((m_pCharList[i] != NULL) && (m_pCharList[i]->iTracker == iAccount)) {
				if(memcmp(m_pCharList[i]->cCharacterName, cCharName, strlen(cCharName)) == 0) {
				delete m_pCharList[i];
				m_pCharList[i] = NULL;
				}
			}
			for (i = 0; i < DEF_MAXCHARACTER; i++)
			if((m_pCharList[i] != NULL) && (m_pCharList[i]->iTracker == iAccount))	cTotalChar++;

		GetLocalTime(&SysTime);
		m_pAccountList[iAccount]->dAccountID = (int)(SysTime.wYear + SysTime.wMonth + SysTime.wDay + SysTime.wHour + SysTime.wMinute + timeGetTime()); 

		cp2 = (char *)(cData); //outgoing messag

		memcpy(cp2, cAccountName, 10);
		cp2 += 10;

		dwp = (DWORD *)cp2;
		*dwp = m_pAccountList[iAccount]->dAccountID;
	    cp2 += 4;

		memcpy(cp2, cCharName, 10);
		cp2 += 10;

	    *cp2 = cTotalChar;
	    cp2++;

		for (i = 0; i < DEF_MAXCHARACTER; i++)
			if((m_pCharList[i] != NULL) && (m_pCharList[i]->iTracker == iAccount)) {
			memcpy(cp2, m_pCharList[i]->cCharacterName, 11);
			cp2 += 11;
		}

		for (i = 0; i < DEF_MAXCLIENTSOCK; i++)
			if((m_pClientList[i] != NULL) && (strcmp(m_pClientList[i]->m_cWorldName, cWorldName) == 0)&& (m_pClientList[i]->bIsActiveWL == TRUE)) {
			SendEventToWLS(MSGID_REQUEST_DELETECHARACTER, DEF_MSGTYPE_CONFIRM, cData, 25+(cTotalChar*11), i);
			break;
		}
		break;
		case 1: //message from WL to client
		cp = (char *)(pData + DEF_INDEX2_MSGTYPE + 2); //incomeing message

			memcpy(cAccountName, cp, 10);
			cp += 10;

		    dwp = (DWORD *)cp;
			dwAccountid = *dwp;
			cp += 4;

			for (i = 0; i < DEF_MAXACCOUNTS; i++)
				if((m_pAccountList[i] != NULL) && (strcmp(m_pAccountList[i]->cAccountName, cAccountName) == 0)&&(m_pClientList[m_pAccountList[i]->iClientH] != NULL)) {
				  iTracker = m_pAccountList[i]->iClientH;
				  iAccount = i;
				}

				if(m_pClientList[iTracker] == NULL) return;
				if(m_pAccountList[iAccount] == NULL) {
					SendEventToClient(DEF_LOGRESMSGTYPE_NOTEXISTINGCHARACTER, DEF_LOGRESMSGTYPE_NOTEXISTINGCHARACTER, 0, 0, iTracker);
					return;
				}
				  //m_pAccountList[i]->iClientH = NULL; //set iclient to null becouse this is only to be sure he didnt dc on logon
				if(m_pAccountList[iAccount]->dAccountID != dwAccountid) {
					SendEventToClient(DEF_LOGRESMSGTYPE_NOTEXISTINGCHARACTER, DEF_LOGRESMSGTYPE_NOTEXISTINGCHARACTER, 0, 0, iTracker);
					return;
				}
			cp = (char *)(pData + 27); //incomeing
			cTotalChar = *cp;

			cp = (char *)(pData + 26); //incomeing
			cp2 = (char *)(cData); //outgoing messag
			memcpy(cp2, cp, 2+(cTotalChar*65)); // get message between total*
			cp2 += 2+(cTotalChar*65);

			SendEventToClient(DEF_LOGRESMSGTYPE_CHARACTERDELETED, DEF_LOGRESMSGTYPE_CHARACTERDELETED, cData, 2+(cTotalChar*65), iTracker);

		break;
	}
}
void CMainLog::ChangePassword(int iClientH, char *pData)
{
char *cp, cAccountName[11], cAccountPass[11], cNewPass[11], cCheckNewPass[11], cTemp[50];
int iAccount, iMessage;
bool bPasswordCorrect = false;
std::string sMD5;
  ZeroMemory(cAccountName, sizeof(cAccountName));
   ZeroMemory(cAccountPass, sizeof(cAccountPass));
    ZeroMemory(cNewPass, sizeof(cNewPass));
	ZeroMemory(cCheckNewPass, sizeof(cCheckNewPass));
	ZeroMemory(cTemp, sizeof(cTemp));
	iAccount = -1;
	iMessage = 0;

	if(m_pClientList[iClientH] == NULL) return;


		cp = (char *)(pData + DEF_INDEX2_MSGTYPE + 2); //incomeing message
		
		memcpy(cAccountName, cp, 10);
		cp += 10;
		memcpy(cAccountPass, cp, 10);
		cp += 10;
		memcpy(cNewPass, cp, 10);
		cp += 10;
		memcpy(cCheckNewPass, cp, 10);
		cp += 10;

		iMessage = GetAccountInfo(iClientH, cAccountName, cAccountPass, "WS1", &iAccount, 1);

		wsprintf(G_cTxt, "(!)Request Pass Change (%s)(%s) to (%s)", cAccountName, cAccountPass, cNewPass);
		PutLogList(G_cTxt);

		if((iMessage > 0) || (iAccount <= -1)) {
		SendEventToClient(DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, 0, 0, iClientH);
		return;
		}
#ifdef DEF_MD5PASS
		sMD5 = m_pMD5.getHashFromString(cAccountPass, DEF_MD5SALT);
		if (sMD5 == m_pAccountList[iAccount]->cPassword) bPasswordCorrect = true;
#else
		memcpy(cTemp, cAccountPass, 11);
		if (strcmp(m_pAccountList[iAccount]->cPassword, cTemp) == 0) bPasswordCorrect = true;
#endif
		if (bPasswordCorrect == false) {
		SendEventToClient(DEF_LOGRESMSGTYPE_PASSWORDMISMATCH, DEF_LOGRESMSGTYPE_PASSWORDMISMATCH, 0, 0, iClientH);
		return;
		}
		if(strcmp(cNewPass, cCheckNewPass) != 0) {
		SendEventToClient(DEF_LOGRESMSGTYPE_PASSWORDCHANGEFAIL, DEF_LOGRESMSGTYPE_PASSWORDCHANGEFAIL, 0, 0, iClientH);
		return;
		}
		//change password in account file
		if (SaveAccountInfo(iAccount, NULL, cNewPass, NULL, 2, iClientH) == FALSE) {
				SendEventToClient(DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, 0, 0, iClientH);
				return;
		}
		else {
			SendEventToClient(DEF_LOGRESMSGTYPE_PASSWORDCHANGESUCCESS, DEF_LOGRESMSGTYPE_PASSWORDCHANGESUCCESS, 0, 0, iClientH);
			return;
		}

}
void CMainLog::RequestEnterGame(int iClientH, char *pData, char cMode)
{
 char *cp, *cp2, cData[154], cData2[100], cAccountName[11], cAccountPass[11], cWorldName[30], m_cMapName[31], cCharName[15], G_cCmdLineTokenA[120];
 int  i, x, m_iLevel, iMessage, iAccount, iGameServer;
 char cTemp[50];
 DWORD *dwp;
 WORD * wpMsgType;
 BOOL bMapOnline, bAccountUse;
 bool bPasswordCorrect = false;
 std::string sMD5 = "";
 bMapOnline = FALSE;
 bAccountUse = FALSE;
 ZeroMemory(cAccountName, sizeof(cAccountName));
 ZeroMemory(cAccountPass, sizeof(cAccountPass));
 ZeroMemory(cWorldName, sizeof(cWorldName));
 ZeroMemory(m_cMapName, sizeof(m_cMapName));
 ZeroMemory(cTemp, sizeof(cTemp));
 ZeroMemory(cData, sizeof(cData));
 ZeroMemory(cData2, sizeof(cData2));
 ZeroMemory(G_cCmdLineTokenA, sizeof(G_cCmdLineTokenA));


		//wsprintf(G_cTxt, "(!)RequestEnterGame");
		//PutLogList(G_cTxt);

		if(m_pClientList[iClientH] == NULL) return;
		wpMsgType  = (WORD *)(pData + DEF_INDEX2_MSGTYPE);

		cp = (char *)(pData + DEF_INDEX2_MSGTYPE + 2);

		memcpy(cCharName, cp, 10);
		cp += 10;

		memcpy(m_cMapName, cp, 10);
		cp += 10;

		memcpy(cAccountName, cp, 10);
		cp += 10;

		memcpy(cAccountPass, cp, 10);
		cp += 10;

		dwp = (DWORD *)cp;
		m_iLevel = *dwp;
		cp += 4;

        memcpy(cWorldName, cp, 30);
		cp += 30;

		memcpy(cp, G_cCmdLineTokenA, 120);
		cp += 120;
		
		for (i = 0; i < DEF_MAXACCOUNTS; i++) //account is active and already playing
			if((m_pAccountList[i] != NULL)&& (strcmp(m_pAccountList[i]->cAccountName, cAccountName) == 0)
				&&(strcmp(m_pAccountList[i]->cWorldName, cWorldName) == 0)&&(m_pAccountList[i]->bAcctive == TRUE)) {
				cp2 = (char *)(cData); //
				*cp2 = 1;
				SendEventToClient(DEF_ENTERGAMERESTYPE_PLAYING, DEF_ENTERGAMERESTYPE_PLAYING, cData, 1, iClientH);
				return;
			}
		iMessage = GetAccountInfo(iClientH, cAccountName, cAccountPass, cWorldName, &iAccount); //install account

		if(iMessage > 0) {
		 cp2 = (char *)(cData); //outgoing messag - to WLserv
		 *cp2 = 4;
		 SendEventToClient(DEF_ENTERGAMERESTYPE_REJECT, DEF_ENTERGAMERESTYPE_REJECT, cData, 1, iClientH);
		 PutLogList("Install account failed");
		 return;
		}

		if(m_pAccountList[iAccount] == NULL) {
			cp2 = (char *)(cData); //
			*cp2 = 1;
			SendEventToClient(DEF_ENTERGAMERESTYPE_PLAYING, DEF_ENTERGAMERESTYPE_PLAYING, cData, 1, iClientH);
			return;
		}

		for (i = 0; i < DEF_MAXCLIENTSOCK; i++)
			if((m_pClientList[i] != NULL) && (strcmp(m_pClientList[i]->m_cWorldName, m_pAccountList[iAccount]->cWorldName) == 0)&& (m_pClientList[i]->bIsActiveWL == TRUE)) {
				if(*wpMsgType == DEF_ENTERGAMEMSGTYPE_NOENTER_FORCEDISCONN) //tell WLserver to force dc client.
				 SendEventToWLS(MSGID_REQUEST_FORCEDISCONECTACCOUNT, DEF_MSGTYPE_CONFIRM, cAccountName, 10, i);
				if(m_pClientList[i]->bIsShutdown == TRUE) {
				 cp2 = (char *)(cData); //outgoing messag - to WLserv
				 *cp2 = 5;
				 SendEventToClient(DEF_ENTERGAMERESTYPE_REJECT, DEF_ENTERGAMERESTYPE_REJECT, cData, 1, iClientH);
				 return;
				}
			break;
		}
			
		if(bShutDown == TRUE) {

		}

		//worldserver Decline messages
		//1 - Char Trial level is made
		//2 - ip groop expired
		//3 - cannot connect to gameserver - isnt avalible
		//4 - cannot connect to gameserver - Character data differs
		//5 - cannot connect to gameserver - offline
		//6 - max users on server
		//7 - world server full
		/*iMessage = GetAccountInfo(iClientH, cAccountName, cAccountPass, cWorldName, &iAccount);
		if(iMessage > 0) {
		cp2 = (char *)(cData); //outgoing messag - to WLserv
		*cp2 = 4;
		SendEventToClient(DEF_ENTERGAMERESTYPE_REJECT, DEF_ENTERGAMERESTYPE_REJECT, cData, 1, iClientH);
		return;
		}*/
#ifdef DEF_MD5PASS
		sMD5 = m_pMD5.getHashFromString(cAccountPass, DEF_MD5SALT);
		if (sMD5 == m_pAccountList[iAccount]->cPassword) bPasswordCorrect = true;
#else
		memcpy(cTemp, cAccountPass, 11);
		if (strcmp(m_pAccountList[iAccount]->cPassword, cTemp) == 0) bPasswordCorrect = true;
#endif
		if (bPasswordCorrect == false) {
		cp2 = (char *)(cData); //outgoing messag - to WLserver
		*cp2 = 4;
		SendEventToClient(DEF_ENTERGAMERESTYPE_REJECT, DEF_ENTERGAMERESTYPE_REJECT, cData, 1, iClientH);
		return;
		}
		for (i = 0; i < DEF_MAXGAME; i++)
			if((m_pGameList[i] != NULL)&& (strcmp(m_pGameList[i]->m_cWorldName, m_pAccountList[iAccount]->cWorldName) == 0)) {
			for (x = 0; x < DEF_MAXMAPS; x++)
				if((m_pMapList[x] != NULL)&&(m_pMapList[x]->iIndex == i)&& (strcmp(m_pMapList[x]->m_cMapName, m_cMapName) == 0)) {
					iGameServer = i;
					bMapOnline = TRUE;
				}
			}

			if(bMapOnline == FALSE){
			cp2 = (char *)(cData); //outgoing messag - to WLserver
			*cp2 = 3;
			SendEventToClient(DEF_ENTERGAMERESTYPE_REJECT, DEF_ENTERGAMERESTYPE_REJECT, cData, 1, iClientH);
			wsprintf(G_cTxt, "####(!)GameServer(%s-%s) for %s is not registered.####", cWorldName, m_cMapName, cAccountName);
			PutLogList(G_cTxt);
			return;
			}

		cp2 = (char *)(cData); //outgoing messag - to WLserver

		memcpy(cp2, cAccountName, 10);
		cp2 += 10;

		if (sMD5 == "") {
			memcpy(cp2, cAccountPass, 10); //this needs encryption setup
			cp2 += 10;
		}
		else {
			memcpy(cp2, sMD5.c_str(), 64); //this needs encryption setup
			cp2 += 64;
		}

		dwp = (DWORD *)cp2;
		*dwp = m_iLevel;
		cp2 += 4;

		for (i = 0; i < DEF_MAXCLIENTSOCK; i++)
			if((m_pClientList[i] != NULL) && (strcmp(m_pClientList[i]->m_cWorldName, cWorldName) == 0)&& (m_pClientList[i]->bIsActiveWL == TRUE)) {
			if(sMD5 == "") SendEventToWLS(MSGID_REQUEST_SETACCOUNTINITSTATUS, DEF_MSGTYPE_CONFIRM, cData, 24, i);
			else SendEventToWLS(MSGID_REQUEST_SETACCOUNTINITSTATUS, DEF_MSGTYPE_CONFIRM, cData, 24 + 54, i);
			break;
		}
		cp2 = (char *)(cData2); //outgoing messag - to Client
		
		//for lan connections
		if((memcmp(m_pClientList[iClientH]->m_cIPaddress, "192.168", 7) != 0) && (memcmp(m_pGameList[iGameServer]->m_cGameServerAddress, m_pClientList[iClientH]->m_cIPaddress, 15) == 0)) {
		memcpy(cp2, m_pGameList[iGameServer]->m_cGameAddressLan, 16);
		cp2 += 16;
		}
		else {
		memcpy(cp2, m_pGameList[iGameServer]->m_cGameServerAddress, 16);
		cp2 += 16;
		}
		
		dwp = (DWORD *)cp2;
		*dwp = m_pGameList[iGameServer]->m_iGamePort;
		cp2 += 2;

		memcpy(cp2, m_pGameList[iGameServer]->m_cGameName, 20);
		cp2 += 20;
		m_iValidAccounts++;
		//m_pClientList[iClientH]->m_cMode = 3; //set client to playing
		m_pAccountList[iAccount]->bAcctive = TRUE;
		strcpy(m_pClientList[iClientH]->m_cAccountName, cAccountName);
		strcpy(m_pClientList[iClientH]->m_cWorldName, cWorldName);
		//DeleteAccount(iAccount);
		SendEventToClient(DEF_ENTERGAMERESTYPE_CONFIRM, DEF_ENTERGAMERESTYPE_CONFIRM, cData2, 38, iClientH);
}
BOOL CMainLog::SaveAccountInfo(int iAccount, char cAccountName[11], char cTemp[11], char cCharName[30], char cMode, int iClientH)
{
	char cFileName[255], cBackup[255], cDir[63], cTxt[50], cAccount[11] , cTxt2[2000], cData[2000], cTemp2[100];
	int iLine, i;
	int    iSize;
	short iMinus;
	int    iCharPos = -1;
    int    iTest = -1;
	BOOL bDeleteLine;
	HANDLE hFile;
	fpos_t pos;
	DWORD  dwSize = 0;
	DWORD dwFileSize;
	FILE * pFile;
	std::string sNewPassword;

	if(m_pClientList[iClientH] == NULL) return FALSE;

	memset(cData, 0, 2000);
    memset(cTxt2, 0, 2000);
	dwFileSize = 0;
	iLine = 0;
	bDeleteLine = FALSE;
	iMinus = 0;
	ZeroMemory(cAccount, sizeof(cAccount));
	ZeroMemory(cTxt, sizeof(cTxt));
	ZeroMemory(cTxt2, sizeof(cTxt2));
	ZeroMemory(cData, sizeof(cData));
	if((cAccountName != NULL) && (iAccount == -1)) {
	for (i = 0; i < DEF_MAXACCOUNTS; i++)
		if((m_pAccountList[i] != NULL)&& (strcmp(m_pAccountList[i]->cAccountName, cAccountName) == 0)) {
		strcpy(cAccount, cAccountName);
		iAccount = i;
		break;
		}
	}
	else if(iAccount != -1) strcpy(cAccount, m_pAccountList[iAccount]->cAccountName); 
	if((iAccount == -1)) return FALSE;

		ZeroMemory(cFileName, sizeof(cFileName));
		ZeroMemory(cDir, sizeof(cDir));
		strcat(cFileName, "Account");
		strcat(cFileName, "\\");
 		strcat(cFileName, "\\");
		wsprintf(cDir, "AscII%d", *cAccount);
		strcat(cFileName, cDir);
		strcat(cFileName, "\\");
		strcat(cFileName, "\\");
		strcat(cFileName, cAccount);	
		strcat(cFileName, ".txt");
		//Backup Directory
		if(strlen(m_cBackupDrive) > 0) {
		ZeroMemory(cBackup, sizeof(cBackup));
		ZeroMemory(cDir, sizeof(cDir));
		strcat(cBackup, m_cBackupDrive);
 		strcat(cBackup, "\\");
		strcat(cBackup, "\\");
		strcat(cBackup, "Account");
		strcat(cBackup, "\\");
 		strcat(cBackup, "\\");
		wsprintf(cDir, "AscII%d", *cAccount);
		strcat(cBackup, cDir);
		strcat(cBackup, "\\");
		strcat(cBackup, "\\");
		strcat(cBackup, cAccount);	
		strcat(cBackup, ".txt");
		}

		hFile = CreateFileA(cFileName, GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL);
		iSize = GetFileSize(hFile, NULL);
		if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
		if(iSize > 2000) {
		SendEventToClient(DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, 0, 0, iClientH);
		return FALSE;
		}

	switch(cMode) {
	case 1: //save new character
		ZeroMemory(cTxt2, sizeof(cTxt2));
		ZeroMemory(cData, sizeof(cData));
			pFile = fopen(cFileName, "rt");
			if (pFile == NULL) {
				wsprintf(G_cTxt, "(X) Account none exist : Name(%s)", cAccount);
				PutLogList(G_cTxt);
				if (pFile != NULL) fclose(pFile);
				return FALSE;
			}
			if(strcmp(cCharName, "WS1") == 0) wsprintf(cTxt, "account-character  = %s\n", cTemp);
			else wsprintf(cTxt, "account-character-%s  = %s\n", cCharName, cTemp);
			fgetpos(pFile, &pos);
			iSize = fread(cData, 1, iSize, pFile);
			fread(cData, dwFileSize, 1, pFile);
			memcpy(cTxt2, cTxt, strlen(cTxt));
			memcpy(cTxt2+strlen(cTxt), cData, iSize);
			SaveInfo(cFileName, cTxt2, 1);
			if(strlen(m_cBackupDrive) > 0) SaveInfo(cBackup, cTxt2, 1);
			//fwrite(cTxt2, 1, strlen(cTxt), pFile);
			if (pFile != NULL) fclose(pFile);
			return TRUE;
		break;

	case 2: //password change
		ZeroMemory(cTxt2, sizeof(cTxt2));
		ZeroMemory(cData, sizeof(cData));
		ZeroMemory(cTemp2, sizeof(cTemp2));
		//memcpy(cTemp2, cTemp, 11);
#ifdef DEF_MD5PASS
		sNewPassword = m_pMD5.getHashFromString(cTemp, DEF_MD5SALT);
#else
//memcpy(cTemp2, cTemp, 11);
		sNewPassword = cTemp;
#endif
		wsprintf(G_cTxt, "(X) PasswordChange(%s %s)", cFileName, sNewPassword.c_str());
		PutLogList(G_cTxt);
		wsprintf(cTxt, "account-password = %s", sNewPassword.c_str());
		pFile = fopen(cFileName, "rt");
			if (pFile == NULL) {
					SendEventToClient(DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, 0, 0, iClientH);
				if (pFile != NULL) fclose(pFile);
				return FALSE;
			}
		fgetpos(pFile, &pos);
        iSize = fread(cData, 1, iSize, pFile);
		//fread(cData, dwFileSize, 1, pFile);
		 for (i = 0; i < iSize; i++)
        {
            if (memcmp((char*)cData+i, "[CHARACTERS]", 12) == 0)
            {
                iCharPos = i;
            }
            if (memcmp((char*)cData+i, "account-password = ", 19) == 0)
            {
                iTest = i;
            }
        }
		if (iTest == -1)
        {
			SendEventToClient(DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, 0, 0, iClientH);
            fclose(pFile);
            return FALSE;
        }
        else
        {
            memcpy(cTxt2, cData, iTest+19);
			memcpy(cTxt2+iTest+19, sNewPassword.c_str(), strlen(sNewPassword.c_str()));
            memcpy(cTxt2+iTest+19+strlen(sNewPassword.c_str()), cData+iTest+19+strlen(m_pAccountList[iAccount]->cPassword), iSize-19-iTest-strlen(m_pAccountList[iAccount]->cPassword));
			SaveInfo(cFileName, cTxt2, 1);
			if(strlen(m_cBackupDrive) > 0) SaveInfo(cBackup, cTxt2, 1);
        }
		if (pFile != NULL) fclose(pFile);
		return TRUE;
		break;

	case 3: //delete character
		ZeroMemory(cTxt2, sizeof(cTxt2));
		ZeroMemory(cData, sizeof(cData));
		ZeroMemory(cTemp, sizeof(cTemp));
		wsprintf(G_cTxt, "(X) Character Delete(%s)", cCharName);
		PutLogList(G_cTxt);
		if(strcmp(cTemp, "WS1")) wsprintf(cTxt, "account-character  = %s", cCharName);
		else wsprintf(cTxt, "account-character-%s  = %s", cTemp, cCharName);
		pFile = fopen(cFileName, "rt");
			if (pFile == NULL) {
					SendEventToClient(DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, 0, 0, iClientH);
				if (pFile != NULL) fclose(pFile);
				return FALSE;
			}
		fgetpos(pFile, &pos);
        iSize = fread(cData, 1, iSize, pFile);
		//fread(cData, dwFileSize, 1, pFile);
		 for (int i = 0; i < iSize; i++)
        {
            if (memcmp((char*)cData+i, "[CHARACTERS]", 12) == 0)
            {
                iCharPos = i;
            }
            if (memcmp((char*)cData+i, cTxt, strlen(cTxt)) == 0)
            {
                iTest = i;
            }
        }
		if (iTest == -1)
        {
			SendEventToClient(DEF_LOGRESMSGTYPE_NOTEXISTINGCHARACTER, DEF_LOGRESMSGTYPE_NOTEXISTINGCHARACTER, 0, 0, iClientH);
            fclose(pFile);
            return FALSE;
        }
        else
        {
			if(iTest-1 <= 0) {
            memcpy(cTxt2, cData+strlen(cTxt)+1, iSize-strlen(cTxt)-1);
			SaveInfo(cFileName, cTxt2, 1);
			if(strlen(m_cBackupDrive) > 0) SaveInfo(cBackup, cTxt2, 1);
			}
			else {
            memcpy(cTxt2, cData, iTest-1);
            memcpy(cTxt2+iTest-1, cData+iTest+strlen(cTxt), iSize-strlen(cTxt)-iTest);
			SaveInfo(cFileName, cTxt2, 1);
			if(strlen(m_cBackupDrive) > 0) SaveInfo(cBackup, cTxt2, 1);
			}
        }
		if (pFile != NULL) fclose(pFile);
		return TRUE;
		break;
	}
	return FALSE;
}
void CMainLog::SaveInfo(char cFileName[255], char *pData, DWORD dwStartSize)
{
	//DWORD dwFileSize;
	FILE * pFile;

		//hFile = CreateFileA(cFileName, GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL);
		//dwFileSize = GetFileSize(hFile, NULL);
		//if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
	try {
		pFile = fopen(cFileName, "wt");
		if (pFile != NULL)
			if (sizeof(pData) > 0)
				fwrite(pData, dwStartSize, strlen(pData), pFile);
		if (pFile != NULL) fclose(pFile);
	}
	catch (std::exception const& e) {
		wsprintf(G_cTxt, "(!CRITICAL) Crash error in SaveInfo #1 (%s)", e.what());
		PutLogList(G_cTxt);
	}

}
int CMainLog::GetAccountInfo(int iClientH, char cAccountName[11], char cAccountPass[11], char cWorldName[30], int * iAccount, char cMode)
{
	char cFileName[255], cDir[63], cTxt[50], cTemp[50];
	char *cp, *token, cReadModeA, cReadModeB;
	char seps[] = "= \t\n";
	class CStrTok * pStrTok;
	int i, iAccountid;
	HANDLE hFile;
	DWORD dwFileSize;
	FILE * pFile;

	BOOL bWorld;
	bWorld = FALSE;
	cReadModeA = cReadModeB = 0;
	iAccountid = 0;

	ZeroMemory(cTxt, sizeof(cTxt));
	ZeroMemory(cTemp, sizeof(cTemp));
    wsprintf(cTxt, "account-character-%s", cWorldName);

	if (strlen(cAccountName) == 0) return 1; // if account blank
	if (strlen(cAccountPass) == 0) return 2; // if password blank
	if ((strlen(cWorldName) == 0)) return 3; // if world dosnt exist

	/*if(cMode == NULL) {
	for (i = 0; i < DEF_MAXCLIENTSOCK; i++)
		if((m_pClientList[i] != NULL) && (strcmp(m_pClientList[i]->m_cWorldName, cWorldName) == 0) && (m_pClientList[i]->bIsActiveWL == TRUE)) bWorld = TRUE; // World exist
	if(bWorld == FALSE) return 3;
	}*/

		ZeroMemory(cFileName, sizeof(cFileName));
		ZeroMemory(cDir, sizeof(cDir));
		strcat(cFileName, "Account");
		strcat(cFileName, "\\");
 		strcat(cFileName, "\\");
		wsprintf(cDir, "AscII%d", *cAccountName);
		strcat(cFileName, cDir);
		strcat(cFileName, "\\");
		strcat(cFileName, "\\");
		strcat(cFileName, cAccountName);	
		strcat(cFileName, ".txt");	

		for (i = 0; i < DEF_MAXACCOUNTS; i++)
			if(m_pAccountList[i] == NULL){
					m_pAccountList[i] = new class CAccount(cAccountName, cWorldName, iClientH);
					strcpy(m_pClientList[iClientH]->m_cAccountName, cAccountName); // save account name
					iAccountid = i;
					*iAccount = i;
			}

		hFile = CreateFileA(cFileName, GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL);
		dwFileSize = GetFileSize(hFile, NULL);
		if(hFile != INVALID_HANDLE_VALUE)  CloseHandle(hFile);
		pFile = fopen(cFileName, "rt");
		if (pFile == NULL) {
			wsprintf(G_cTxt, "(!)Account Dose not Exist (%s)", cFileName);
			PutLogList(G_cTxt);
			return 1;
		}
		//get all account info
		cp = new char[dwFileSize+1];
		ZeroMemory(cp, dwFileSize+1);
		fread(cp, dwFileSize, 1, pFile);
		pStrTok = new class CStrTok(cp, seps);
		token = pStrTok->pGet();
			while (token != NULL) {
				if (cReadModeA != 0) {
					switch (cReadModeA) {
						case 1:
							if(strlen(token) <= 0) {
								delete pStrTok;
								delete cp;
								return 1;
							}
							/*if(memcmp(cAccountName, token, 11) != 0) {
								delete pStrTok;
								delete cp;
								delete m_pAccountList[iAccountid];
								m_pAccountList[iAccountid] = NULL;
								return 1;
							}*/
							cReadModeA = 0;
							break;
						case 2:
							if(strlen(token) <= 0) {
								delete pStrTok;
								delete cp;
								delete m_pAccountList[iAccountid];
								m_pAccountList[iAccountid] = NULL;
								return 1;
							}
							strcpy(m_pAccountList[iAccountid]->cPassword, token);
							cReadModeA = 0;
							break;
						case 3:
							if(strlen(token) <= 0) {
								delete pStrTok;
								delete cp;
								delete m_pAccountList[iAccountid];
								m_pAccountList[iAccountid] = NULL;
								return 1;
							}
							m_pAccountList[iAccountid]->iAccountValid = atoi(token);
							cReadModeA = 0;
							break;
						case 4:
							switch(cReadModeB) {
								case 1:
									if(strlen(token) <= 0) {
										delete pStrTok;
										delete cp;
										delete m_pAccountList[iAccountid];
										m_pAccountList[iAccountid] = NULL;
										return 1;
									}
								m_pAccountList[iAccountid]->m_iAccntYear = atoi(token);
								cReadModeB = 2;
								break;
								case 2:
								if(strlen(token) <= 0) {
										delete pStrTok;
										delete cp;
										delete m_pAccountList[iAccountid];
										m_pAccountList[iAccountid] = NULL;
										return 1;
									}
								m_pAccountList[iAccountid]->m_iAccntMonth = atoi(token);
								cReadModeB = 3;
								break;
								case 3:
								if(strlen(token) <= 0) {
										delete pStrTok;
										delete cp;
										delete m_pAccountList[iAccountid];
										m_pAccountList[iAccountid] = NULL;
										return 1;
									}
								m_pAccountList[iAccountid]->m_iAccntDay = atoi(token);
								cReadModeB = 0;
								cReadModeA = 0;
								break;
							}
							break;
						case 5:
							switch(cReadModeB) {
								case 1:
									if(strlen(token) <= 0) {
										delete pStrTok;
										delete cp;
										delete m_pAccountList[iAccountid];
										m_pAccountList[iAccountid] = NULL;
										return 1;
									}
								m_pAccountList[iAccountid]->m_iPassYear = atoi(token);
								cReadModeB = 2;
								break;
								case 2:
								if(strlen(token) <= 0) {
										delete pStrTok;
										delete cp;
										delete m_pAccountList[iAccountid];
										m_pAccountList[iAccountid] = NULL;
										return 1;
									}
								m_pAccountList[iAccountid]->m_iPassMonth = atoi(token);
								cReadModeB = 3;
								break;
								case 3:
								if(strlen(token) <= 0) {
										delete pStrTok;
										delete cp;
										delete m_pAccountList[iAccountid];
										m_pAccountList[iAccountid] = NULL;
									}
								m_pAccountList[iAccountid]->m_iPassDay = atoi(token);
								cReadModeB = 0;
								cReadModeA = 0;
								break;
							}
							break;
						case 6:
							for (i = 0; i < DEF_MAXCHARACTER; i++)
								if(m_pCharList[i] == NULL) {
								m_pCharList[i] = new class CCharacter(token, iAccountid);
								break;
								}
							cReadModeA = 0;
							break;
					}
				}
				else {
					//if (memcmp(token, "account-name", 12) == 0)			cReadModeA = 1;
					if (memcmp(token, "account-password", 16) == 0)			cReadModeA = 2;
					if (memcmp(token, "account-valid-time", 18) == 0)			cReadModeA = 3;
					if (memcmp(token, "account-valid-date", 18) == 0) {
						cReadModeB = 1;
						cReadModeA = 4;
					}
					if (memcmp(token, "account-change-password", 23) == 0) {
						cReadModeB = 1;
						cReadModeA = 5;
					}
					if((strcmp(cWorldName, "WS1") == 0)) {
					if (memcmp(token, "account-character", 17) == 0)			cReadModeA = 6;
					}
					else { if((memcmp(token, cTxt, strlen(cTxt)) == 0))			cReadModeA = 6; }
				}
				token = pStrTok->pGet();
			}
		delete pStrTok;
		delete cp;
		if (pFile != NULL) fclose(pFile);

	return 0;
}
void CMainLog::PutPacketLogData(DWORD dwMsgID, char *cData, DWORD dwMsgSize)
{
 FILE * pFile;
 char DbgBuffer[15000];

	pFile = fopen("PacketLog.txt", "at");
	if (pFile == NULL) return;

	if (dwMsgSize > 10000) dwMsgSize = 10000;
	int i=0;
	//Needs to be wewrote...
	/*while(i<(int)dwMsgSize){

		wsprintf(DbgBuffer, "MSGID -> 0x%.8X\n", dwMsgID);
		fwrite(DbgBuffer, 1, strlen(DbgBuffer), pFile);
		memset(DbgBuffer,0,sizeof(DbgBuffer));
		strcpy(DbgBuffer, "DATA -> ");
		for(int j=i;j < i+16 && j < (int)dwMsgSize;j++)
			wsprintf(&DbgBuffer[strlen(DbgBuffer)],"%.2X ",(unsigned char)cData[j]);

		for(int a=strlen(DbgBuffer);a < 56; a+=3)
			strcat(DbgBuffer,"   ");

		strcat(DbgBuffer,"\t\t\t");
		for(int j=i;j < i+16 && j < (int)dwMsgSize;j++)
			DbgBuffer[strlen(DbgBuffer)] = isprint((unsigned char)cData[j]) ? cData[j]:'.';

		fwrite(DbgBuffer, 1, strlen(DbgBuffer), pFile);
		fwrite("\r\n", 1, 2, pFile);
		i=j;
	}*/
	fwrite("\r\n\r\n", 1, 4, pFile);
	if (pFile != NULL) fclose(pFile);
}

void CMainLog::CleanupLogFiles()
{
//	PutGMLogData(NULL, NULL, TRUE);
//	PutItemLogData(NULL, NULL, TRUE);
//	PutCrusadeLogData(NULL, NULL, TRUE);
}
BOOL CMainLog::bReadServerConfigFile(char *cFn)
{
 FILE * pFile;
 HANDLE hFile;
 DWORD  dwFileSize;
 char * cp, * token, cReadMode, cTotalList;
 char seps[] = "= \t\n";
 class CStrTok * pStrTok;

	pFile = NULL;
	cTotalList  = 0;
	cReadMode	= 0;
	cTotalList  = 0;
	hFile = CreateFileA(cFn, GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL);
	dwFileSize = GetFileSize(hFile, NULL);
	if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
	pFile = fopen(cFn, "rt");
	if (pFile == NULL) {
		PutLogList("(!) Cannot open configuration file.");
		return FALSE;
	}
	else {
		PutLogList("(!) Reading configuration file...");
		cp = new char[dwFileSize+1];
		ZeroMemory(cp, dwFileSize+1);
		fread(cp, dwFileSize, 1, pFile);
		pStrTok = new class CStrTok(cp, seps);
		token = pStrTok->pGet();
		while (token != NULL) {
			if (cReadMode != 0) {
				switch (cReadMode) {

				case 1: // Mainserver addresss
					strncpy(m_cMainServerAddress, token, 15);
					wsprintf(G_cTxt, "(*) Main-Log-Server Address : %s", m_cMainServerAddress);
					PutLogList(G_cTxt);
					cReadMode = 0;
					break;

				case 2: // backup-drive-letter
					if(strlen(token) > 255) {
						wsprintf(G_cTxt, "(!!) Backup Drive Larger 255Char(%s)", token);
						PutLogList(G_cTxt);
						return FALSE;
					}
					strcpy(m_cBackupDrive, token);
					wsprintf(G_cTxt, "(*) Backup Drive Letter : %s", m_cBackupDrive);
					PutLogList(G_cTxt);
					cReadMode = 0;
					break;

				case 3: // internal-main-server-port
					m_iMainServerInternalPort = atoi(token);
					wsprintf(G_cTxt, "(*) Main-Server Internal port : %d", m_iMainServerInternalPort);
					PutLogList(G_cTxt);
					cReadMode = 0;
					break;

				case 4:	// log server port
					m_iMainServerPort = atoi(token);
					wsprintf(G_cTxt, "(*) Main-Server Internal port : %d", m_iMainServerPort);
					PutLogList(G_cTxt);
					cReadMode = 0;
					break;
				case 5://wlserver list
					strcpy(m_sWLServerList[cTotalList << 4], token);
					wsprintf(G_cTxt, "(*) WL-Server Name: %s", m_sWLServerList[cTotalList << 4]);
					PutLogList(G_cTxt);
					cTotalList++;
					cReadMode = 0;
					break;
				case 6: //SQL Address
					strcpy(m_pSql.cAddress, token);
					wsprintf(G_cTxt, "(*) WL-Server SQL Address: %s", m_pSql.cAddress);
					PutLogList(G_cTxt);
					cReadMode = 0;
					break;
				case 7: //SQL DB
					strcpy(m_pSql.cDatabase, token);
					wsprintf(G_cTxt, "(*) WL-Server SQL DB: %s", m_pSql.cDatabase);
					PutLogList(G_cTxt);
					cReadMode = 0;
					break;
				
				}
			}
			else {
				if (memcmp(token, "log-server-address", 18) == 0)			cReadMode = 1;
				if (memcmp(token, "backup-drive-letter", 19) == 0)			cReadMode = 2;
				if (memcmp(token, "internal-log-server-port", 24) == 0)	cReadMode = 3;
				if (memcmp(token, "log-server-port", 15) == 0)			cReadMode = 4;
				if (memcmp(token, "accept-wlserver-name", 20) == 0)			cReadMode = 5;
				if (memcmp(token, "mysql-address", 13) == 0)			cReadMode = 6;
				if (memcmp(token, "mysql-database", 14) == 0)			cReadMode = 7;
			}
			token = pStrTok->pGet();
		}
		delete pStrTok;
		delete cp;
	}
	if (pFile != NULL) fclose(pFile);
	return TRUE;
}
void CMainLog::DeleteAccount(int iClientH, char cAccountName[11])
{
	int i, x;

	//useing account name
		if(cAccountName != NULL)
		for (i = 0; i < DEF_MAXACCOUNTS; i++)
			if((m_pAccountList[i] != NULL) && (strcmp(m_pAccountList[i]->cAccountName, cAccountName) == 0)&&(m_pAccountList[i]->bAcctive == FALSE)) {
				for (x = 0; x < DEF_MAXCHARACTER; x++)
					if((m_pCharList[x] != NULL) && (m_pCharList[x]->iTracker == i)) {
					delete m_pCharList[x];
					m_pCharList[x] = NULL;
					}
					//wsprintf(G_cTxt, "(!)Account Deleted(%s)", m_pAccountList[i]->cAccountName);
					//PutLogList(G_cTxt);
					delete m_pAccountList[i];
					m_pAccountList[i] = NULL;
			}

	//ussing account id
	if(m_pAccountList[iClientH] == NULL) return;
	if(m_pAccountList[iClientH]->bAcctive == TRUE) return;
	for (x = 0; x < DEF_MAXCHARACTER; x++)
		if((m_pCharList[x] != NULL) && (m_pCharList[x]->iTracker == iClientH)) {
		delete m_pCharList[x];
		m_pCharList[x] = NULL;
		}
		//wsprintf(G_cTxt, "(!)Account Deleted(%s)", m_pAccountList[iClientH]->cAccountName);
		//PutLogList(G_cTxt);
		delete m_pAccountList[iClientH];
		m_pAccountList[iClientH] = NULL;
}
