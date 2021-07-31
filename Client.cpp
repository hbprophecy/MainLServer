#include "Client.h"

CClient::CClient(HWND hWnd)
{
	m_pXSock = NULL;
	m_pXSock = new class XSocket(hWnd, DEF_CLIENTSOCKETBLOCKLIMIT);
	m_pXSock->bInitBufferSize(DEF_MSGBUFFERSIZE);
	ZeroMemory(m_cIPaddress, sizeof(m_cIPaddress));
	ZeroMemory(m_cWorldName, sizeof(m_cWorldName));
	ZeroMemory(m_cWorldServerAddress, sizeof(m_cWorldServerAddress));
	ZeroMemory(m_cAccountName, sizeof(m_cAccountName));
	m_iWorldPort = 0;
	bIsworld = FALSE;
	bIsActiveWL = FALSE;
	bIsShutdown = FALSE;
}
CClient::~CClient()
{
if (m_pXSock != NULL) delete m_pXSock;
}