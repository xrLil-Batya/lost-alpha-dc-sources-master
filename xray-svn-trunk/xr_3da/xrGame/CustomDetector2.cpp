#include "stdafx.h"
#include "customdetector2.h"
#include "ui/ArtefactDetectorUI.h"

#include "inventory.h"

#include "actor.h"

#include "player_hud.h"
#include "weapon.h"


bool CCustomDetectorR::CheckCompatibilityInt(CHudItem* itm, u16* slot_to_activate) //Проверяет, можно ли достать
{

	if (itm == NULL)
	{
		return true;
	}

	CInventoryItem& iitm			= itm->item();
	u32 slot						= iitm.GetSlot();
	bool bres = false;

	//Msg("CheckCompatibilityInt:start, %s, %u, nextactiveslot %u, activeslot %u", itm->object().cNameSect_str(), iitm.GetSlot(), m_pCurrentInventory->GetNextActiveSlot(), m_pCurrentInventory->GetActiveSlot());

	CInventoryItem* invitemfromslot = m_pCurrentInventory->ItemFromSlot(slot);

	if (slot == DETECTOR_SLOT || (invitemfromslot && invitemfromslot->TrueFalseSingleHand()))
	{
		bres = true;
	}
	else if (GetState() != eHiding)
	{
		m_pCurrentInventory->Activate(BOLT_SLOT);
		m_bNeedActivation = true;
		bres = true;
	}

	if(bres)
	{
		CWeapon* W = smart_cast<CWeapon*>(itm);
		if (W)
		{
			bres = bres && (W->GetState() != CHUDState::eBore) && (W->GetState() != CHUDState::eHiding) && (W->GetState() != CWeapon::eReload) && (W->GetState() != CWeapon::eSwitch) && !W->IsZoomed();
		}
	}
	return bres;
}

void CCustomDetectorR::ToggleDetector(bool bFastMode)
{
	m_bNeedActivation		= false;
	m_bFastAnimMode			= bFastMode;

	attachable_hud_item* i0		= g_player_hud->attached_item(0);

	if(GetState()==eHidden)
	{
		PIItem iitem = m_pCurrentInventory->ActiveItem();
		CHudItem* itm = (iitem)?iitem->cast_hud_item():NULL;
		u16 slot_to_activate = (u16)NO_ACTIVE_SLOT;

		if(CheckCompatibilityInt(itm, &slot_to_activate))
		{
			if(slot_to_activate!=NO_ACTIVE_SLOT && true==false)
			{
				m_pCurrentInventory->Activate(slot_to_activate);
				m_bNeedActivation		= true;
			}else
			{
				if (m_bNeedActivation == false){
					SwitchState(eShowing);
					TurnDetectorInternal(true);

					m_pCurrentInventory->SetCurrentDetector(this);

				}
			}
		}
	}
	else
		if (GetState() == eIdle){
			SwitchState(eHiding);
			m_pCurrentInventory->SetCurrentDetector(NULL);
		}
}


void CCustomDetectorR::OnStateSwitch(u32 S)
{
	inherited::OnStateSwitch(S);

	switch(S)
	{
	case eShowing:
		{
			g_player_hud->attach_item	(this);
			m_sounds.PlaySound			("sndShow", Fvector().set(0,0,0), this, true, false);
			PlayHUDMotion				("anim_show_fast", FALSE, this, GetState());
			SetPending					(TRUE);

		}break;
	case eHiding:
		{
			m_sounds.PlaySound			("sndHide", Fvector().set(0,0,0), this, true, false);
			PlayHUDMotion				(m_bFastAnimMode?"anim_hide_fast":"anim_hide", FALSE/*TRUE*/, this, GetState());
			SetPending					(TRUE);

		}break;
	case eIdle:
		{
			PlayAnimIdle				();
			SetPending					(FALSE);
		}break;
}
}

void CCustomDetectorR::OnAnimationEnd(u32 state)
{
	inherited::OnAnimationEnd	(state);
	switch(state)
	{
	case eShowing:
		{
			SwitchState					(eIdle);
		} break;
	case eHiding:
		{
			SwitchState					(eHidden);
			TurnDetectorInternal		(false);
			g_player_hud->detach_item	(this);
			u32 activateweapon = m_pCurrentInventory->Getneedtoactivateweapon();			
			if (activateweapon == 1 || activateweapon == 2 || activateweapon == 3 || activateweapon == 4 || activateweapon == 5 || activateweapon == 6){
				activateweapon -= 1;//чтобы избежать случаи активации ножа
				m_pCurrentInventory->SetCurrentDetector(NULL);
				m_pCurrentInventory->Activate(activateweapon);
				m_pCurrentInventory->Setneedtoactivateweapon (-1);
			}
		} break;
	}
}

void CCustomDetectorR::UpdateXForm()
{
	CInventoryItem::UpdateXForm();
}

void CCustomDetectorR::OnActiveItem()
{
	return;
}

void CCustomDetectorR::OnHiddenItem()
{
}

CCustomDetectorR::CCustomDetectorR() 
{
	m_ui				= NULL;
	m_bFastAnimMode		= false;
	m_bNeedActivation	= false;
}

CCustomDetectorR::~CCustomDetectorR() 
{
	//m_artefacts.destroy		();
	TurnDetectorInternal	(false);
	xr_delete				(m_ui);
}

BOOL CCustomDetectorR::net_Spawn(CSE_Abstract* DC) 
{
	TurnDetectorInternal(false);
	return		(inherited::net_Spawn(DC));
}

void CCustomDetectorR::Load(LPCSTR section) 
{

	inherited::Load			(section);

	fdetect_radius			= pSettings->r_float(section,"detect_radius");//Считываем базовый радиус действия
	foverallrangetocheck	= READ_IF_EXISTS(pSettings, r_float, section, "overall_range_to_check", 50.0f);
	fortestrangetocheck		= READ_IF_EXISTS(pSettings, r_float, section, "test_range_to_check", 250.0f);
	for_test				= READ_IF_EXISTS(pSettings, r_u32, section, "for_test", 0);
	reaction_sound_off		= !!READ_IF_EXISTS(pSettings, r_u8, section, "reaction_sound_off", 0);

	// Подгрузим список всех артов, список прописан в линии af_to_detect_list. Пока будет так, так как по другому сделать не хватает опыта
	xr_vector<shared_str>			af_total_list;

	LPCSTR	S = READ_IF_EXISTS(pSettings, r_string, section, "af_to_detect_list", "null");

	if (S && S[0])
	{
		string128		artefact;
		int				count = _GetItemCount(S);
		for (int it = 0; it < count; ++it)
		{
			_GetItem(S, it, artefact);
			af_total_list.push_back(artefact);
		}
	}


	// Отсеим только те арты, которые построчно прописаны в конфиге детектора
	af_types.clear();

	for (int it = 0; it < af_total_list.size(); ++it)
	{
		u32 af_line_true_false;
		af_line_true_false = READ_IF_EXISTS(pSettings, r_u32, section, af_total_list[it].c_str(), 0);
		if (af_line_true_false == 1 || (xr_strcmp(af_total_list[it].c_str(), "all") == 0)){
			af_types.push_back(af_total_list[it]);
		}
	}

	//Для проверки
	//for (int it = 0; it < af_types.size(); ++it)
	//{
		//Msg("af_types%i == %s, it was marked true", it, af_types[it].c_str());
	//}


	m_sounds.LoadSound( section, "snd_draw", "sndShow");
	m_sounds.LoadSound( section, "snd_holster", "sndHide");

	//Этот звук берется по дефолту если не задана строка "sounds"
	HUD_SOUND_ITEM::LoadSound(section, "af_sound_if_no_artefact_sound", detect_sndsnd, SOUND_TYPE_ITEM);

	//Добавил врзможность задать разные звуки для различных артов
	af_sounds_section = READ_IF_EXISTS(pSettings, r_string, section, "sounds", "null");

	detector_section = section;

}


void CCustomDetectorR::shedule_Update(u32 dt) 
{
	inherited::shedule_Update(dt);
	
	if( !IsWorking() )			return;

	Position().set(H_Parent()->Position());

	Fvector						P; 
	P.set						(H_Parent()->Position());
}


bool CCustomDetectorR::IsWorking()
{
	return m_bWorking && H_Parent() && H_Parent()==Level().CurrentViewEntity();
}

void CCustomDetectorR::UpdateCL() 
{
	inherited::UpdateCL();

	if(H_Parent()!=Level().CurrentEntity() )			return;

	UpdateVisibility		();
	if( !IsWorking() )		return;
	UpfateWork				();
}

void CCustomDetectorR::OnH_A_Chield() 
{
	inherited::OnH_A_Chield		();
}

void CCustomDetectorR::OnH_B_Independent(bool just_before_destroy) 
{
	inherited::OnH_B_Independent(just_before_destroy);
}


void CCustomDetectorR::OnMoveToRuck()
{

	m_pCurrentInventory->SetCurrentDetector(NULL);
	SwitchState					(eHidden);
	g_player_hud->detach_item	(this);

	TurnDetectorInternal			(false);
	StopCurrentAnimWithoutCallback	();
}

void CCustomDetectorR::OnMoveToSlot()
{

}

void CCustomDetectorR::TurnDetectorInternal(bool b)
{
	m_bWorking				= b;
	if(b && m_ui==NULL)
	{
		CreateUI			();
	}else
	{
		xr_delete			(m_ui);
	}

}
///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool  CCustomDetectorR::CheckCompatibility(CHudItem* itm)//вроди не нужен
{
	return true;
}

void CCustomDetectorR::HideDetector(bool bFastMode)
{
	if(GetState()==eIdle)
		ToggleDetector(bFastMode);
}

void CCustomDetectorR::ShowDetector(bool bFastMode)
{

	bool toggle = true;

	if (GetState() == eHidden && toggle == true)
		ToggleDetector(bFastMode);
}

void CCustomDetectorR::UpfateWork() 
{
	UpdateAf				();
	m_ui->update			();
}

void CCustomDetectorR::UpdateVisibility()//Подгонка нахождения детектора в левой руке под различные события худа
{

	attachable_hud_item* i0 = g_player_hud->attached_item(0);

	
	if (i0 && HudItemData() && m_pCurrentInventory->CurrentDetector() != NULL)
	{
		// если оружие в правой руке перезаряжается или увеличение или смена патрона
		{
			CWeapon* wpn			= smart_cast<CWeapon*>(i0->m_parent_hud_item);
			if(wpn)
			{
				u32 state			= wpn->GetState();
				if(wpn->IsZoomed() || state==CWeapon::eReload || state==CWeapon::eSwitch) 
				{
					OnMoveToRuck();
					m_bNeedActivation	= true;
				}
			}
		}

		//----------------------
		CInventoryItem* invitem = smart_cast<CInventoryItem*>(i0->m_parent_hud_item);
		u32 slot = invitem->GetSlot();
		CInventoryItem* invitemfromslot = m_pCurrentInventory->ItemFromSlot(slot);

		if (slot != DETECTOR_SLOT && (invitemfromslot && !invitemfromslot->TrueFalseSingleHand()))
		{
			OnMoveToRuck();
		}
	}else
		if (m_bNeedActivation)
	{
		attachable_hud_item* i0		= g_player_hud->attached_item(0);
		bool bClimb					= ( (Actor()->MovingState()&mcClimb) != 0 );
		if(!bClimb)
		{
			CHudItem* huditem		= (i0)?i0->m_parent_hud_item : NULL;
			bool bChecked			= !huditem || CheckCompatibilityInt(huditem, 0);
			if (bChecked){
				ShowDetector(true);
			}
		}
	}
}


bool CCustomDetectorR::install_upgrade_impl(LPCSTR section, bool test)
{
	Msg("Detector Upgrade");
	bool result = inherited::install_upgrade_impl(section, test);
	
	//Msg("Detecting radius before %f", fdetect_radius);
	result |= process_if_exists(section, "detect_radius", &CInifile::r_float, fdetect_radius, test);
	//Msg("Detecting radius after %f", fdetect_radius);
	result |= process_if_exists(section, "overall_range_to_check", &CInifile::r_float, foverallrangetocheck, test);
	result |= process_if_exists(section, "test_range_to_check", &CInifile::r_float, fortestrangetocheck, test);
	result |= process_if_exists(section, "for_test", &CInifile::r_u32, for_test, test);
	result |= process_if_exists(section, "reaction_sound_off", &CInifile::r_u8, reaction_sound_off, test);
	result |= process_if_exists_set(section, "sounds", &CInifile::r_string, af_sounds_section, test);

	LPCSTR str;
	bool result2 = process_if_exists_set(section, "af_sound_if_no_artefact_sound", &CInifile::r_string, str, test);
	if (result2 && !test)
	{
		HUD_SOUND_ITEM::LoadSound(section, "af_sound_if_no_artefact_sound", detect_sndsnd, SOUND_TYPE_ITEM);
	}

	result |= result2;

	//Для проверки
	//for (int it = 0; it < af_types.size(); ++it)
	//{
	//	Msg("Afs After Upgrade af_types%i == %s, it is marked true", it, af_types[it].c_str());
	//}

	// Подгрузим список всех артов, список прописан в линии af_to_detect_list. Пока будет так, так как по другому сделать не хватает опыта
	xr_vector<shared_str>			af_total_list;

	LPCSTR	S = READ_IF_EXISTS(pSettings, r_string, detector_section, "af_to_detect_list", "null");

	if (S && S[0])
	{
		string128		artefact;
		int				count = _GetItemCount(S);
		for (int it = 0; it < count; ++it)
		{
			_GetItem(S, it, artefact);
			af_total_list.push_back(artefact);
		}
	}

	// Отсеим только те арты, которые построчно прописаны в конфиге апгрейда и добавим их в список видимых для детектора af_types

	for (int it = 0; it < af_total_list.size(); ++it)
	{
		u32 af_line_true_false = 0;
		//Для проверки

		//Msg("Afs processing af_total_list %i == %s", it, af_total_list[it].c_str());

		result2 = process_if_exists_set(section, af_total_list[it].c_str(), &CInifile::r_u32, af_line_true_false, test);

		if (af_line_true_false == 1){
			af_types.push_back(af_total_list[it]);
			//Msg("found true %s", af_total_list[it].c_str());
		}
	}

	///Для проверки
	//for (int it = 0; it < af_types.size(); ++it)
	//{
	//	Msg("Afs After Upgrade af_types%i == %s, it was marked true", it, af_types[it].c_str());
	//}

	result |= result2;

	return result;
}
