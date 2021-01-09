#include "stdafx.h"
#include "UIMainIngameWnd.h"
#include "UIMotionIcon.h"
#include "UIXmlInit.h"
#include "../HUDManager.h"

CUIMotionIcon::CUIMotionIcon()
{
	m_bchanged		= false;
	m_luminosity	= 0.0f;
}

CUIMotionIcon::~CUIMotionIcon()
{

}

void CUIMotionIcon::ResetVisibility()
{
	m_npc_visibility.clear	();
	m_bchanged				= true;
}

void CUIMotionIcon::Init()
{
	CUIXml		uiXml;

	string128		MOTION_ICON_XML;
	xr_sprintf		(MOTION_ICON_XML, "motion_icon_%d.xml", ui_hud_type);

	bool result = uiXml.Load(CONFIG_PATH, UI_PATH, MOTION_ICON_XML);
	R_ASSERT3(result, "xml file not found", MOTION_ICON_XML);

	CUIXmlInit	xml_init;

	xml_init.InitStatic			(uiXml, "background", 0, this);	

	AttachChild					(&m_power_progress);
	xml_init.InitProgressBar	(uiXml, "power_progress", 0, &m_power_progress);	

	AttachChild					(&m_luminosity_progress);
	xml_init.InitProgressBar	(uiXml, "luminosity_progress", 0, &m_luminosity_progress);	

	AttachChild					(&m_noise_progress);
	xml_init.InitProgressBar	(uiXml, "noise_progress", 0, &m_noise_progress);	


}

void CUIMotionIcon::SetPower(float Pos)
{
	m_power_progress.SetProgressPos(Pos);
}

void CUIMotionIcon::SetNoise(float Pos)
{
	Pos	= clampr(Pos, m_noise_progress.GetRange_min(), m_noise_progress.GetRange_max());
	m_noise_progress.SetProgressPos(Pos);
}

void CUIMotionIcon::SetLuminosity(float Pos)
{
	Pos						= clampr(Pos, m_luminosity_progress.GetRange_min(), m_luminosity_progress.GetRange_max());
	m_luminosity			= Pos;
}

void CUIMotionIcon::Update()
{
	if(m_bchanged){
		m_bchanged = false;
		if( m_npc_visibility.size() )
		{
			std::sort					(m_npc_visibility.begin(), m_npc_visibility.end());
			SetLuminosity				(m_npc_visibility.back().value);
		}else
			SetLuminosity				(m_luminosity_progress.GetRange_min() );
	}
	inherited::Update();
	
	//m_luminosity_progress 
	{
		float len					= m_noise_progress.GetRange_max()-m_noise_progress.GetRange_min();
		float cur_pos				= m_luminosity_progress.GetProgressPos();
		if(cur_pos!=m_luminosity){
			float _diff = _abs(m_luminosity-cur_pos);
			if(m_luminosity>cur_pos){
				cur_pos				+= _min(len*Device.fTimeDelta, _diff);
			}else{
				cur_pos				-= _min(len*Device.fTimeDelta, _diff);
			}
			clamp(cur_pos, m_noise_progress.GetRange_min(), m_noise_progress.GetRange_max());
			m_luminosity_progress.SetProgressPos(cur_pos);
		}
	}
}

void CUIMotionIcon::SetActorVisibility		(u16 who_id, float value)
{
	float v		= float(m_luminosity_progress.GetRange_max() - m_luminosity_progress.GetRange_min());
	value		*= v;
	value		+= m_luminosity_progress.GetRange_min();

	xr_vector<_npc_visibility>::iterator it = std::find(m_npc_visibility.begin(), 
														m_npc_visibility.end(),
														who_id);

	if(it==m_npc_visibility.end() && value!=0)
	{
		m_npc_visibility.resize	(m_npc_visibility.size()+1);
		_npc_visibility& v		= m_npc_visibility.back();
		v.id					= who_id;
		v.value					= value;
	}
	else if( fis_zero(value) )
	{
		if (it!=m_npc_visibility.end())
			m_npc_visibility.erase	(it);
	}
	else
	{
		(*it).value				= value;
	}

	m_bchanged = true;
}
