#pragma once

#include "inventory_item_object.h"
#include "hudsound.h"

struct SBoneProtections;

class CCustomOutfit: public CInventoryItemObject {
private:
    typedef	CInventoryItemObject inherited;
public:
									CCustomOutfit		(void);
	virtual							~CCustomOutfit		(void);

	virtual void					Load				(LPCSTR section);
	
	//уменьшенная версия хита, для вызова, когда костюм надет на персонажа
	virtual void					Hit					(float P, ALife::EHitType hit_type);

	//коэффициенты на которые домножается хит
	//при соответствующем типе воздействия
	//если на персонаже надет костюм
	float							GetHitTypeProtection(ALife::EHitType hit_type, s16 element);
	float							GetDefHitTypeProtection(ALife::EHitType hit_type);

	////tatarinrafa: Замена на ЗП систему хита
	float							GetBoneArmor(s16 element);
	float							HitThroughArmor(float hit_power, s16 element, float ap, bool& add_wound, ALife::EHitType hit_type);

	//коэффициент на который домножается потеря силы
	//если на персонаже надет костюм
	float							GetPowerLoss		();


	virtual void					OnMoveToSlot		();
	virtual void					OnMoveToRuck		();

	/*	
		void					SwitchNightVision			();
		void					SwitchNightVision			(bool light_on);
		IC bool					GetNightVisionStatus		() const { return m_bNightVisionOn; }
	*/
protected:
	HitImmunity::HitTypeSVec		m_HitTypeProtection;
	float							m_fPowerLoss;

	shared_str						m_ActorVisual;
	shared_str						m_ActorVisual_legs;
	shared_str						m_full_icon_name;
	SBoneProtections*				m_boneProtection;	
protected:
	u32								m_ef_equipment_type;

	bool								m_bNightVisionEnabled;
	bool								m_bNightVisionOn;
	/*
	HUD_SOUND_ITEM							m_NightVisionOnSnd;
	HUD_SOUND_ITEM							m_NightVisionOffSnd;
	HUD_SOUND_ITEM							m_NightVisionIdleSnd;
	HUD_SOUND_ITEM							m_NightVisionBrokenSnd;
	*/
public:
	//tatarinrafa: added additional jump speed sprint speed walk speed
	float							m_additional_jump_speed;
	float							m_additional_run_coef;
	float							m_additional_sprint_koef;

	//--
	u32								block_pnv_slot;
	u32								block_helmet_slot;
	float							m_additional_weight;
	float							m_additional_weight2;
	shared_str						m_BonesProtectionSect;
	shared_str						m_NightVisionSect;
	virtual u32						ef_equipment_type		() const;
	virtual	BOOL					BonePassBullet			(int boneID);
	const shared_str&				GetFullIconName			() const	{return m_full_icon_name;};

	virtual void			net_Export			(NET_Packet& P);
	virtual void			net_Import			(NET_Packet& P);
	
			void			ReloadBonesProtection	();
			void			AddBonesProtection		(LPCSTR bones_section);
	virtual void			net_Destroy			();
	virtual BOOL			net_Spawn			(CSE_Abstract* DC);

	virtual void			OnH_A_Chield		();
	virtual void			OnH_B_Independent	(bool just_before_destroy);
protected:
	virtual bool			install_upgrade_impl( LPCSTR section, bool test );
};
