// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include ".\resource.h"
#include "AboutDlg.h"

#include <string>

class CMainDlg : public CDialogImpl<CMainDlg>, public CUpdateUI<CMainDlg>,
		public CMessageFilter, public CIdleHandler
{
public:
	enum MODE {
		MODE_NO = 0, // nothing is in progress. so, user should click Install

		MODE_DRIVER, // we are going to start device/driver isntalltion procedure
		MODE_DEVCON, // waiting for 'devcon.exe restart' finished
		MODE_DEVICE_WIZARD, // 'new device found' wizard is running

		MODE_SOFTWARE, // soft. is not installed yet. but software installation is not in progress
		MODE_SOFT_WAIT, // software installtion is in progress

		MODE_IP, // installing IP

		MODE_GLOBAX, // installing (coping) globax software
	
		MODE_PROXY, // Setup HTTP proxy for IE

		MODE_RUN, // installation is Done. Run is in progress

		MODE_SETTINGS // the first mode. user should enter
	};

protected:
	MODE m_nMode;

	PROCESS_INFORMATION m_pi;
	STARTUPINFO p_si;
	HANDLE m_hThread;
	UINT_PTR m_TimerID;


	std::string m_strLogin;
	std::string m_strPassword;
	std::string m_strIP;
	int m_nGlobaxPort;
	int m_nInputSpeed;
	int m_nOutputSpeed;

public:
	inline UINT_PTR getTimerID() { return m_TimerID; }
	inline void setTimerID(UINT_PTR i) { m_TimerID = i; }

public:
	enum { IDD = IDD_MAINDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		return CWindow::IsDialogMessage(pMsg);
	}

	virtual BOOL OnIdle()
	{
		return FALSE;
	}

	BEGIN_UPDATE_UI_MAP(CMainDlg)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDC_NEXT, OnBnClickedNext)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		// center the dialog on the screen
		CenterWindow();

		// set icons
		HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
			IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
		SetIcon(hIcon, TRUE);
		HICON hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
			IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
		SetIcon(hIconSmall, FALSE);

		// register object for message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->AddMessageFilter(this);
		pLoop->AddIdleHandler(this);

		UIAddChildWindowContainer(m_hWnd);

/*
		// positioning the window to the 
		DEVMODE devMode;
		if (::EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode))
		{
			int ws = devMode.dmPelsWidth;
			RECT rect = {0, 0, 0, 0};

			const int WIDTH = 160;
			const int HEIGHT = 150;

			rect.top = 20;
			rect.bottom = rect.top + HEIGHT;
//			rect.left = ws - WIDTH - 20;
//			rect.right = rect.left + WIDTH;
			rect.left = 20;
			rect.right = rect.left + WIDTH;

			//SetWindowPos(HWND_TOP, &rect, SWP_SHOWWINDOW);
		}
*/

		m_TimerID = 0;
		m_nMode = MODE_NO;

		m_strIP = "";
		m_strLogin = "";
		m_strPassword = "";

		// try to install the driver first
		InstallDriver();
		//ShowSettings();

		return TRUE;
	}

	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CAboutDlg dlg;
		dlg.DoModal();
		return 0;
	}

	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		// TODO: Add validation code 
		CloseDialog(wID);
		return 0;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CloseDialog(wID);
		return 0;
	}

	void CloseDialog(int nVal)
	{
		DestroyWindow();
		::PostQuitMessage(nVal);
	}


	LRESULT OnBnClickedNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);


	void InstallDriver();
	bool CheckDriver();

	void InstallSoftware();
	bool CheckSoft();
	bool CheckSoft2();

	void InstallIP();

	void InstallGlobax();

	void InstallProxy();

	void InstallRun();

	bool ShowSettings();

protected:
	static DWORD WINAPI ThreadFuncDriver( LPVOID lpParam );
	static DWORD WINAPI ThreadFuncIP( LPVOID lpParam );
	static DWORD WINAPI ThreadFuncGlobax( LPVOID lpParam );
	void WaitForDevCon();

	static void WriteConfigLine(HANDLE hFile, LPCTSTR lpszLine);
};
