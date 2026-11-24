// SettingsDlg.h : Declaration of the CSettingsDlg

#pragma once

#include "resource.h"       // main symbols
#include <atlhost.h>

#include <atlctrls.h>

#include <string>



// CSettingsDlg

class CSettingsDlg : 
	public CAxDialogImpl<CSettingsDlg>
{
protected:
	int m_nInputSpeed;
	int m_nOutputSpeed;
	std::string m_strLogin;
	std::string m_strPassword;
	std::string m_strIP;
	int m_nGlobaxPort;

public:
	inline int GetInputSpeed() {return m_nInputSpeed;}
	inline int GetOutputSpeed() {return m_nOutputSpeed;}
	inline LPCTSTR GetLogin() {return m_strLogin.c_str();}
	inline LPCTSTR GetPassword() {return m_strPassword.c_str();}
	inline LPCTSTR GetIP() {return m_strIP.c_str();}
	inline int GetGlobaxPort() {return m_nGlobaxPort;}


	CSettingsDlg()
	{
	}

	~CSettingsDlg()
	{
	}

	enum { IDD = IDD_SETTINGSDLG };

BEGIN_MSG_MAP(CSettingsDlg)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
	COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
	CHAIN_MSG_MAP(CAxDialogImpl<CSettingsDlg>)
END_MSG_MAP()

// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		CAxDialogImpl<CSettingsDlg>::OnInitDialog(uMsg, wParam, lParam, bHandled);

		int n;
		HWND hWnd;

		std::string asInput[9] = {
			std::string("32 Kbits"),
			std::string("64 Kbits"),
			std::string("128 Kbits"),
			std::string("256 Kbits"),
			std::string("384 Kbits"),
			std::string("512 Kbits"),
			std::string("768 Kbits"),
			std::string("1024 Kbits"),
			std::string("2048 Kbits")
		};
		hWnd = ::GetDlgItem(m_hWnd, IDC_INPUTSPEED);
		for (n=0;n<9;++n)
			::SendMessage(hWnd, CB_ADDSTRING, -1, (LPARAM)asInput[n].c_str());
		::SendMessage(hWnd, CB_SETCURSEL, 0, 0L);

		std::string asOutput[7] = {
			std::string("9 Kbits"),
			std::string("14 Kbits"),
			std::string("32 Kbits"),
			std::string("64 Kbits"),
			std::string("128 Kbits"),
			std::string("256 Kbits"),
			std::string("512 Kbits")
		};
		hWnd = ::GetDlgItem(m_hWnd, IDC_OUTPUTSPEED);
		for (n=0;n<7;++n)
			::SendMessage(hWnd, CB_ADDSTRING, -1, (LPARAM)asOutput[n].c_str());
		::SendMessage(hWnd, CB_SETCURSEL, 0, 0L);

		return 1;  // Let the system set the focus
	}

	LRESULT OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		int nInput[9] = {32*128, 64*128, 128*128, 256*128, 512*128, 768*128, 1024*128, 2048*128};
		int nOutput[7] = {1024, 1024, 32*128, 64*128, 128*128, 256*128, 512*128};

		int i;
		HWND hWnd = ::GetDlgItem(m_hWnd, IDC_INPUTSPEED);
		i = ::SendMessage(hWnd, CB_GETCURSEL, 0, 0L);
		if (i>=0 && i<9)
			m_nInputSpeed = nInput[i];

		hWnd = ::GetDlgItem(m_hWnd, IDC_OUTPUTSPEED);
		i = ::SendMessage(hWnd, CB_GETCURSEL, 0, 0L);
		if (i>=0 && i<7)
			m_nOutputSpeed = nOutput[i];

		TCHAR sz[51];
		::GetDlgItemText(m_hWnd, IDC_LOGIN, sz, 50);
		m_strLogin = sz;
		::GetDlgItemText(m_hWnd, IDC_PASSWORD, sz, 50);
		m_strPassword = sz;
		::GetDlgItemText(m_hWnd, IDC_IP, sz, 50);
		m_strIP = sz;
		::GetDlgItemText(m_hWnd, IDC_GLOBAXPORT, sz, 50);
		m_nGlobaxPort = atoi(sz);

		bool bError = false;
		std::string strError("Errors:\n");
		if (m_strLogin.length() == 0)
		{
			bError = true;
			strError += "\tLogin field should have min. 1 char\n";
		}
		if (m_strLogin.length() > 40)
		{
			bError = true;
			strError += "\tLogin field should have max. 40 chars\n";
		}
		if (m_strPassword.length() == 0)
		{
			bError = true;
			strError += "\tPassword field should have min. 1 char\n";
		}
		if (m_strPassword.length() > 40)
		{
			bError = true;
			strError += "\tPassword field should have max. 40 chars\n";
		}
		if (m_strIP.length() < 7)
		{
			bError = true;
			strError += "\tIP field should have min. 7 char\n";
		}
		if (m_strIP.length() > 40)
		{
			bError = true;
			strError += "\tIP field should have max. 40 chars\n";
		}
		if (m_nGlobaxPort < 1)
		{
			bError = true;
			strError += "\tGlobax port field should have min. 1\n";
		}
		if (m_nGlobaxPort > 99999)
		{
			bError = true;
			strError += "\tGlobax port field should have max. 99999\n";
		}

		if (bError)
		{
			::MessageBox(hWnd, strError.c_str(), "Incorrect parameters", MB_ICONERROR);
			return 0 ;
		}

		EndDialog(1);
		return 1;
	}

	LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		

		EndDialog(0);
		return 0;
	}
};

