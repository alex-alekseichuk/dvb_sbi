#include "StdAfx.h"
#include ".\maindlg.h"

#include "SettingsDlg.h"

#include <cassert>

#include <iphlpapi.h>
#include <ipifcons.h>
#include "NetCfg/winerr.h"
#define ALLOCATE_FROM_PROCESS_HEAP( bytes )		::HeapAlloc( ::GetProcessHeap(), HEAP_ZERO_MEMORY, bytes )
#define DEALLOCATE_FROM_PROCESS_HEAP( ptr )		if( ptr ) ::HeapFree( ::GetProcessHeap(), 0, ptr )
#define REALLOC_FROM_PROCESS_HEAP( ptr, bytes )	::HeapReAlloc( ::GetProcessHeap(), HEAP_ZERO_MEMORY, ptr, bytes )

#include "ProcInfo/Toolhelp.h"

#include <shellapi.h>
#pragma comment(lib,"shell32.lib")

/**
	handles the user's 'Install' buttons clicks
*/
LRESULT CMainDlg::OnBnClickedNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	switch (m_nMode)
	{
	case MODE_NO:
	case MODE_DEVICE_WIZARD:
		InstallDriver();
		break;

	case MODE_SOFTWARE:
		InstallSoftware();
		break;
	}

	return 0;
}



bool CMainDlg::ShowSettings()
{
	CSettingsDlg dlg;
	
	if (dlg.DoModal(m_hWnd) != IDOK)
	{
		CloseDialog(0);
		return false;
	}

	m_strLogin = dlg.GetLogin();
	m_strPassword = dlg.GetPassword();
	m_strIP = dlg.GetIP();
	m_nGlobaxPort = dlg.GetGlobaxPort();
	m_nInputSpeed = dlg.GetInputSpeed();
	m_nOutputSpeed = dlg.GetOutputSpeed();

	InstallSoftware();

	return false;
}




///////////////////////////////////////////////////////////
//
// Driver Installation
//
///////////////////////////////////////////////////////////

static CMainDlg *s_pMain = NULL;
/** thread for waiting for devcon.exe rescan finished */
DWORD WINAPI CMainDlg::ThreadFuncDriver( LPVOID lpParam ) 
{ 
	s_pMain->WaitForDevCon(); // call waiter and process method

	::CloseHandle( s_pMain->m_hThread );
	s_pMain->m_hThread = NULL;

	return 0; 
} 
/** main method for install driver stage */
void CMainDlg::InstallDriver()
{
	if (m_nMode == MODE_DRIVER)
		return;
	m_nMode = MODE_DRIVER; // we are going to install the driver

	HWND hWnd = ::GetDlgItem(m_hWnd, IDC_STEP_DRIVER);
	LONG l = ::GetWindowLong(hWnd, GWL_STYLE);
	l &= ~WS_DISABLED;
	::SetWindowLong(hWnd, GWL_STYLE, l);
	this->Invalidate();
	this->UpdateWindow();
	this->ShowWindow(SW_SHOW);

	// kill timer of this stage if it's on
	if (m_TimerID != 0)
	{
		::KillTimer(m_hWnd, getTimerID());
		setTimerID(0);
	}

	// does driver/devise is installed
	if (CheckDriver())
	{
		ShowSettings(); // just go to the next step of the installation
		return;
	}

	::MessageBox(m_hWnd, "We are going to scan your system for skystar2 device.\nYou have to choose the driver from Skybroadcast Installtion CD\n\n If you already have installed driver for skystar2\nremove it by Device Manager then press Ok here", "Warning", MB_ICONEXCLAMATION);

	// call devcon.exe rescan to scan for skystar2 card
	ZeroMemory( &p_si, sizeof(p_si) );
	p_si.cb = sizeof(p_si);
	ZeroMemory( &m_pi, sizeof(m_pi) );
	TCHAR sSetup[] = "devcon.exe rescan";
	if (! CreateProcess(0, sSetup, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &p_si, &m_pi))
	{
		::MessageBox(m_hWnd, "Can't install driver", "SkyStar2 driver installation", MB_ICONERROR);
		return;
	}

	DWORD dwThreadId, dwThrdParam = 1;
	s_pMain = this;


	// create thread for wainting that 'devcon.exe rescan'
	// and followed process
	m_hThread = CreateThread( 
		NULL,                        // default security attributes 
		0,                           // use default stack size  
		ThreadFuncDriver,                  // thread function 
		&dwThrdParam,                // argument to thread function 
		0,                           // use default creation flags 
		&dwThreadId);                // returns the thread identifier 

	assert(m_hThread);
}

/** every 5 seconds checking for wizard finishing */
VOID CALLBACK DevConTimerProcHandler(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	// if driver is installed
	if (s_pMain->CheckDriver())
	{
		// kill timer
		::KillTimer(s_pMain->m_hWnd, s_pMain->getTimerID());
		s_pMain->setTimerID(0);

		// go to next step of installation
		s_pMain->ShowSettings();
	}
}
void CMainDlg::WaitForDevCon() // called from the waiter thread
{
	m_nMode = MODE_DEVCON; // so, now we are waiting for devcon

	::WaitForSingleObject(m_pi.hProcess, INFINITE);
	::CloseHandle( m_pi.hProcess );
	::CloseHandle( m_pi.hThread );

	m_nMode = MODE_DEVICE_WIZARD; // 'new device found' wizard is running

	// each 5 seconds we are checking for that wizard finishing
	m_TimerID = ::SetTimer(m_hWnd, 1, 5000,
		(TIMERPROC)&DevConTimerProcHandler);
}
/**
	returns true if specified device driver is installed.
	it just checks network adapters
*/
bool CMainDlg::CheckDriver()
{
	IP_ADAPTER_INFO* pAdptInfo	= NULL;
	IP_ADAPTER_INFO* pNextAd	= NULL;	
	ULONG ulLen					= 0;
	int nCnt					= 0;
	CWinErr erradapt;
	

	erradapt = ::GetAdaptersInfo( pAdptInfo, &ulLen );
	if( erradapt == ERROR_BUFFER_OVERFLOW ) {
		pAdptInfo = ( IP_ADAPTER_INFO* )ALLOCATE_FROM_PROCESS_HEAP( ulLen );
		erradapt = ::GetAdaptersInfo( pAdptInfo, &ulLen );		
	}

	bool bReturn = false;

		// acquire a count of how many network adapters
		// we are going to need to work on so that
		// we can make sure we have adequate buffer space.
	pNextAd = pAdptInfo;
	while( pNextAd ) {
		if (! strncmp(pNextAd->Description, "B2C2 Broadband Receiver", 23))
		{
			bReturn = true;
		}

		nCnt++;
		pNextAd = pNextAd->Next;
	}

		// free any memory we allocated from the heap before
		// exit.  we wouldn't wanna leave memory leaks now would we? ;p
	DEALLOCATE_FROM_PROCESS_HEAP( pAdptInfo );		

	return bReturn;
}








///////////////////////////////////////////////////////////
//
// Software Installation
//
///////////////////////////////////////////////////////////

/** we a checking for finishing of soft. installation */
VOID CALLBACK SoftTimerProcHandler(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	// huge soft.install check
	if (s_pMain->CheckSoft2())
	{
		// kil timer
		::KillTimer(s_pMain->m_hWnd, s_pMain->getTimerID());
		s_pMain->setTimerID(0);

		s_pMain->InstallIP(); // go to next step
	}
}
void CMainDlg::InstallSoftware()
{
	if (m_nMode == MODE_SOFT_WAIT)
		return;
	m_nMode = MODE_SOFT_WAIT; // we are going to install soft.

	// kil timer if we need
	if (m_TimerID != 0)
	{
		::KillTimer(m_hWnd, getTimerID());
		setTimerID(0);
	}

	HWND hWnd = ::GetDlgItem(m_hWnd, IDC_STEP_SOFT);
	LONG l = ::GetWindowLong(hWnd, GWL_STYLE);
	l &= ~WS_DISABLED;
	::SetWindowLong(hWnd, GWL_STYLE, l);
	this->Invalidate();
	this->UpdateWindow();

	// check if it's already the soft. is installed
	// lite soft.install check
	if (CheckSoft())
	{
		InstallIP(); // just go to next step
		return;
	}

	s_pMain = this;

	// run 'install\\setup.exe'
	ZeroMemory( &p_si, sizeof(p_si) );
	p_si.cb = sizeof(p_si);
	ZeroMemory( &m_pi, sizeof(m_pi) );
	TCHAR sSetup[] = "install\\setup.exe";
	if (! CreateProcess(0, sSetup, NULL, NULL, FALSE, 0, NULL, NULL, &p_si, &m_pi))
	{
		::MessageBox(m_hWnd, "Can't install the software", "Software installation", MB_ICONERROR);
		return;
	}

	DWORD dwThreadId, dwThrdParam = 1;

	::CloseHandle( m_pi.hProcess );
	::CloseHandle( m_pi.hThread );

	m_TimerID = ::SetTimer(m_hWnd, 2, 5000,
		(TIMERPROC)&SoftTimerProcHandler);

}

/**
	lite soft.install check
	return true if soft. is installed 
*/
bool CMainDlg::CheckSoft()
{
	bool bReturn = false;

	// just try open specified key in registry
	// to see if there is soft.
	LONG lRet;
	HKEY hKey;
	lRet = ::RegOpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Broadband Technologies, Inc. -\\Broadband Technologies, Inc. - Broadband4PC\\1.00.000", &hKey);
	if (lRet == ERROR_SUCCESS)
	{
		::RegCloseKey(hKey);
		bReturn = true;
	}

	return bReturn;
}

/**
	huge soft.install check
	used after 'install\setup.exe' running
	it uses lite soft.install check
*/
bool CMainDlg::CheckSoft2()
{
	// if even lite check is not ok
	// then the huge is not ok as well
	if (! CheckSoft())
		return false;

	bool bReturn = true;


	// looking throw the processes list runned on the system
	CToolhelp thProcesses(TH32CS_SNAPPROCESS);
	PROCESSENTRY32 pe = { sizeof(pe) };
	BOOL fOk = thProcesses.ProcessFirst(&pe);
	for (; fOk; fOk = thProcesses.ProcessNext(&pe))
	{
		TCHAR sz[1024];

		// Place the process name (without its path) & ID in the list
		PCTSTR pszExeFile = _tcsrchr(pe.szExeFile, TEXT('\\'));
		if (pszExeFile == NULL) pszExeFile = pe.szExeFile;
		else pszExeFile++; // Skip over the slash

		// if there is special process that setup.exe initiated
		// then soft.install still in progress
		if (!strcmp(pszExeFile, "_ISDel.exe"))
			bReturn = false;
	}

	// if soft.install process is finished
	if (bReturn)
	{
		bReturn = CheckSoft();
		// if soft. is not installed yet
		// then we should NOT go to next step of the installation
		// but the soft.install process is finished ;-)
		if (! bReturn)
			m_nMode = MODE_SOFTWARE; // soft. is not installed yet. but software installation is not in progress
	}

	return bReturn;
}




///////////////////////////////////////////////////////////
//
// IP Installation
//
///////////////////////////////////////////////////////////

DWORD WINAPI CMainDlg::ThreadFuncIP( LPVOID lpParam ) 
{ 
	::WaitForSingleObject(s_pMain->m_pi.hProcess, INFINITE);
	::CloseHandle( s_pMain->m_pi.hProcess );
	::CloseHandle( s_pMain->m_pi.hThread );
	s_pMain->m_hThread = NULL;

	s_pMain->InstallGlobax();
	return 0; 
} 

void CMainDlg::InstallIP()
{
	if (m_nMode == MODE_IP)
		return;
	m_nMode = MODE_IP;

	// kil timer if we need
	if (m_TimerID != 0)
	{
		::KillTimer(m_hWnd, getTimerID());
		setTimerID(0);
	}

	HWND hWnd = ::GetDlgItem(m_hWnd, IDC_STEP_SETTINGS);
	LONG l = ::GetWindowLong(hWnd, GWL_STYLE);
	l &= ~WS_DISABLED;
	::SetWindowLong(hWnd, GWL_STYLE, l);
	this->Invalidate();
	this->UpdateWindow();


	TCHAR szServiceName[1025];
	bool bServiceNameFound = false;

	// open spec. reg.key with network cards
	HKEY hKey;
	LONG lRes;
	lRes = ::RegOpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards", &hKey);
	if (lRes != ERROR_SUCCESS)
	{
		::MessageBox(m_hWnd, "Can't scan network cards", "Error", 0);
		return;
	}

	// enum all network cards
	DWORD dwIndex = 0, dwSize;
	TCHAR szName[1025];
	FILETIME filetime;
	dwSize = 1024;
	lRes = ::RegEnumKeyEx(hKey, dwIndex, szName, &dwSize, NULL, NULL, NULL, &filetime);
	while( lRes == ERROR_SUCCESS)
	{
		//::MessageBox(NULL, lpName, "key", 0);

		// open key with the card name
		szName[dwSize] = '\0';
		HKEY hKeyCard;
		if (ERROR_SUCCESS == ::RegOpenKeyEx(hKey, szName, 0, KEY_READ, &hKeyCard))
		{
			// read card name
			TCHAR szCardName[1025];
			DWORD dwNameSize = 1024;
			DWORD dwFlags = REG_SZ;
			if (ERROR_SUCCESS == ::RegQueryValueEx(hKeyCard, "Description", NULL, &dwFlags, (LPBYTE)szCardName, &dwNameSize))
			{
				szCardName[dwNameSize] = '\0';

				// ! ! ! correct this ! ! !
				LPCTSTR lpCardName = "B2C2 Broadband Receiver"; 
				//LPCTSTR lpCardName = "D-Link DFE-538TX ";
				if (! strncmp(szCardName, lpCardName, strlen(lpCardName)))
				{
					dwNameSize = 1024;
					dwFlags = REG_SZ;
					if (ERROR_SUCCESS == ::RegQueryValueEx(hKeyCard, "ServiceName", NULL, &dwFlags, (LPBYTE)szServiceName, &dwNameSize))
					{
						bServiceNameFound = true;
					}
				}
			}
			::RegCloseKey(hKeyCard);
		}
		
		++ dwIndex;
		dwSize = 1024;
		lRes = ::RegEnumKeyEx(hKey, dwIndex, szName, &dwSize, NULL, NULL, NULL, &filetime);
	}
	::RegCloseKey(hKey);

	// We didn't find the service GUID
	if (! bServiceNameFound)
	{
		::MessageBox(m_hWnd, "Can't find B2C2 Broadband Receiver network service", "Error", 0);
		return;
	}

	


	// find Net GUID
	bool bClassNet = false;
	TCHAR szNetGUID[1025];

	// open network key to enum to find Class=Net
	lRes = ::RegOpenKey(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Network", &hKey);
	assert(lRes == ERROR_SUCCESS);
	
	// enum subkeys
	dwIndex = 0;
	dwSize = 1024;
	lRes = ::RegEnumKeyEx(hKey, dwIndex, szNetGUID, &dwSize, NULL, NULL, NULL, &filetime);
	while( lRes == ERROR_SUCCESS)
	{
		szNetGUID[dwSize] = '\0';
		HKEY hKeyCard;
		// open subkey
		if (ERROR_SUCCESS == ::RegOpenKeyEx(hKey, szNetGUID, 0, KEY_READ, &hKeyCard))
		{

			// read card name
			TCHAR szClassVal[1025];
			DWORD dwClassValSize = 1024;
			DWORD dwFlags = REG_SZ;
			if (ERROR_SUCCESS == ::RegQueryValueEx(hKeyCard, "Class", NULL, &dwFlags, (LPBYTE)szClassVal, &dwClassValSize))
			{
				szClassVal[dwClassValSize] = '\0';

				LPCTSTR lpClassValName = "Net";
				if (! strncmp(szClassVal, lpClassValName, strlen(lpClassValName)))
				{
					bClassNet = true;
					::RegCloseKey(hKeyCard);
					break;
				}
			}

			::RegCloseKey(hKeyCard);
		}

		++ dwIndex;
		dwSize = 1024;
		lRes = ::RegEnumKeyEx(hKey, dwIndex, szNetGUID, &dwSize, NULL, NULL, NULL, &filetime);
	}
	::RegCloseKey(hKey);

	// we didn't find Class=Net
	if (! bClassNet)
	{
		::MessageBox(m_hWnd, "Can't find Network info", "Error", 0);
		return;
	}


	TCHAR szNetworkName[1025];
	bool bNetworkName = false;

	strcpy(szName, "SYSTEM\\CurrentControlSet\\Control\\Network\\");
	strcat(szName, szNetGUID);
	strcat(szName, "\\");
	strcat(szName, szServiceName);
	strcat(szName, "\\Connection");
	lRes = ::RegOpenKey(HKEY_LOCAL_MACHINE, szName, &hKey);
	if (lRes == ERROR_SUCCESS)
	{
		DWORD dwNetworkNameSize = 1024;
		DWORD dwFlags = REG_SZ;
		if (ERROR_SUCCESS == ::RegQueryValueEx(hKey, "Name", NULL, &dwFlags, (LPBYTE)szNetworkName, &dwNetworkNameSize))
		{
			szNetworkName[dwNetworkNameSize] = '\0';
			bNetworkName = true;
		}
		::RegCloseKey(hKey);
	}

	if (! bNetworkName)
	{
		::MessageBox(m_hWnd, "Can't find Network Name", "Error", 0);
		return;
	}


	TCHAR szCmd[1025];
	strcpy(szCmd, "netsh -c \"interface ip\" set address name=\"");
	strcat(szCmd, szNetworkName);
	strcat(szCmd, "\" source=static addr=");
	strcat(szCmd, m_strIP.c_str());
	strcat(szCmd, " mask=255.255.255.0");

	s_pMain = this;

	// run 'install\\setup.exe'
	ZeroMemory( &p_si, sizeof(p_si) );
	p_si.cb = sizeof(p_si);
	ZeroMemory( &m_pi, sizeof(m_pi) );
	if (! CreateProcess(0, szCmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &p_si, &m_pi))
	{
		::MessageBox(m_hWnd, "Can't setup IP", "Settings", MB_ICONERROR);
		return;
	}

	DWORD dwThreadId, dwThrdParam = 1;

	m_hThread = CreateThread( 
		NULL,                        // default security attributes 
		0,                           // use default stack size  
		ThreadFuncIP,                  // thread function 
		&dwThrdParam,                // argument to thread function 
		0,                           // use default creation flags 
		&dwThreadId);                // returns the thread identifier 

	assert(m_hThread);


}



///////////////////////////////////////////////////////////
//
// Globax Installation
//
///////////////////////////////////////////////////////////

DWORD WINAPI CMainDlg::ThreadFuncGlobax( LPVOID lpParam ) 
{ 
	::WaitForSingleObject(s_pMain->m_pi.hProcess, INFINITE);
	::CloseHandle( s_pMain->m_pi.hProcess );
	::CloseHandle( s_pMain->m_pi.hThread );
	s_pMain->m_hThread = NULL;

	s_pMain->InstallProxy();
	return 0; 
} 

void CMainDlg::WriteConfigLine(HANDLE hFile, LPCTSTR lpszLine)
{
	DWORD dw;
	::WriteFile(hFile, lpszLine, strlen(lpszLine), &dw, NULL);
}
void CMainDlg::InstallGlobax()
{
	if (m_nMode == MODE_GLOBAX)
		return;
	m_nMode = MODE_GLOBAX;

	// kil timer if we need
	if (m_TimerID != 0)
	{
		::KillTimer(m_hWnd, getTimerID());
		setTimerID(0);
	}

	HWND hWnd = ::GetDlgItem(m_hWnd, IDC_STEP_GLOBAX);
	LONG l = ::GetWindowLong(hWnd, GWL_STYLE);
	l &= ~WS_DISABLED;
	::SetWindowLong(hWnd, GWL_STYLE, l);
	this->Invalidate();
	this->UpdateWindow();

	// create C:\\Program Files\\GlobaX folder
	LPCTSTR lpszProgramFolder("C:\\Program Files\\GlobaX");
	if (INVALID_HANDLE_VALUE == ::CreateFile(lpszProgramFolder, READ_CONTROL, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL))
	{
		if (! ::CreateDirectory(lpszProgramFolder, 0))
		{
			::MessageBox(m_hWnd, "Can't create GlobaX fodler", "GlobaX", MB_ICONERROR);
			return;
		}
	}


	// get current directory
	// ! ! ! maybe it should be corrected ? ! ! !
	TCHAR szSource[MAX_PATH];
	DWORD dw = ::GetCurrentDirectory(MAX_PATH - 1, szSource);
	if (dw == 0)
	{
		::MessageBox(m_hWnd, "Can't find source GlobaX folder", "GlobaX", MB_ICONERROR);
		return;
	}
	szSource[dw] = '\0';
	strcat(szSource, _T("\\GlobaX\\"));

	// the mask for files search
	TCHAR szMask[MAX_PATH];
	strcpy(szMask, szSource);
	strcat(szMask, "*.*");

	HANDLE hSearch = 0;
	WIN32_FIND_DATA wfd;

	// scan source globax folder and copy them
	if ((hSearch = ::FindFirstFile(szMask, &wfd)) == INVALID_HANDLE_VALUE)
	{
		::MessageBox(m_hWnd, "Can't find GlobaX files", "GlobaX", MB_ICONERROR);
		return;
	}

    do
    {
        if (!strncmp(wfd.cFileName, ".", 1) || !strncmp(wfd.cFileName, "..", 2))            
            continue;

		TCHAR szSrc[MAX_PATH];
		strcpy(szSrc, szSource);
		strcat(szSrc, wfd.cFileName);

		TCHAR szNew[MAX_PATH];
		strcpy(szNew, lpszProgramFolder);
		strcat(szNew, "\\");
		strcat(szNew, wfd.cFileName);

		::CopyFile(szSrc, szNew, FALSE);
    }
	while (::FindNextFile(hSearch, &wfd));

	::FindClose (hSearch);


	// create globax.conf file
	TCHAR szConfig[MAX_PATH];
	strcpy(szConfig, lpszProgramFolder);
	strcat(szConfig, "\\globax.conf");

	HANDLE hFile = ::CreateFile(szConfig, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		::MessageBox(m_hWnd, "Can't create GlobaX config file", "GlobaX", MB_ICONERROR);
		return;
	}

	TCHAR sz[1024];

	// ! ! ! correct these values ! ! !
	// They seem correct. right?
	WriteConfigLine(hFile, "[server]\n");
	sprintf(sz, "port = %d\n", m_nGlobaxPort);
	WriteConfigLine(hFile, sz);
	WriteConfigLine(hFile, "log = client.log\n");
	WriteConfigLine(hFile, "[remote]\n");
	WriteConfigLine(hFile, "name = globax\n");
	sprintf(sz, "server = 62.68.95.22:%d\n", m_nGlobaxPort);
	WriteConfigLine(hFile, sz);
	sprintf(sz, "login = %s\n", m_strLogin.c_str());
	WriteConfigLine(hFile, sz);
	sprintf(sz, "passwd = %s\n", m_strPassword.c_str());
	WriteConfigLine(hFile, sz);
	sprintf(sz, "speed_in = %d\n", m_nInputSpeed);
	WriteConfigLine(hFile, sz);
	sprintf(sz, "speed_out = %d\n", m_nOutputSpeed);
	WriteConfigLine(hFile, sz);
	WriteConfigLine(hFile, "mtu = 1400\n");
	WriteConfigLine(hFile, "mru = 1500\n");
	WriteConfigLine(hFile, "[local]\n");
	WriteConfigLine(hFile, "remote = globax\n");
	WriteConfigLine(hFile, "port = 127.0.0.1:3128\n");
	WriteConfigLine(hFile, "service_int = 0\n");
	WriteConfigLine(hFile, "[local]\n");
	WriteConfigLine(hFile, "remote = globax\n");
	WriteConfigLine(hFile, "port = 127.0.0.1:1080\n");
	WriteConfigLine(hFile, "service_int = 2\n");

	::CloseHandle(hFile);


	s_pMain = this;


	// run 'install.bat'
	ZeroMemory( &p_si, sizeof(p_si) );
	p_si.cb = sizeof(p_si);
	ZeroMemory( &m_pi, sizeof(m_pi) );
	if (! CreateProcess(0, "C:\\Program files\\GlobaX\\install.bat", NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &p_si, &m_pi))
	{
		::MessageBox(m_hWnd, "Can't run intallation script", "GlobaX", MB_ICONERROR);
		return;
	}

	DWORD dwThreadId, dwThrdParam = 1;

	m_hThread = CreateThread( 
		NULL,                        // default security attributes 
		0,                           // use default stack size  
		ThreadFuncGlobax,                  // thread function 
		&dwThrdParam,                // argument to thread function 
		0,                           // use default creation flags 
		&dwThreadId);                // returns the thread identifier 

	assert(m_hThread);

}







//////////////////////////////////////////////////////////
//
// Setup HTTP proxy
//
///////////////////////////////////////////////////////////

void CMainDlg::InstallProxy()
{
	if (m_nMode == MODE_PROXY)
		return;
	m_nMode = MODE_PROXY;

	// kil timer if we need
	if (m_TimerID != 0)
	{
		::KillTimer(m_hWnd, getTimerID());
		setTimerID(0);
	}

	HWND hWnd = ::GetDlgItem(m_hWnd, IDC_STEP_PROXY);
	LONG l = ::GetWindowLong(hWnd, GWL_STYLE);
	l &= ~WS_DISABLED;
	::SetWindowLong(hWnd, GWL_STYLE, l);
	this->Invalidate();
	this->UpdateWindow();

	HKEY hKey;
	DWORD dwFlags;
	REGSAM sam = KEY_WRITE;
	LONG lRes;

	lRes = ::RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", 0, sam, &hKey);
	if (lRes != ERROR_SUCCESS)
	{
		::MessageBox(m_hWnd, "Can't open Internet settings", "Error", 0);
		return;
	}

	dwFlags = REG_DWORD;
	DWORD dw = 1;
	lRes = ::RegSetValueEx(hKey, "ProxyEnable", 0, dwFlags, reinterpret_cast<LPBYTE>(&dw), 4);
	if (lRes == ERROR_SUCCESS)
	{
		dwFlags = REG_SZ;
		LPTSTR lpszProxy("localhost:3128");
		lRes = ::RegSetValueEx(hKey, "ProxyServer", 0, dwFlags, reinterpret_cast<LPBYTE>(lpszProxy), strlen(lpszProxy) + 1);
	}

	if (lRes != ERROR_SUCCESS)
	{
		::MessageBox(m_hWnd, "Can't setup HTTP proxy settings", "Error", 0);
		::RegCloseKey(hKey);
		return;
	}

	::RegCloseKey(hKey);

	InstallRun();
}







//////////////////////////////////////////////////////////
//
// Run the software
//
///////////////////////////////////////////////////////////


void CMainDlg::InstallRun()
{
	if (m_nMode == MODE_RUN)
		return;
	m_nMode = MODE_RUN;

	// kil timer if we need
	if (m_TimerID != 0)
	{
		::KillTimer(m_hWnd, getTimerID());
		setTimerID(0);
	}

	HWND hWnd = ::GetDlgItem(m_hWnd, IDC_STEP_RUN);
	LONG l = ::GetWindowLong(hWnd, GWL_STYLE);
	l &= ~WS_DISABLED;
	::SetWindowLong(hWnd, GWL_STYLE, l);
	this->Invalidate();
	this->UpdateWindow();


	// get the dir of installed soft from registry
	HKEY hKey;
	LONG lRes;
	lRes = ::RegOpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\B2C2, Inc.\\Install", &hKey);
	if (lRes != ERROR_SUCCESS)
	{
		::MessageBox(m_hWnd, "Can't find installed software", "Error", 0);
		return;
	}
	bool bPath = false;
	TCHAR szSoftPath[MAX_PATH];
	DWORD dwszSoftPath = MAX_PATH - 1;
	DWORD dwFlags = REG_SZ;
	if (ERROR_SUCCESS == ::RegQueryValueEx(hKey, "InstDir", NULL, &dwFlags, (LPBYTE)szSoftPath, &dwszSoftPath))
	{
		szSoftPath[dwszSoftPath] = '\0';
		bPath = true;
	}
	::RegCloseKey(hKey);
	if (! bPath)
	{
		::MessageBox(m_hWnd, "Can't find installed software", "Error", 0);
		return;
	}

	// add filename to path
	strcat(szSoftPath, "\\bin\\Server4PC.exe");

	s_pMain = this;

	// run 'Server4PC.exe'
	ZeroMemory( &p_si, sizeof(p_si) );
	p_si.cb = sizeof(p_si);
	ZeroMemory( &m_pi, sizeof(m_pi) );
	if (! CreateProcess(0, szSoftPath, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &p_si, &m_pi))
	{
		::MessageBox(m_hWnd, "Can't run software", "Error", MB_ICONERROR);
		return;
	}
	::CloseHandle( m_pi.hProcess );
	::CloseHandle( m_pi.hThread );

	// run globax
	ZeroMemory( &p_si, sizeof(p_si) );
	p_si.cb = sizeof(p_si);
	ZeroMemory( &m_pi, sizeof(m_pi) );
	if (! CreateProcess(0, "net start globax", NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &p_si, &m_pi))
	{
		::MessageBox(m_hWnd, "Can't run GlobaX", "Error", MB_ICONERROR);
		return;
	}
	::CloseHandle( m_pi.hProcess );
	::CloseHandle( m_pi.hThread );

	// update status. enable 'Done!'
	hWnd = ::GetDlgItem(m_hWnd, IDC_STEP_DONE);
	l = ::GetWindowLong(hWnd, GWL_STYLE);
	l &= ~WS_DISABLED;
	::SetWindowLong(hWnd, GWL_STYLE, l);
	this->Invalidate();
	this->UpdateWindow();

	::ShellExecute(m_hWnd, "open", "http://www.skybroadcast.net/", "", "c:\\", SW_SHOWNORMAL);

}


