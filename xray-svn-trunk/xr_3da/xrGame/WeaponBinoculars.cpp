#include "stdafx.h"
#include "WeaponBinoculars.h"

#include "xr_level_controller.h"

#include "level.h"
#include "ui\UIFrameWindow.h"
#include "WeaponBinocularsVision.h"
#include "object_broker.h"
#include "hudmanager.h"
#include "inventory.h"

CWeaponBinoculars::CWeaponBinoculars() : CWeaponCustomPistol("BINOCULARS")
{
	m_binoc_vision	= NULL;
	m_bVision		= false;
	m_bautoDetectEntities = 0;
}

CWeaponBinoculars::~CWeaponBinoculars()
{
	xr_delete				(m_binoc_vision);
}

void CWeaponBinoculars::Load	(LPCSTR section)
{
	inherited::Load(section);

	// Sounds
	m_sounds.LoadSound(section, "snd_zoomin",  "sndZoomIn",		false, SOUND_TYPE_ITEM_USING);
	m_sounds.LoadSound(section, "snd_zoomout", "sndZoomOut",	false, SOUND_TYPE_ITEM_USING);
	if(pSettings->line_exist(section,"vision_present"))
		m_bVision = !!pSettings->r_bool(section,"vision_present");
	if (pSettings->line_exist(section, "auto_detect_entities"))
		m_bautoDetectEntities = !!pSettings->r_u32(section, "auto_detect_entities");
}


bool CWeaponBinoculars::Action(u16 cmd, u32 flags) 
{
	switch(cmd) 
	{
	case kWPN_FIRE : 
		return inherited::Action(kWPN_ZOOM, flags);
	}

	return inherited::Action(cmd, flags);
}

void CWeaponBinoculars::OnZoomIn		()
{
	if(H_Parent() && !IsZoomed())
	{
		m_sounds.StopSound("sndZoomOut");
		bool b_hud_mode = (Level().CurrentEntity() == H_Parent());
		m_sounds.PlaySound("sndZoomIn", H_Parent()->Position(), H_Parent(), b_hud_mode);
		if(m_bVision && !m_binoc_vision) 
		{
			//.VERIFY			(!m_binoc_vision);
			m_binoc_vision	= new CBinocularsVision(cNameSect());
			m_binoc_vision->m_bautoDetectEntities = m_bautoDetectEntities;
		}
	}
	inherited::OnZoomIn		();
}

void CWeaponBinoculars::OnZoomOut		()
{
	if(H_Parent() && IsZoomed() && !IsRotatingToZoom())
	{
		m_sounds.StopSound("sndZoomIn");
		bool b_hud_mode = (Level().CurrentEntity() == H_Parent());	
		m_sounds.PlaySound("sndZoomOut", H_Parent()->Position(), H_Parent(), b_hud_mode);
		VERIFY			(m_binoc_vision);
		xr_delete		(m_binoc_vision);
	}


	inherited::OnZoomOut();
}

BOOL	CWeaponBinoculars::net_Spawn			(CSE_Abstract* DC)
{
	inherited::net_Spawn	(DC);
	return					TRUE;
}

void	CWeaponBinoculars::net_Destroy()
{
	inherited::net_Destroy();
	xr_delete(m_binoc_vision);
}

void	CWeaponBinoculars::UpdateCL()
{
	inherited::UpdateCL();
	//manage visible entities here...
	if(H_Parent() && IsZoomed() && !IsRotatingToZoom() && m_binoc_vision)
		m_binoc_vision->Update();
}

bool CWeaponBinoculars::render_item_ui_query()
{
	bool b_is_active_item = m_pCurrentInventory->ActiveItem()==this;
	return b_is_active_item && H_Parent() && IsZoomed() && !IsRotatingToZoom() && m_binoc_vision;
}

void CWeaponBinoculars::render_item_ui()
{
	m_binoc_vision->Draw();
	inherited::render_item_ui	();
}

void GetZoomData(const float scope_factor, float& delta, float& min_zoom_factor)
{
	float def_fov = float(g_fov);
	float min_zoom_k = 0.3f;
	float zoom_step_count = 3.0f;
	float delta_factor_total = def_fov-scope_factor;
	VERIFY(delta_factor_total>0);
	min_zoom_factor = def_fov-delta_factor_total*min_zoom_k;
	delta = (delta_factor_total*(1-min_zoom_k) )/zoom_step_count;

}

void CWeaponBinoculars::ZoomInc()
{
	float delta,min_zoom_factor;
	GetZoomData(m_zoom_params.m_fScopeZoomFactor, delta, min_zoom_factor);

	float f					= GetZoomFactor()-delta;
	clamp					(f,m_zoom_params.m_fScopeZoomFactor,min_zoom_factor);
	SetZoomFactor			( f );
}

void CWeaponBinoculars::ZoomDec()
{
	float delta,min_zoom_factor;
	GetZoomData(m_zoom_params.m_fScopeZoomFactor,delta,min_zoom_factor);

	float f					= GetZoomFactor()+delta;
	clamp					(f,m_zoom_params.m_fScopeZoomFactor,min_zoom_factor);
	SetZoomFactor			( f );

}

//tatarinrafa: Cweapon alredy has this

//void CWeaponBinoculars::save(NET_Packet &output_packet)
//{
//	inherited::save(output_packet);
//	save_data		(m_fRTZoomFactor,output_packet);
//}

//void CWeaponBinoculars::load(IReader &input_packet)
//{
//	inherited::load(input_packet);
//	load_data		(m_fRTZoomFactor,input_packet);
//}

void CWeaponBinoculars::GetBriefInfo(xr_string& str_name, xr_string& icon_sect_name, xr_string& str_count)
{
	str_name		= NameShort();
	str_count		= "";
	icon_sect_name	= *cNameSect();
}

void CWeaponBinoculars::net_Relcase	(CObject *object)
{
	if (!m_binoc_vision)
		return;

	m_binoc_vision->remove_links	(object);
}
