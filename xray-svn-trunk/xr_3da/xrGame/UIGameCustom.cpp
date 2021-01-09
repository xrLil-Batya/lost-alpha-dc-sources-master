#include "pch_script.h"
#include "UIGameCustom.h"
#include "level.h"
#include "ui/UIXmlInit.h"
#include "ui/UIStatic.h"
#include "ui/UIMultiTextStatic.h"
#include "object_broker.h"
#include "string_table.h"

#include "InventoryOwner.h"

#include "ui/UIPdaWnd.h"
#include "ui/UIMainIngameWnd.h"
#include "ui/UIMessagesWindow.h"
#include "ui/UIInventoryWnd.h"
#include "ui/UICarBodyWnd.h"

#include "actor.h"
#include "inventory.h"
#include "game_cl_base.h"

#include "../x_ray.h"

bool predicate_sort_stat(const SDrawStaticStruct* s1, const SDrawStaticStruct* s2) 
{
	return ( s1->IsActual() > s2->IsActual() );
}

struct predicate_find_stat 
{
	LPCSTR	m_id;
	predicate_find_stat(LPCSTR id):m_id(id)	{}
	bool	operator() (SDrawStaticStruct* s) 
	{
		return ( s->m_name==m_id );
	}
};

CUIGameCustom::CUIGameCustom()
	:m_pgameCaptions(NULL), m_msgs_xml(NULL), m_window(NULL), m_InventoryMenu(NULL), m_PdaMenu(NULL), m_UICarBodyMenu(NULL), UIMainIngameWnd(NULL), m_pMessagesWnd(NULL), m_WeatherEditor(NULL)
{
	ShowGameIndicators		(true);
	ShowCrosshair			(true);
}
bool g_b_ClearGameCaptions = false;

CUIGameCustom::~CUIGameCustom()
{
	delete_data				(m_custom_statics);
	g_b_ClearGameCaptions	= false;
}


void CUIGameCustom::OnFrame() 
{
	CDialogHolder::OnFrame();

	st_vec_it it = m_custom_statics.begin();
	st_vec_it it_e = m_custom_statics.end();
	for(;it!=it_e;++it)
		(*it)->Update();

	std::sort(	it, it_e, predicate_sort_stat );
	
	while(!m_custom_statics.empty() && !m_custom_statics.back()->IsActual())
	{
		delete_data					(m_custom_statics.back());
		m_custom_statics.pop_back	();
	}
	
	if(g_b_ClearGameCaptions)
	{
		delete_data				(m_custom_statics);
		g_b_ClearGameCaptions	= false;
	}
	m_window->Update();

	//update windows
	if( GameIndicatorsShown() && psHUD_Flags.is(HUD_DRAW|HUD_DRAW_RT) )
		UIMainIngameWnd->Update	();

	m_pMessagesWnd->Update();
}

void CUIGameCustom::Render()
{
	GameCaptions()->Draw();

	st_vec_it it = m_custom_statics.begin();
	st_vec_it it_e = m_custom_statics.end();
	for(;it!=it_e;++it)
		(*it)->Draw();

	m_window->Draw();

	CEntity* pEntity = smart_cast<CEntity*>(Level().CurrentEntity());
	if (pEntity)
	{
		CActor* pActor = smart_cast<CActor*>(pEntity);
	        if (pActor && pActor->HUDview() && pActor->g_Alive() &&
	            psHUD_Flags.is(HUD_WEAPON | HUD_WEAPON_RT | HUD_WEAPON_RT2))
	        {

			CInventory& inventory = pActor->inventory();
			xr_vector<CInventorySlot>::iterator I = inventory.m_slots.begin();
			xr_vector<CInventorySlot>::iterator E = inventory.m_slots.end();

			for (u32 slot_idx=0 ; I != E; ++I,++slot_idx)
			{
		                CInventoryItem* item = inventory.ItemFromSlot(slot_idx);
		                if (item && item->render_item_ui_query())
		                    item->render_item_ui();
		        }
		}

		if( GameIndicatorsShown() && psHUD_Flags.is(HUD_DRAW | HUD_DRAW_RT) )
		{
			UIMainIngameWnd->Draw();
			m_pMessagesWnd->Draw();
		} else {  //hack - draw messagess wnd in scope mode
			if (!m_PdaMenu->GetVisible())
				m_pMessagesWnd->Draw();
		}	
	}
	else
		m_pMessagesWnd->Draw();

	DoRenderDialogs();
}

void CUIGameCustom::AddCustomMessage		(LPCSTR id, float x, float y, float font_size, CGameFont *pFont, u16 alignment, u32 color)
{
	GameCaptions()->addCustomMessage(id,x,y,font_size,pFont,(CGameFont::EAligment)alignment,color,"");
}

void CUIGameCustom::AddCustomMessage		(LPCSTR id, float x, float y, float font_size, CGameFont *pFont, u16 alignment, u32 color, float flicker )
{
	AddCustomMessage(id,x,y,font_size, pFont, alignment, color);
	GameCaptions()->customizeMessage(id, CUITextBanner::tbsFlicker)->fPeriod = flicker;
}

void CUIGameCustom::CustomMessageOut(LPCSTR id, LPCSTR msg, u32 color)
{
	GameCaptions()->setCaption(id,msg,color,true);
}

void CUIGameCustom::RemoveCustomMessage		(LPCSTR id)
{
	GameCaptions()->removeCustomMessage(id);
}


SDrawStaticStruct* CUIGameCustom::AddCustomStatic(LPCSTR id, bool bSingleInstance)
{
	if(bSingleInstance)
	{
		st_vec::iterator it = std::find_if(m_custom_statics.begin(),m_custom_statics.end(), predicate_find_stat(id) );
		if(it!=m_custom_statics.end())
			return (*it);
	}
	
	CUIXmlInit xml_init;
	m_custom_statics.push_back		( new SDrawStaticStruct() );
	SDrawStaticStruct* sss			= m_custom_statics.back();

	sss->m_static					= new CUIStatic();
	sss->m_name						= id;
	xml_init.InitStatic				(*m_msgs_xml, id, 0, sss->m_static);
	float ttl						= m_msgs_xml->ReadAttribFlt(id, 0, "ttl", -1);
	if(ttl>0.0f)
		sss->m_endTime				= Device.fTimeGlobal + ttl;

	return sss;
}

SDrawStaticStruct* CUIGameCustom::GetCustomStatic(LPCSTR id)
{
	st_vec::iterator it = std::find_if(m_custom_statics.begin(),m_custom_statics.end(), predicate_find_stat(id));
	if(it!=m_custom_statics.end())
		return (*it);

	return NULL;
}

void CUIGameCustom::RemoveCustomStatic(LPCSTR id)
{
	st_vec::iterator it = std::find_if(m_custom_statics.begin(),m_custom_statics.end(), predicate_find_stat(id) );
	if(it!=m_custom_statics.end())
	{
			delete_data				(*it);
		m_custom_statics.erase	(it);
	}
}

#include "ui/UIGameTutorial.h"

extern CUISequencer* g_tutorial;
extern CUISequencer* g_tutorial2;

void CUIGameCustom::reset_ui()
{
	if(g_tutorial2)
	{ 
		g_tutorial2->Destroy	();
		xr_delete				(g_tutorial2);
	}

	if(g_tutorial)
	{
		g_tutorial->Destroy	();
		xr_delete(g_tutorial);
	}
}

void CUIGameCustom::SetClGame(game_cl_GameState* g)
{
	g->SetGameUI(this);
}

void CUIGameCustom::UnLoad()
{
	xr_delete					(m_pgameCaptions);
	xr_delete					(m_msgs_xml);
	xr_delete					(m_window);
	xr_delete					(UIMainIngameWnd);
	xr_delete					(m_pMessagesWnd);
	xr_delete					(m_InventoryMenu);
	xr_delete					(m_PdaMenu);
	xr_delete					(m_UICarBodyMenu);
	xr_delete					(m_WeatherEditor);
}

void CUIGameCustom::Load()
{
	if(g_pGameLevel)
	{
		R_ASSERT				(NULL==m_pgameCaptions);
		m_pgameCaptions				= new CUICaption();

		R_ASSERT				(NULL==m_msgs_xml);
		m_msgs_xml				= new CUIXml();
		m_msgs_xml->Load		(CONFIG_PATH, UI_PATH, "ui_custom_msgs.xml");

		R_ASSERT				(NULL==m_PdaMenu);
		m_PdaMenu				= new CUIPdaWnd			();
		
		R_ASSERT				(NULL==m_window);
		m_window				= new CUIWindow			();

		R_ASSERT				(NULL==UIMainIngameWnd);
		UIMainIngameWnd			= new CUIMainIngameWnd	();
		UIMainIngameWnd->Init	();

		R_ASSERT				(NULL==m_InventoryMenu);
		m_InventoryMenu			= new CUIInventoryWnd	();

		R_ASSERT				(NULL==m_UICarBodyMenu);
		m_UICarBodyMenu			= new CUICarBodyWnd		();

		R_ASSERT				(NULL==m_pMessagesWnd);
		m_pMessagesWnd			= new CUIMessagesWindow();

		R_ASSERT				(NULL==m_WeatherEditor);
		m_WeatherEditor			= new CUIWeatherEditor();
		
		Init					(0);
		Init					(1);
		Init					(2);
	}
}

void CUIGameCustom::OnConnected()
{
	if(g_pGameLevel)
	{
		if(!UIMainIngameWnd)
			Load();

		UIMainIngameWnd->OnConnected();
	}
}

void CUIGameCustom::CommonMessageOut(LPCSTR text)
{
	m_pMessagesWnd->AddLogMessage(text);
}

SDrawStaticStruct::SDrawStaticStruct	()
{
	m_static	= NULL;
	m_endTime	= -1.0f;	
}

void SDrawStaticStruct::destroy()
{
	delete_data(m_static);
}

bool SDrawStaticStruct::IsActual() const
{
	if(m_endTime<0)			return true;
	return (Device.fTimeGlobal < m_endTime);
}

void SDrawStaticStruct::SetText(LPCSTR text)
{
	m_static->Show(text!=NULL);
	if(text)
	{
		m_static->TextItemControl()->SetTextST(text);
		m_static->ResetColorAnimation();
	}
}


void SDrawStaticStruct::Draw()
{
	if(m_static->IsShown())
		m_static->Draw();
}

void SDrawStaticStruct::Update()
{
	if(IsActual() && m_static->IsShown())	
		m_static->Update();
}