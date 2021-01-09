#include "stdafx.h"

#include "customoutfit.h"
#include "PhysicsShell.h"
#include "inventory_space.h"
#include "Inventory.h"
#include "Actor.h"
#include "game_cl_base.h"
#include "Level.h"
#include "BoneProtections.h"
#include "../../Include/xrRender/Kinematics.h"
#include "../../Include/xrRender/RenderVisual.h"
#include "ai_sounds.h"
#include "actorEffector.h"
#include "player_hud.h"

CCustomOutfit::CCustomOutfit()
{
	m_slot = OUTFIT_SLOT;

	m_flags.set(FUsingCondition, TRUE);

	m_HitTypeProtection.resize(ALife::eHitTypeMax);
	for(int i=0; i<ALife::eHitTypeMax; i++)
		m_HitTypeProtection[i] = 1.0f;

	m_boneProtection = new SBoneProtections();
	m_BonesProtectionSect = NULL;
}

CCustomOutfit::~CCustomOutfit() 
{
	xr_delete(m_boneProtection);

	/*HUD_SOUND_ITEM::DestroySound	(m_NightVisionOnSnd);
	HUD_SOUND_ITEM::DestroySound	(m_NightVisionOffSnd);
	HUD_SOUND_ITEM::DestroySound	(m_NightVisionIdleSnd);
	HUD_SOUND_ITEM::DestroySound	(m_NightVisionBrokenSnd);*/
}

void CCustomOutfit::net_Export(NET_Packet& P)
{
	inherited::net_Export	(P);
	P.w_float_q8			(m_fCondition,0.0f,1.0f);
	//P.w_u8(m_bNightVisionOn ? 1 : 0);
}

void CCustomOutfit::net_Import(NET_Packet& P)
{
	inherited::net_Import	(P);
	P.r_float_q8			(m_fCondition,0.0f,1.0f);
	/*bool new_bNightVisionOn = !!P.r_u8();

	if (new_bNightVisionOn != m_bNightVisionOn)	
		SwitchNightVision(new_bNightVisionOn);*/
}

void CCustomOutfit::Load(LPCSTR section) 
{
	inherited::Load(section);

	m_HitTypeProtection[ALife::eHitTypeBurn]		= pSettings->r_float(section,"burn_protection");
	m_HitTypeProtection[ALife::eHitTypeStrike]		= pSettings->r_float(section,"strike_protection");
	m_HitTypeProtection[ALife::eHitTypeShock]		= pSettings->r_float(section,"shock_protection");
	m_HitTypeProtection[ALife::eHitTypeWound]		= pSettings->r_float(section,"wound_protection");
	m_HitTypeProtection[ALife::eHitTypeRadiation]	= pSettings->r_float(section,"radiation_protection");
	m_HitTypeProtection[ALife::eHitTypeTelepatic]	= pSettings->r_float(section,"telepatic_protection");
	m_HitTypeProtection[ALife::eHitTypeChemicalBurn]= pSettings->r_float(section,"chemical_burn_protection");
	m_HitTypeProtection[ALife::eHitTypeExplosion]	= pSettings->r_float(section,"explosion_protection");
	m_HitTypeProtection[ALife::eHitTypeFireWound]	= pSettings->r_float(section,"fire_wound_protection");
	m_HitTypeProtection[ALife::eHitTypePhysicStrike]= READ_IF_EXISTS(pSettings, r_float, section, "physic_strike_protection", 0.0f);
	m_boneProtection->m_fHitFracActor = READ_IF_EXISTS(pSettings, r_float, section, "hit_fraction_actor", 0.0f); // pSettings->r_float(section, "hit_fraction_actor");

	if (pSettings->line_exist(section, "actor_visual"))
		m_ActorVisual = pSettings->r_string(section, "actor_visual");
	else
		m_ActorVisual = NULL;

	if (pSettings->line_exist(section, "actor_visual_legs"))
		m_ActorVisual_legs = pSettings->r_string(section, "actor_visual_legs");
	else
		m_ActorVisual_legs = NULL;

	m_ef_equipment_type		= pSettings->r_u32(section,"ef_equipment_type");
	if (pSettings->line_exist(section, "power_loss"))
		m_fPowerLoss = pSettings->r_float(section, "power_loss");
	else
		m_fPowerLoss = 1.0f;	

	m_additional_weight				= pSettings->r_float(section,"additional_inventory_weight");
	m_additional_weight2			= pSettings->r_float(section,"additional_inventory_weight2");

		//tatarinrafa: added additional jump speed sprint speed walk speed
	m_additional_jump_speed		= READ_IF_EXISTS(pSettings, r_float, section, "additional_jump_speed", 0.0f);
	m_additional_run_coef		= READ_IF_EXISTS(pSettings, r_float, section, "additional_run_coef", 0.0f);
	m_additional_sprint_koef	= READ_IF_EXISTS(pSettings, r_float, section, "additional_sprint_koef", 0.0f);

	if (pSettings->line_exist(section, "nightvision_sect"))
		m_NightVisionSect = pSettings->r_string(section, "nightvision_sect");
	else
		m_NightVisionSect = NULL;

	block_pnv_slot						= READ_IF_EXISTS(pSettings, r_u32, section, "block_pnv_slot", 0);
	block_helmet_slot					= READ_IF_EXISTS(pSettings, r_u32, section, "block_helmet_slot", 0);
/*
	m_bNightVisionEnabled	= !!pSettings->r_bool(section,"night_vision");
	if(m_bNightVisionEnabled)
	{
		HUD_SOUND_ITEM::LoadSound(section,"snd_night_vision_on"	, m_NightVisionOnSnd	, SOUND_TYPE_ITEM_USING);
		HUD_SOUND_ITEM::LoadSound(section,"snd_night_vision_off"	, m_NightVisionOffSnd	, SOUND_TYPE_ITEM_USING);
		HUD_SOUND_ITEM::LoadSound(section,"snd_night_vision_idle", m_NightVisionIdleSnd	, SOUND_TYPE_ITEM_USING);
		HUD_SOUND_ITEM::LoadSound(section,"snd_night_vision_broken", m_NightVisionBrokenSnd, SOUND_TYPE_ITEM_USING);
	}
*/
	m_full_icon_name								= pSettings->r_string(section,"full_icon_name");
	m_BonesProtectionSect	= READ_IF_EXISTS(pSettings, r_string, section, "bones_koeff_protection",  "" );
}

//void CCustomOutfit::SwitchNightVision()
//{
//	if (OnClient()) return;
//	SwitchNightVision(!m_bNightVisionOn);	
//}

//void CCustomOutfit::SwitchNightVision(bool vision_on)
//{
//	if(!m_bNightVisionEnabled) return;
//	
//	m_bNightVisionOn = vision_on;
//
//	CActor *pA = smart_cast<CActor*>(H_Parent());
//
//	if(!pA)					return;
//	bool bPlaySoundFirstPerson = (pA == Level().CurrentViewEntity());
//
//	LPCSTR disabled_names	= pSettings->r_string(cNameSect(),"disabled_maps");
//	LPCSTR curr_map			= *Level().name();
//	u32 cnt					= _GetItemCount(disabled_names);
//	bool b_allow			= true;
//	string512				tmp;
//	for(u32 i=0; i<cnt;++i)
//	{
//		_GetItem(disabled_names, i, tmp);
//		if(0==stricmp(tmp, curr_map))
//		{
//			b_allow = false;
//			break;
//		}
//	}
//
//	if(m_NightVisionSect.size()&&!b_allow)
//	{
//		HUD_SOUND_ITEM::PlaySound(m_NightVisionBrokenSnd, pA->Position(), pA, bPlaySoundFirstPerson);
//		return;
//	}
//
//	if(m_bNightVisionOn)
//	{
//		CEffectorPP* pp = pA->Cameras().GetPPEffector((EEffectorPPType)effNightvision);
//		if(!pp)
//		{
//			if (m_NightVisionSect.size())
//			{
//				AddEffector(pA,effNightvision, m_NightVisionSect);
//				HUD_SOUND_ITEM::PlaySound(m_NightVisionOnSnd, pA->Position(), pA, bPlaySoundFirstPerson);
//				HUD_SOUND_ITEM::PlaySound(m_NightVisionIdleSnd, pA->Position(), pA, bPlaySoundFirstPerson, true);
//			}
//		}
//	} else {
// 		CEffectorPP* pp = pA->Cameras().GetPPEffector((EEffectorPPType)effNightvision);
//		if(pp)
//		{
//			pp->Stop			(1.0f);
//			HUD_SOUND_ITEM::PlaySound(m_NightVisionOffSnd, pA->Position(), pA, bPlaySoundFirstPerson);
//			HUD_SOUND_ITEM::StopSound(m_NightVisionIdleSnd);
//		}
//	}
//}

void CCustomOutfit::net_Destroy() 
{
	//SwitchNightVision		(false);

	inherited::net_Destroy	();
}

BOOL CCustomOutfit::net_Spawn(CSE_Abstract* DC)
{
	if (IsGameTypeSingle())
		ReloadBonesProtection();
	//SwitchNightVision		(false);
	return inherited::net_Spawn(DC);
}

void CCustomOutfit::OnH_B_Independent	(bool just_before_destroy) 
{
	inherited::OnH_B_Independent	(just_before_destroy);

	/*SwitchNightVision			(false);

	HUD_SOUND_ITEM::StopSound		(m_NightVisionOnSnd);
	HUD_SOUND_ITEM::StopSound		(m_NightVisionOffSnd);
	HUD_SOUND_ITEM::StopSound		(m_NightVisionIdleSnd);*/
}

void CCustomOutfit::OnH_A_Chield()
{
	inherited::OnH_A_Chield();
}

void CCustomOutfit::Hit(float hit_power, ALife::EHitType hit_type)
{
	hit_power *= GetHitImmunity(hit_type);
	ChangeCondition(-hit_power);
}

float CCustomOutfit::GetDefHitTypeProtection(ALife::EHitType hit_type)
{
	return m_HitTypeProtection[hit_type]*GetCondition();
}

float CCustomOutfit::GetHitTypeProtection(ALife::EHitType hit_type, s16 element)
{
	float fBase = m_HitTypeProtection[hit_type]*GetCondition();
	float bone = m_boneProtection->getBoneProtection(element);
	return fBase*bone;
}

//tatarinrafa: Берем ситсему из ЗП

float CCustomOutfit::GetBoneArmor(s16 element)
{
	return m_boneProtection->getBoneArmor(element);
}

float CCustomOutfit::HitThroughArmor(float hit_power, s16 element, float ap, bool& add_wound, ALife::EHitType hit_type)
{
	float NewHitPower = hit_power;
	if (hit_type == ALife::eHitTypeFireWound)
	{
		float ba = GetBoneArmor(element);
		if (ba<0.0f)
			return NewHitPower;

		float BoneArmor = ba*GetCondition();
		if (/*!fis_zero(ba, EPS) && */(ap > BoneArmor))
		{
			//пуля пробила бронь
			float d_hit_power = (ap - BoneArmor) / ap;
			if (d_hit_power < m_boneProtection->m_fHitFracActor)
			{
				d_hit_power = m_boneProtection->m_fHitFracActor;
			}			
			NewHitPower *= d_hit_power;

			VERIFY(NewHitPower >= 0.0f);
		}
		else
		{
			//пуля НЕ пробила бронь
			NewHitPower *= m_boneProtection->m_fHitFracActor;
			add_wound = false; 	//раны нет
		}
	}
	else
	{
		float one = 0.1f;
		if (hit_type == ALife::eHitTypeStrike ||
			hit_type == ALife::eHitTypeWound ||
			hit_type == ALife::eHitTypeWound_2 ||
			hit_type == ALife::eHitTypeExplosion)
		{
			one = 1.0f;
		}
		float protect = GetDefHitTypeProtection(hit_type);
		NewHitPower -= protect * one;
		if (NewHitPower < 0.f)
			NewHitPower = 0.f;
	}
	//увеличить изношенность костюма
	Hit(hit_power, hit_type);
	//Msg("Hit Through Outfit: new hit power = %f; old hit power = %f; hit_type = %u, ammopiercing = %f; element = %i, bonearmor = %f", NewHitPower, hit_power, hit_type, element, GetBoneArmor(element));
	return NewHitPower;
}

BOOL	CCustomOutfit::BonePassBullet					(int boneID)
{
	return m_boneProtection->getBonePassBullet(s16(boneID));
};

void	CCustomOutfit::OnMoveToSlot		()
{
	if (m_pCurrentInventory)
	{
		CActor* pActor = smart_cast<CActor*> (m_pCurrentInventory->GetOwner());
		if (pActor)
		{
			//SwitchNightVision(false);

			if (pActor->IsFirstEye() && IsGameTypeSingle() && !pActor->IsActorShadowsOn())
			{
				if (m_ActorVisual_legs.size())
				{
						shared_str NewVisual = m_ActorVisual_legs;
						pActor->ChangeVisual(NewVisual);

						if (pActor == Level().CurrentViewEntity())	
							g_player_hud->load(pSettings->r_string(cNameSect(),"player_hud_section"));

				} else {
						shared_str NewVisual = pActor->GetDefaultVisualOutfit_legs();
						pActor->ChangeVisual(NewVisual);

						if (pActor == Level().CurrentViewEntity())
							g_player_hud->load_default();
				}
				if(pSettings->line_exist(cNameSect(),"bones_koeff_protection")){
					m_boneProtection->reload( pSettings->r_string(cNameSect(),"bones_koeff_protection"), smart_cast<IKinematics*>(pActor->Visual()) );
				};
			} else {
				if (m_ActorVisual.size())
				{
					shared_str NewVisual = NULL;
					char* TeamSection = Game().getTeamSection(pActor->g_Team());
					if (TeamSection)
					{
						if (pSettings->line_exist(TeamSection, *cNameSect()))
						{
							NewVisual = pSettings->r_string(TeamSection, *cNameSect());
							string256 SkinName;
							xr_strcpy(SkinName, pSettings->r_string("mp_skins_path", "skin_path"));
							xr_strcat(SkinName, *NewVisual);
							xr_strcat(SkinName, ".ogf");
							NewVisual._set(SkinName);
						}
					}
				
					if (!NewVisual.size())
						NewVisual = m_ActorVisual;
	
					pActor->ChangeVisual(NewVisual);

					if (pActor == Level().CurrentViewEntity())	
						g_player_hud->load(pSettings->r_string(cNameSect(),"player_hud_section"));

				} else {
					shared_str NewVisual = pActor->GetDefaultVisualOutfit();
					pActor->ChangeVisual(NewVisual);

					if (pActor == Level().CurrentViewEntity())	
						g_player_hud->load_default();
				}
				if(pSettings->line_exist(cNameSect(),"bones_koeff_protection")){
					m_boneProtection->reload( pSettings->r_string(cNameSect(),"bones_koeff_protection"), smart_cast<IKinematics*>(pActor->Visual()) );
				};
			}
		}
	}
};

void	CCustomOutfit::OnMoveToRuck		()
{
	if (m_pCurrentInventory)
	{
		CActor* pActor = smart_cast<CActor*> (m_pCurrentInventory->GetOwner());
		if (pActor)
		{
			CCustomOutfit* outfit	= pActor->GetOutfit();
			if (!outfit)
			{
				//pActor->SwitchNightVision();

				if (pActor->IsFirstEye() && IsGameTypeSingle())
				{
					shared_str DefVisual = pActor->GetDefaultVisualOutfit_legs();
					if (DefVisual.size())
					{
						pActor->ChangeVisual(DefVisual);
					}
				} else {
					shared_str DefVisual = pActor->GetDefaultVisualOutfit();
					if (DefVisual.size())
					{
						pActor->ChangeVisual(DefVisual);
					}
				}

				if (pActor == Level().CurrentViewEntity())
					g_player_hud->load_default();

			}
		}
	}
};

u32	CCustomOutfit::ef_equipment_type	() const
{
	return		(m_ef_equipment_type);
}

float CCustomOutfit::GetPowerLoss() 
{
	if (m_fPowerLoss<1 && GetCondition() <= 0)
	{
		return 1.0f;			
	};
	return m_fPowerLoss;
}


bool CCustomOutfit::install_upgrade_impl( LPCSTR section, bool test )
{
	bool result = inherited::install_upgrade_impl( section, test );

	result |= process_if_exists( section, "burn_protection",          &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeBurn]        , test );
	result |= process_if_exists( section, "shock_protection",         &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeShock]       , test );
	result |= process_if_exists( section, "strike_protection",        &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeStrike]      , test );
	result |= process_if_exists( section, "wound_protection",         &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeWound]       , test );
	result |= process_if_exists( section, "radiation_protection",     &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeRadiation]   , test );
	result |= process_if_exists( section, "telepatic_protection",     &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeTelepatic]   , test );
	result |= process_if_exists( section, "chemical_burn_protection", &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeChemicalBurn], test );
	result |= process_if_exists( section, "explosion_protection",     &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeExplosion]   , test );
	result |= process_if_exists( section, "fire_wound_protection",    &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeFireWound]   , test );
//	result |= process_if_exists( section, "physic_strike_protection", &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypePhysicStrike], test );
	LPCSTR str;
	bool result2 = process_if_exists_set( section, "nightvision_sect", &CInifile::r_string, str, test );
	if ( result2 && !test )
	{
		m_NightVisionSect._set( str );
	}
	result |= result2;

	result2 = process_if_exists_set( section, "bones_koeff_protection", &CInifile::r_string, str, test );
	if ( result2 && !test )
	{
		m_BonesProtectionSect	= str;
		ReloadBonesProtection	();
	}
	result2 = process_if_exists_set( section, "bones_koeff_protection_add", &CInifile::r_string, str, test );
	if ( result2 && !test )
		AddBonesProtection	(str);

	result |= result2;
	result |= process_if_exists( section, "hit_fraction_actor", &CInifile::r_float, m_boneProtection->m_fHitFracActor, test );
	
	result |= process_if_exists( section, "additional_inventory_weight",  &CInifile::r_float,  m_additional_weight,  test );
	result |= process_if_exists( section, "additional_inventory_weight2", &CInifile::r_float,  m_additional_weight2, test );
	/*
	result |= process_if_exists( section, "health_restore_speed",    &CInifile::r_float, m_fHealthRestoreSpeed,    test );
	result |= process_if_exists( section, "radiation_restore_speed", &CInifile::r_float, m_fRadiationRestoreSpeed, test );
	result |= process_if_exists( section, "satiety_restore_speed",   &CInifile::r_float, m_fSatietyRestoreSpeed,   test );
	result |= process_if_exists( section, "power_restore_speed",     &CInifile::r_float, m_fPowerRestoreSpeed,     test );
	result |= process_if_exists( section, "bleeding_restore_speed",  &CInifile::r_float, m_fBleedingRestoreSpeed,  test );
	*/
	result |= process_if_exists( section, "power_loss", &CInifile::r_float, m_fPowerLoss, test );
	clamp( m_fPowerLoss, 0.0f, 1.0f );

	result |= process_if_exists(section, "block_pnv_slot", &CInifile::r_u32, block_pnv_slot, test);
	result |= process_if_exists(section, "block_helmet_slot", &CInifile::r_u32, block_helmet_slot, test);

	result |= process_if_exists(section, "additional_jump_speed", &CInifile::r_float, m_additional_jump_speed, test);
	result |= process_if_exists(section, "additional_run_coef", &CInifile::r_float, m_additional_run_coef, test);
	result |= process_if_exists(section, "additional_sprint_koef", &CInifile::r_float, m_additional_sprint_koef, test);
	//result |= process_if_exists( section, "artefact_count", &CInifile::r_u32, m_artefact_count, test );
	//clamp( m_artefact_count, (u32)0, (u32)5 );

	return result;
}

void CCustomOutfit::AddBonesProtection(LPCSTR bones_section)
{
	CObject* parent = (IsGameTypeSingle()) ? smart_cast<CObject*>(Level().CurrentViewEntity()) : H_Parent();
	
	if (parent && parent->Visual() && m_BonesProtectionSect.size())
		m_boneProtection->add(bones_section, smart_cast<IKinematics*>(parent->Visual()));
}

void CCustomOutfit::ReloadBonesProtection()
{
	CObject* parent = (IsGameTypeSingle()) ? smart_cast<CObject*>(Level().CurrentViewEntity()) : H_Parent();

	if (parent && parent->Visual() && m_BonesProtectionSect.size())
		m_boneProtection->reload(m_BonesProtectionSect, smart_cast<IKinematics*>(parent->Visual()));
}