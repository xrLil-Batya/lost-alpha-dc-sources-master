// SVGameTypeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Stalker_net.h"
#include "SVGameTypeDlg.h"

#include "ServerDlg.h"
#include ".\svgametypedlg.h"
// SVGameTypeDlg dialog

IMPLEMENT_DYNAMIC(SVGameTypeDlg, CSubDlg)
SVGameTypeDlg::SVGameTypeDlg(CWnd* pParent /*=NULL*/)
	: CSubDlg(SVGameTypeDlg::IDD, pParent)
{
}

SVGameTypeDlg::~SVGameTypeDlg()
{
}

void SVGameTypeDlg::DoDataExchange(CDataExchange* pDX)
{
	CSubDlg::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_GAMEDM, m_pGameDM);
	DDX_Control(pDX, IDC_GAMETDM, m_pGameTDM);
	DDX_Control(pDX, IDC_GAMEAHUNT, m_pGameAHunt);
//	DDX_Control(pDX, IDC_DEDICATED, m_pDedicated);
//	DDX_Control(pDX, IDC_SPECTATORONLY, m_pSpectatorOnly);
//	DDX_Control(pDX, IDC_SPECTRTIME, m_pSpectrTime);
}


BEGIN_MESSAGE_MAP(SVGameTypeDlg, CSubDlg)
	ON_BN_CLICKED(IDC_GAMEDM, OnBnClickedGameDM)
	ON_BN_CLICKED(IDC_GAMETDM, OnBnClickedGameTDM)
	ON_BN_CLICKED(IDC_GAMEAHUNT, OnBnClickedGameAHunt)
//	ON_BN_CLICKED(IDC_SPECTATORONLY, OnBnClickedSpectatoronly)
END_MESSAGE_MAP()

BOOL SVGameTypeDlg::OnInitDialog()
{
	CSubDlg::OnInitDialog();

	m_pGameDM.EnableWindow(TRUE);
	m_pGameTDM.EnableWindow(TRUE);
	m_pGameAHunt.EnableWindow(TRUE);

	m_pGameDM.SetCheck(1);
	m_pGameTDM.SetCheck(0);
	m_pGameAHunt.SetCheck(0);
	//-----------------------------------------
	
	return TRUE;  // return TRUE  unless you set the focus to a control
};


// SVGameTypeDlg message handlers
void SVGameTypeDlg::OnBnClickedGameDM()
{

}

void SVGameTypeDlg::OnBnClickedGameTDM()
{


void SVGameTypeDlg::OnBnClickedGameAHunt()
{

}

void	SVGameTypeDlg::OnGameTypeSwitch(byte NewGameType)
{

};
/*
void SVGameTypeDlg::OnBnClickedSpectatoronly()
{
	// TODO: Add your control notification handler code here
	if (m_pSpectatorOnly.GetCheck())
	{
		m_pSpectrTime.EnableWindow(true);
	}
	else
	{
		m_pSpectrTime.EnableWindow(false);
	};
}
*/