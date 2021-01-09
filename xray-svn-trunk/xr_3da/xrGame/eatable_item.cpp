////////////////////////////////////////////////////////////////////////////
//	Module 		: eatable_item.cpp
//	Created 	: 24.03.2003
//  Modified 	: 29.01.2004
//	Author		: Yuri Dobronravin
//	Description : Eatable item
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "eatable_item.h"
#include "xrmessages.h"
#include "../../xrNetServer/net_utils.h"
#include "physic_item.h"
#include "Level.h"
#include "entity_alive.h"
#include "EntityCondition.h"
#include "ActorCondition.h"
#include "InventoryOwner.h"
#include "actor.h"

CEatableItem::CEatableItem()
{
	m_fHealthInfluence = 0;
	m_fPowerInfluence = 0;
	m_fSatietyInfluence = 0;
	m_fThirstyInfluence = 0;
	m_fPsyHealthInfluence = 0;
	m_fRadiationInfluence = 0;

	m_iPortionsNum = 1;

	m_physic_item	= 0;

	bProlongedEffect = 0;
	//	iEffectorNumber = 0;
	iEffectorBlockingGroup = 0;
}

CEatableItem::~CEatableItem()
{
}

DLL_Pure *CEatableItem::_construct	()
{
	m_physic_item	= smart_cast<CPhysicItem*>(this);
	return			(inherited::_construct());
}

void CEatableItem::Load(LPCSTR section)
{
	inherited::Load(section);
	
	m_fHealthInfluence			= pSettings->r_float(section, "eat_health");
	m_fPowerInfluence			= pSettings->r_float(section, "eat_power");
	m_fSatietyInfluence			= pSettings->r_float(section, "eat_satiety");
	m_fThirstyInfluence			= pSettings->r_float(section, "eat_thirst");
	m_fPsyHealthInfluence		= pSettings->r_float(section, "eat_psy_health");
	m_fRadiationInfluence		= pSettings->r_float(section, "eat_radiation");
	m_fWoundsHealPerc			= pSettings->r_float(section, "wounds_heal_perc");
	clamp						(m_fWoundsHealPerc, 0.f, 1.f);
//	m_bIsBattery				 = READ_IF_EXISTS(pSettings, r_bool, section, "is_battery", false);
	m_iPortionsNum			= READ_IF_EXISTS	(pSettings,r_u32,section, "eat_portions_num", 1);
	m_fMaxPowerUpInfluence		= READ_IF_EXISTS	(pSettings,r_float,section,"eat_max_power",0.0f);
	VERIFY2						(m_iPortionsNum<10000 || m_iPortionsNum == -1, make_string("'eat_portions_num' should be < 10000. Wrong section [%s]",section));

	//tatarinrafa: Повременное Использование
	//iEffectsAffectedStat 1 = Здоровье
	//iEffectsAffectedStat 2 = Кровиеотечен
	//iEffectsAffectedStat 3 = Радиация
	//iEffectsAffectedStat 4 = Пси-здоровье
	//iEffectsAffectedStat 5 = Еда
	//iEffectsAffectedStat 6 = Вода
	//iEffectsAffectedStat 7 = Энергия
	//iEffectsAffectedStat 8 = Опьянение
	bProlongedEffect = !!READ_IF_EXISTS(pSettings, r_bool, section, "use_prolonged_effect", FALSE);

	if (bProlongedEffect){

		iEffectorBlockingGroup = READ_IF_EXISTS(pSettings, r_u8, section, "effector_blocking_group", 0);

		fItemUseTime = READ_IF_EXISTS(pSettings, r_float, section, "item_use_time", 0.0f);

		//#pragma	todo("тодо: iOverridePriority позволит использовать более сильный предмет, когда уже использован слабый. Например: Обычный бинт и Хороший бинт. Хороший бинт заменит эффект плохого. Но еще один Хороший бинт не поставишь.")

		//Msg("found iEffectorBlockingGroup %u, fItemUseTime %f", iEffectorBlockingGroup, fItemUseTime);

		LPCSTR templist = READ_IF_EXISTS(pSettings, r_string, section, "effects_list", "null");
		if (templist && templist[0])
		{
			string128		effect;
			int				count = _GetItemCount(templist);
			for (int it = 0; it < count; ++it)
			{
				_GetItem(templist, it, effect);
				//Msg("found effect %s", effect);
				sEffectList.push_back(effect);
			}
		}

		string128		temp_string128;
		float			tempfloat;
		u8				tempint;
		bool			tempbool = false;
		for (int i = 0; i < sEffectList.size(); ++i)
		{
		//	Msg("%s", sEffectList[i].c_str());
			xr_sprintf(temp_string128, "%s_rate", sEffectList[i].c_str());
		//	Msg("temp_string128 = %s", temp_string128);
			tempfloat = READ_IF_EXISTS(pSettings, r_float, section, temp_string128, 0.0f);
			fEffectsRate.push_back(tempfloat);

			xr_sprintf(temp_string128, "%s_dur", sEffectList[i].c_str());
		//	Msg("temp_string128 = %s", temp_string128);
			tempfloat = READ_IF_EXISTS(pSettings, r_float, section, temp_string128, 0.0f);
			fEffectsDur.push_back(tempfloat);

			xr_sprintf(temp_string128, "%s_affected_stat", sEffectList[i].c_str());
		//	Msg("temp_string128 = %s", temp_string128);
			tempint = READ_IF_EXISTS(pSettings, r_u8, section, temp_string128, 1);
			iEffectsAffectedStat.push_back(tempint);

			xr_sprintf(temp_string128, "%s_is_booster", sEffectList[i].c_str());
		//	Msg("temp_string128 = %s", temp_string128);
			tempbool = !!READ_IF_EXISTS(pSettings, r_bool, section, temp_string128, FALSE);
			sEffectIsBooster.push_back(tempbool);

			if (sEffectIsBooster[i]){
				//Msg("Loading booster immunities");
				xr_sprintf(temp_string128, "%s_hit_absorbation_sect", sEffectList[i].c_str());
				//Msg("temp_string128 = %s", temp_string128);
				CHitImmunity tempimun;
				tempimun.LoadImmunities(pSettings->r_string(section, temp_string128), pSettings);
				m_BoosterHitImmunities.push_back(tempimun);

				xr_sprintf(temp_string128, "%s_additional_weight", sEffectList[i].c_str());
				//Msg("temp_string128 = %s", temp_string128);
				tempfloat = READ_IF_EXISTS(pSettings, r_float, section, temp_string128, 0.0f);
				fAddWeight.push_back(tempfloat);
			}

#pragma	todo("тодо: bEffectsStackOverideNeither позволит стакать\заменять некоторые эффекты, но не стакать\заменять отдельные Например:Водка - опьянение стакается, но антирадиационный эффект нет")

		}
		
		//for (int i = 0; i < sEffectList.size(); ++i)
		//{
		//	Msg("fEffectsRate %i = %f", i, fEffectsRate[i]);
		//	Msg("fEffectsDur %i = %f", i, fEffectsDur[i]);
		//	Msg("fUseTime %i = %f", i, fItemUseTime);
		//	Msg("iEffectsAffectedStat %i = %i", i, iEffectsAffectedStat[i]);
		//}
		
	}
}

BOOL CEatableItem::net_Spawn				(CSE_Abstract* DC)
{
	if (!inherited::net_Spawn(DC)) return FALSE;

	return TRUE;
};

bool CEatableItem::Useful() const
{
	if(!inherited::Useful()) return false;

	//проверить не все ли еще съедено
	if(Empty()) return false;

	return true;
}

void CEatableItem::OnH_B_Independent(bool just_before_destroy)
{
	if(!Useful()) 
	{
		object().setVisible(FALSE);
		object().setEnabled(FALSE);
		if (m_physic_item)
			m_physic_item->m_ready_to_destroy	= true;
	}
	inherited::OnH_B_Independent(just_before_destroy);
}

void CEatableItem::save				(NET_Packet &packet)
{
	inherited::save				(packet);
	save_data				(m_iPortionsNum, packet);
}

void CEatableItem::load				(IReader &packet)
{
	inherited::load				(packet);
	load_data				(m_iPortionsNum, packet);
}

void CEatableItem::UseBy (CEntityAlive* entity_alive)
{
	CInventoryOwner* IO = smart_cast<CInventoryOwner*>(entity_alive);
	CActor *actor = 0;
	R_ASSERT(IO);
	R_ASSERT(m_pCurrentInventory == IO->m_inventory);
	R_ASSERT(object().H_Parent()->ID() == entity_alive->ID());


	if (bProlongedEffect){ // Если параметр задан, то использовать ситстему эффектов. Если нет - Мгновенное примененеие

		CActor* if_Actor = smart_cast<CActor*>(entity_alive);
		if (if_Actor){
			//Msg("Prolonged Use");
			bool handsarenotinuse = true;
			bool no_sameblockinggroup = true;
			for (int i = 0; i < if_Actor->conditions().Effectors.size(); i++){
				Effector effector = if_Actor->conditions().Effectors[i];

				if (if_Actor->conditions().fHandsHideTime > Device.fTimeGlobal){	// Проверяем не заняты ли руки другим применяемым предметом
					handsarenotinuse = false;
					//Msg("Hands are buisy already");
				}
				if (iEffectorBlockingGroup != 0 && iEffectorBlockingGroup == effector.iBlockingGroup){	//0 - нету группы блока. Проверяем блокируется ли использование уже действующими эффектами
					no_sameblockinggroup = false;
					//Msg("Blocked by other effector");
				}
			}

			if (handsarenotinuse && no_sameblockinggroup){	// Если все норм, вешаем эффекты предмета

				//Msg("All ok");
				Actor()->SetWeaponHideState(INV_STATE_BLOCK_ALL, true);		//Прячем руки
				if_Actor->conditions().fHandsHideTime = fItemUseTime + Device.fTimeGlobal;

				for (int i = 0; i < sEffectList.size(); ++i)
				{
					//Msg("Setting up effect");
					Effector effector;
					effector.fEffectorDuration			= fEffectsDur[i] + Device.fTimeGlobal + fItemUseTime; // чтобы не усложнять код, просто добавим use_time к duration_time и пусть себе течет время
					effector.fEffectorRate				= fEffectsRate[i];
					effector.fEffectorUseTime			= if_Actor->conditions().fHandsHideTime; //Для того, чтобы спрятанные руки не блокировали уже применяемые эффекты. Не удалять!
					effector.iBlockingGroup				= iEffectorBlockingGroup;	//Так как у эффектов разное время действия, нужно давайть им всем этот параметр
					effector.iEffectorAffectedState		= iEffectsAffectedStat[i];
					effector.bIsBooster					= sEffectIsBooster[i];

					effector.fAddWeightBooster			= 0.f;
					if (sEffectIsBooster[i]){
						//Msg("setting up effect immunities");
						effector.m_EffectHitImmunities	= m_BoosterHitImmunities[i];
						effector.fAddWeightBooster		= fAddWeight[i];

						if_Actor->conditions().m_fBoostersAddWeight += fAddWeight[i];
					}

					if_Actor->conditions().Effectors.push_back(effector);
				}

				//(Удаление произходит в след. кадр)
				if (m_iPortionsNum != -1)
				{
					//уменьшить количество порций
					if (m_iPortionsNum > 0)
						--(m_iPortionsNum);
					else
						m_iPortionsNum = 0;
				}
			}

		}
		else{	// Для НПС
			entity_alive->conditions().ChangeHealth(m_fHealthInfluence);
			entity_alive->conditions().ChangePower(m_fPowerInfluence);
			entity_alive->conditions().ChangeSatiety(m_fSatietyInfluence);
			entity_alive->conditions().ChangeThirsty(m_fThirstyInfluence);
			entity_alive->conditions().ChangePsyHealth(m_fPsyHealthInfluence);
			entity_alive->conditions().ChangeRadiation(m_fRadiationInfluence);
			entity_alive->conditions().ChangeBleeding(m_fWoundsHealPerc);
			entity_alive->conditions().SetMaxPower(entity_alive->conditions().GetMaxPower() + m_fMaxPowerUpInfluence);
			if (m_iPortionsNum != -1)
			{
				//уменьшить количество порций
				if (m_iPortionsNum > 0)
					--(m_iPortionsNum);
				else
					m_iPortionsNum = 0;
			}
		}
	}
	else{
		entity_alive->conditions().ChangeHealth(m_fHealthInfluence);
		entity_alive->conditions().ChangePower(m_fPowerInfluence);
		entity_alive->conditions().ChangeSatiety(m_fSatietyInfluence);
		entity_alive->conditions().ChangeThirsty(m_fThirstyInfluence);
		entity_alive->conditions().ChangePsyHealth(m_fPsyHealthInfluence);
		entity_alive->conditions().ChangeRadiation(m_fRadiationInfluence);
		entity_alive->conditions().ChangeBleeding(m_fWoundsHealPerc);
		entity_alive->conditions().SetMaxPower(entity_alive->conditions().GetMaxPower() + m_fMaxPowerUpInfluence);
		if (m_iPortionsNum != -1)
		{
			//уменьшить количество порций
			if (m_iPortionsNum > 0)
				--(m_iPortionsNum);
			else
				m_iPortionsNum = 0;
		}
	}
}
