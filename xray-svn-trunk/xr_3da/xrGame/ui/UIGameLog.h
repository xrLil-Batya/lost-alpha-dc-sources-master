//=============================================================================
//	Created by Vitaly 'Mad Max' Maximov, mad-max@gsc-game.kiev.ua
//	Copyright 2005. GSC Game World
//	---------------------------------------------------------------------------
//  Game log window
//=============================================================================

#pragma once

#include "UIScrollView.h"

class CUIXml;
class CUIPdaMsgListItem;
class CUIStatic;
class CUITextWnd;

class CUIGameLog: public CUIScrollView
{
public:
							CUIGameLog		();
	CUITextWnd*				AddLogMessage	(LPCSTR msg);
	CUIPdaMsgListItem*		AddPdaMessage	(LPCSTR msg, float delay);
	void					AddChatMessage	(LPCSTR msg, LPCSTR author);
	virtual void			Update			();

	void					SetTextAtrib	(CGameFont* pFont, u32 color);
	u32						GetTextColor	()								{return txt_color;}

private:
	WINDOW_LIST				toDelList;
	u32						txt_color;
	CGameFont*				m_pFont;
};