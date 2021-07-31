#include <windows.h>
#include "source.h"
#include "MainLog.h"
#include <stdlib.h>
#include <stdio.h>
#include <direct.h>
#include "GlobalDef.h"
#include "resource.h"
//#include "resource.h"

//#define DEF_UPPERVERSION 3
//#define DEF_LOWERVERSION 82
//Default stuff
char		szAppClass[1000];
WNDCLASS	wnd;
HWND		G_hWnd;
WSADATA		wsInfo;
HWND hMSQL;
//MSG			msg;

//Message Processing On Screen
BOOL G_cMsgUpdated = FALSE;
char G_cMsgList[120*50];
char            G_cTxt[500];
//Main Processing Class .....WORLDLOG!!!!
class CMainLog *G_pMainLog;

//Timer
MMRESULT        G_mmTimer = NULL;
static HWND    List1, Button1, Button2;
static HINSTANCE BCX_hInstance;
static int     BCX_ScaleX, BCX_ScaleY;
static char    BCX_ClassName[2048];
HWND    BCX_Listbox(char*,HWND,int,int,int,int,int,int=0,int=-1);
HWND    BCX_Button(char*,HWND,int,int,int,int,int,int=0,int=-1);
HWND    BCX_Button2(char*,HWND,int,int,int,int,int,int=0,int=-1);
HWND hSv;

HINSTANCE hInst;

int ItemCount=0;
//Sockets
class XSocket * G_pInternalLogSock = NULL;

LRESULT CALLBACK WindowProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg){
		//---------Buttons---------//
	case WM_COMMAND:
		/*switch(wParam){
			case 1010:
					G_cMsgUpdated = TRUE;
					SendMessage(List1,(UINT)LB_RESETCONTENT, 0, 0);
					ItemCount = 0;
					PutLogList("Cleared");
					break;
			case 1011:
				 char pData[120];
	G_pMainLog->UpdateNoticement(wParam);
		if (!G_bIsMainLogActive) {
			G_bIsMainLogActive = TRUE;
			G_pXMainLogSock = new class XSocket(G_hWnd, DEF_SERVERSOCKETBLOCKLIMIT);
			G_pXMainLogSock->bListen(G_pMainLog->m_cWorldServerAddress, G_pMainLog->m_iWorldServerPort, WM_WORLD_LOG_LISTENER);
			PutLogList("(!) Log-Socket Listening... Server Activated.");
			ZeroMemory(pData, sizeof(pData));
			memcpy(pData, G_pMainLog->m_cWorldLogName, 30);
			G_pMainLog->SendEventToMLS(MSGID_WORLDSERVERACTIVATED, 0, pData, 30, -1);
		}
				break;
	case IDM_SERVER:
		DialogBox(hInst, (LPCTSTR)IDD_SERVERS, G_hWnd, (DLGPROC)Server);
		break;
	case IDM_CLIENT:
		DialogBox(hInst, (LPCTSTR)IDD_SERVERS, G_hWnd, (DLGPROC)Clients);
		break;
				 }*/
				 break;

	case WM_CLOSE:
		if (MessageBox(hWnd, "Quit Main log server?", "Main-LOG", MB_YESNO + MB_ICONINFORMATION) == IDYES) {
			return DefWindowProc(hWnd, WM_CLOSE, wParam, lParam);
		}
		break;
		
	case WM_DESTROY:
		OnDestroy();
		break;

	case WM_PAINT:
		OnPaint();
		break;

	case WM_USER_TIMERSIGNAL:
		G_pMainLog->OnTimer();
		break;

	case WM_USER_ACCEPT: //public port 2828 - for clients
		G_pMainLog->bAccept(G_pXMainLogSock);
		break;
	case WM_KEYUP:
		OnKeyUp(wParam, lParam);
		return DefWindowProc(hWnd, WM_KEYUP, wParam, lParam);

	default: 
		if ((uMsg >= WM_ONCLIENTSOCKETEVENT) && (uMsg <= WM_ONCLIENTSOCKETEVENT + DEF_MAXCLIENTSOCK)) {
		G_pMainLog->OnClientSubLogSocketEvent(uMsg, wParam, lParam); //all client pack handler
		}
		return DefWindowProc(hWnd,uMsg,wParam,lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){

	wsprintf(szAppClass, "Main Log Server%d", hInstance);
	if (!InitApplication( hInstance)) return (FALSE);
	if (!InitInstance(hInstance, nCmdShow)) return (FALSE);
	Initialize();
	EventLoop();
	return 0;
}

BOOL InitApplication( HINSTANCE hInstance)
{     
 WNDCLASS  wc;

	wc.style = (CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS);
	wc.lpfnWndProc   = (WNDPROC)WindowProc;             
	wc.cbClsExtra    = 0;                            
	wc.cbWndExtra    = sizeof (int);             
	wc.hInstance     = hInstance;                    
	wc.hIcon         = NULL;
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);  
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);	 
//	wc.lpszMenuName  = (LPCSTR)IDR_MENU1;
	wc.lpszMenuName  = NULL; 
	wc.lpszClassName = szAppClass;                   

	return (RegisterClass(&wc));
}

BOOL InitInstance( HINSTANCE hInstance, int nCmdShow )
{
 char cTitle[100];

	wsprintf(cTitle, "Helbreath Main-Log-Server V%d.%d - Marleythe9", DEF_UPERVERSION, DEF_LOWERVERSION);

	G_hWnd = CreateWindowEx(0,  // WS_EX_TOPMOST,
		szAppClass,
		cTitle,
		WS_VISIBLE | // so we don't have to call ShowWindow
		//WS_POPUP |   // non-app window
		//WS_CAPTION | // so our menu doesn't look ultra-goofy
		WS_SYSMENU |  // so we get an icon in the tray
		WS_MINIMIZEBOX, 
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		600, //GetSystemMetrics(SM_CXSCREEN),
		720, //GetSystemMetrics(SM_CYSCREEN),
		NULL,
		NULL,
		hInstance,
		NULL );

	if (!G_hWnd) return (FALSE);
	//MAJORHG - CLEROTH - 26/03/05
    WNDCLASS Wc;
	strcpy(BCX_ClassName,"ListBox1");
	BCX_ScaleX       = 1;
	BCX_ScaleY       = 1;
	BCX_hInstance    =  hInstance;
	Wc.style         =  CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	Wc.cbClsExtra    =  0;
	Wc.cbWndExtra    =  0;
	Wc.hInstance     =  hInstance;
	Wc.hIcon         =  LoadIcon(NULL,IDI_WINLOGO);
	Wc.hCursor       =  LoadCursor(NULL,IDC_ARROW);
	Wc.hbrBackground =  (HBRUSH)(COLOR_BTNFACE+1);
	Wc.lpszMenuName  =  NULL;
	Wc.lpszClassName =  BCX_ClassName;
	RegisterClass(&Wc);
	//----------Main CMD--------//
	//List1=BCX_Listbox("",G_hWnd,1009,20,45,520,200);
	//Button1=BCX_Button("",G_hWnd,1010,100,245,80,20);
	//Button2=BCX_Button2("",G_hWnd,1011,20,245,80,20);
	//------------------------------//

	ShowWindow(G_hWnd, nCmdShow);    
	UpdateWindow(G_hWnd);            
	return (TRUE);                 
}

int EventLoop()
{
 static unsigned short _usCnt = 0; 
 register MSG msg;

	while(1) {
		if(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
			if(!GetMessage(&msg, NULL, 0, 0)) {
				return msg.wParam;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			UpdateScreen();
		}
		else WaitMessage();
	}
}

void Initialize()
{

	if (_InitWinsock() == FALSE) {
		MessageBox(G_hWnd, "Socket 1.1 not found! Cannot execute program.","ERROR", MB_ICONEXCLAMATION | MB_OK);
		PostQuitMessage(0);
		return;
	}

	G_pMainLog = new class CMainLog(G_hWnd);
	if (G_pMainLog == NULL) {
		MessageBox(G_hWnd, "Init fail - Invalid WorldLServer!","ERROR",MB_OK+MB_ICONERROR);
		return;
	}
	if (G_pMainLog->bInit() == FALSE) {
		MessageBox(G_hWnd, "Init fail - Missing required .cfg files!","ERROR",MB_OK+MB_ICONERROR);
		return;
	}

	//G_pInternalLogSock = new class XSocket(G_hWnd, DEF_SERVERSOCKETBLOCKLIMIT);
	//G_pInternalLogSock->bListen(G_pMainLog->m_cMainServerAddress, G_pMainLog->m_iMainServerInternalPort, WM_INTERNAL_LOG_ACCEPT);
	//PutLogList("(!) Internal-log-Socket Listening...");
	G_mmTimer = _StartTimer(300);
	
}

void OnDestroy()
{
	//G_pMainLog->CleanupLogFiles();
	if (G_pMainLog != NULL) delete G_pMainLog;
	if (G_pXMainLogSock != NULL) delete G_pXMainLogSock;
	if (G_pInternalLogSock != NULL) delete G_pInternalLogSock;
	_TermWinsock();
	if (G_mmTimer != NULL) _StopTimer(G_mmTimer);
	PostQuitMessage(0);
}

void UpdateScreen()
{
	if (G_cMsgUpdated == TRUE) {
		InvalidateRect(G_hWnd, NULL, TRUE);
		G_cMsgUpdated = FALSE;
	}
}

void OnPaint()
{
 HDC hdc;
 PAINTSTRUCT ps;
 register short i;
 char * cMsg;

	hdc = BeginPaint(G_hWnd, &ps);
	SetBkMode(hdc,TRANSPARENT);
	for (i = 0; i < 40; i++) {
		cMsg = (char *)(G_cMsgList + i*120);
		TextOut(hdc, 5, 5 + 660 - i*16, cMsg, strlen(cMsg));
	}
	if (G_bIsMainLogActive == TRUE) {
		wsprintf(G_cTxt,"Total World(%d) / Game(%d) / Valid Accounts(%d)", G_pMainLog->m_iTotalWorld,  G_pMainLog->m_iTotalGame, G_pMainLog->m_iValidAccounts);
	}
	else {
		wsprintf(G_cTxt,"PRESS [HOME] KEY TO ACTIVATE SERVER!!!");
	}
	TextOut(hdc, 5, 10, G_cTxt, strlen(G_cTxt));
	if (G_pMainLog != NULL) {

	}
	
	EndPaint(G_hWnd, &ps);
}

void PutGameLogData(char * cMsg)
{
 FILE * pFile;
 char cBuffer[512];
 SYSTEMTIME SysTime;

	pFile = NULL;
	ZeroMemory(cBuffer, sizeof(cBuffer));
	_mkdir("GameLogData");
	GetLocalTime(&SysTime);
	wsprintf(cBuffer, "GameLogData\\GameEvents%04d%02d%02d.log", SysTime.wYear, SysTime.wMonth, SysTime.wDay);
	pFile = fopen(cBuffer, "at");
	if (pFile == NULL) return;
	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "(%4d:%2d:%2d:%2d:%2d) - ", SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute);
	strcat(cBuffer, cMsg);
	strcat(cBuffer, "\n");
	fwrite(cBuffer, 1, strlen(cBuffer), pFile);
	fclose(pFile);

}

void PutEventLog(char * cMsg)
{
 FILE * pFile;
 char cBuffer[512];
 SYSTEMTIME SysTime;

	pFile = fopen("MainLog.log", "at");
	if (pFile == NULL) return;
	ZeroMemory(cBuffer, sizeof(cBuffer));
	GetLocalTime(&SysTime);
	wsprintf(cBuffer, "(%4d:%2d:%2d:%2d:%2d) - ", SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute);
	strcat(cBuffer, cMsg);
	strcat(cBuffer, "\n");
	fwrite(cBuffer, 1, strlen(cBuffer), pFile);
	fclose(pFile);
}

void PutLogList(char * cMsg)
{
	char cTemp[120*50];

	G_cMsgUpdated = TRUE;
	ZeroMemory(cTemp, sizeof(cTemp));
	memcpy((cTemp + 120), G_cMsgList, 5880);
	memcpy(cTemp, cMsg, strlen(cMsg));
	memcpy(G_cMsgList, cTemp, 6000);
	PutEventLog(cMsg);
	//SendMessage(List1,(UINT)LB_ADDSTRING,(WPARAM)0,(LPARAM)cMsg);
	//SendMessage(List1,(UINT)LB_SETCURSEL,ItemCount,0);
	//ItemCount++;
}

void CALLBACK _TimerFunc(UINT wID, UINT wUser, DWORD dwUSer, DWORD dw1, DWORD dw2)
{
	PostMessage(G_hWnd, WM_USER_TIMERSIGNAL, wID, NULL);
}

MMRESULT _StartTimer(DWORD dwTime)
{
 TIMECAPS caps;
 MMRESULT timerid;

	timeGetDevCaps(&caps, sizeof(caps));
	timeBeginPeriod(caps.wPeriodMin);
	timerid = timeSetEvent(dwTime,0,_TimerFunc,0, (UINT)TIME_PERIODIC);

	return timerid;
}

void _StopTimer(MMRESULT timerid)
{
 TIMECAPS caps;

	if (timerid != 0) {
		timeKillEvent(timerid);
		timerid = 0;
		timeGetDevCaps(&caps, sizeof(caps));
		timeEndPeriod(caps.wPeriodMin);
	}
}

void OnKeyUp(WPARAM wParam, LPARAM lParam)
{
//int i;
 char pData[120];
	switch(wParam) {

	case VK_HOME:
		if (!G_bIsMainLogActive) {
			G_bIsMainLogActive = TRUE;
			G_pXMainLogSock = new class XSocket(G_hWnd, DEF_SERVERSOCKETBLOCKLIMIT);
			G_pXMainLogSock->bListen(G_pMainLog->m_cMainServerAddress, G_pMainLog->m_iMainServerPort, WM_USER_ACCEPT);
			PutLogList("(!) Log-Socket Listening... Server Activated.");
			ZeroMemory(pData, sizeof(pData));
		}
		break;
	}
}

LRESULT CALLBACK UMYSQL(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	hMSQL = hDlg;
	switch (message)
	{
	case WM_INITDIALOG:
		return TRUE;

	case WM_COMMAND:
		/*if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}*/
		switch (wParam) {

		case IDOK:
			//char cUser[11];
			//char cPass[11];
			GetDlgItemText(hDlg, IDC_EDIT1, G_pMainLog->m_pSql.cUsername, 20);
			GetDlgItemText(hDlg, IDC_EDIT2, G_pMainLog->m_pSql.cPassword, 64);
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;;
			break;

		}
		break;
	}
	return FALSE;
}
void MysqlUser()
{
	DialogBox(hInst, (LPCTSTR)IDD_DIALOG1, G_hWnd, (DLGPROC)UMYSQL);
}