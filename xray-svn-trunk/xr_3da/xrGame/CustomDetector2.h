#pragma once

#include "hud_item_object.h"
#include "ai_sounds.h"
//#include "script_export_space.h"

class CUIArtefactDetectorBase;

class CCustomDetectorR : public CHudItemObject
{
	typedef	CHudItemObject	inherited;
protected:
	CUIArtefactDetectorBase*			m_ui;
	bool			m_bFastAnimMode;
	bool			m_bNeedActivation;
public:
	CCustomDetectorR();
	virtual			~CCustomDetectorR();

	virtual BOOL 	net_Spawn(CSE_Abstract* DC);
	virtual void 	Load(LPCSTR section);

	virtual void 	OnH_A_Chield();
	virtual void 	OnH_B_Independent(bool just_before_destroy);

	virtual void 	shedule_Update(u32 dt);
	virtual void 	UpdateCL();


	bool 	IsWorking();

	virtual void 	OnMoveToSlot();
	virtual void 	OnMoveToRuck();

	virtual void	OnActiveItem();
	virtual void	OnHiddenItem();
	virtual void	OnStateSwitch(u32 S);
	virtual void	OnAnimationEnd(u32 state);
	virtual	void	UpdateXForm();

	void			ToggleDetector(bool bFastMode);
	void			HideDetector(bool bFastMode);
	void			ShowDetector(bool bFastMode);
	virtual bool	CheckCompatibility(CHudItem* itm);

	float							fdetect_radius;
	float							foverallrangetocheck;
	float							fortestrangetocheck;
	u32								for_test;
	u8								reaction_sound_off;
	xr_vector<shared_str>			af_types;
	LPCSTR	 						af_sounds_section;
	LPCSTR							closestart;
	LPCSTR							detector_section;
protected:
	bool	CheckCompatibilityInt(CHudItem* itm, u16* slot_to_activate);
	void 	TurnDetectorInternal(bool b);

	void			UpdateVisibility();
	virtual void	UpfateWork();
	virtual void 	UpdateAf()				{};
	virtual void 	CreateUI()				{};

	bool			m_bWorking;
	float							snd_timetime;
	float							cur_periodperiod;
	u8								feel_touch_update_delay;
	HUD_SOUND_ITEM		detect_sndsnd;
	HUD_SOUND_ITEM		sound;

	virtual bool			install_upgrade_impl(LPCSTR section, bool test);

	//DECLARE_SCRIPT_REGISTER_FUNCTION
};
//add_to_type_list(CCustomDetectorR)
//#undef script_type_list
//#define script_type_list save_type_list(CCustomDetectorR)