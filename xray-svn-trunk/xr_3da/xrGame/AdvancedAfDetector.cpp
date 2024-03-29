#include "stdafx.h"
#include "advancedafdetector.h"
#include "ui/ArtefactDetectorUI.h"
//#include "../Include/xrRender/Kinematics.h"
//#include "../LightAnimLibrary.h"
#include "player_hud.h"
//#include "clsid_game.h"
#include "artifact.h"
//#include "ai\monsters\ai_monster_utils.h"


CAdvancedDetector::CAdvancedDetector()
{
}

CAdvancedDetector::~CAdvancedDetector()
{}

void CAdvancedDetector::CreateUI()
{
	R_ASSERT(NULL == m_ui);
	m_ui = new CUIArtefactDetectorAdv();
	ui().construct(this);

	if (for_test == 1){
		Fvector						P;
		P.set(this->Position());
		feel_touch_update(P, fortestrangetocheck);


		for (xr_vector<CObject*>::iterator it = feel_touch.begin(); it != feel_touch.end(); it++)
		{
			CObject* item = *it;
			if (item){
				Fvector dir, to;

				CInventoryItem* invitem = smart_cast<CInventoryItem*>(item);
				item->Center(to);
				float range = dir.sub(to, this->Position()).magnitude();
				Msg("Artefact %s, distance is %f", invitem->object().cNameSect().c_str(), range);
			}
		}
	}
}

//------------------------------------Feel Touch---------------------------------------
void CAdvancedDetector::feel_touch_new(CObject* O)
{
}

void CAdvancedDetector::feel_touch_delete(CObject* O)
{

}

BOOL CAdvancedDetector::feel_touch_contact(CObject *O)
{

	bool is_visibleforDetector = false;
	CArtefact* artefact = smart_cast<CArtefact*>(O);

	if (artefact){
		for (size_t id = 0; id < af_types.size(); ++id) {

			if ((xr_strcmp(O->cNameSect(), af_types[id]) == 0) || (xr_strcmp(af_types[id], "all") == 0)) {
				is_visibleforDetector = true;
			}
		}
		if (is_visibleforDetector == true) {
			return true;
		}
		else{ return false; }
	}
	else{ return false; }


}

//----------------------------------------------------------------------------------------

void CAdvancedDetector::UpdateAf()
{
	ui().SetValue(0.0f, Fvector().set(0, 0, 0));

	//------------������� ��������� ��� �� ������ feeltouch
	CArtefact* pCurrentAf;
	LPCSTR closest_art = "null";
	feel_touch_update_delay = feel_touch_update_delay + 1;
	if (feel_touch_update_delay >= 5)//��� �� ������� �������� �� ���� �������
	{
		feel_touch_update_delay = 0;
		Fvector						P;
		P.set(this->Position());
		feel_touch_update(P, foverallrangetocheck);
	}

	float disttoclosestart = 0.0;

	for (xr_vector<CObject*>::iterator it = feel_touch.begin(); it != feel_touch.end(); it++)
	{
		float disttoart = DetectorFeel(*it);
		if (disttoart != -10.0)
		{
			//���� ���������� ����� ��� �� �������(������ ������ �����), �� ���� �� ��������
			if (disttoclosestart <= 0.0)
			{
				disttoclosestart = disttoart;
				closest_art = closestart;
				pCurrentAf = smart_cast<CArtefact*>(*it);
			}
			//����� ����� ������� ���...
			if (disttoclosestart > disttoart)
			{
				disttoclosestart = disttoart;
				closest_art = closestart;
				pCurrentAf = smart_cast<CArtefact*>(*it);
			}
		}
	}

	//���������� ������� ������� ������������ �������
	if (disttoclosestart == 0.0)
	{
		return;
	}
	else{
		cur_periodperiod = disttoclosestart / fdetect_radius * pCurrentAf->detect_radius_koef;
	}

	//����� �� ����������� ����. ������
	if (cur_periodperiod < 0.11)
	{
		cur_periodperiod = 0.11;
	}

	if (reaction_sound_off == 0){
		//������� ����������� ������ ������ ����� ��� ��������� �����
		if (xr_strcmp(af_sounds_section, "null") != 0){
			HUD_SOUND_ITEM::LoadSound(closest_art, af_sounds_section, sound, SOUND_TYPE_ITEM);
			LPCSTR check = READ_IF_EXISTS(pSettings, r_string, closest_art, af_sounds_section, "null");
			if (xr_strcmp(check, "null") != 0){
				detect_sndsnd = sound;
			}

		}

		if (snd_timetime > cur_periodperiod)
		{
			snd_timetime = 0;
			HUD_SOUND_ITEM::PlaySound(detect_sndsnd, Fvector().set(0, 0, 0), this, true, false);
			if (detect_sndsnd.m_activeSnd){

				float procent_cur_periodperiod = (1.0 - 0.11) / 100;
				float procent_freq = (1.8 - 0.8) / 100;
				float freqq = 1.8 - (procent_freq * ((cur_periodperiod - 0.11) / procent_cur_periodperiod));

				if (freqq < 0.8)
					freqq = 0.8;

				detect_sndsnd.m_activeSnd->snd.set_frequency(freqq);
			}
		}
		else
			snd_timetime += Device.fTimeDelta;
	}

	if (!pCurrentAf) return; //�� ������ ������

	//������� ����������� � �������� ��� � �� ������
	Fvector					dir_to_artefact;
	dir_to_artefact.sub(pCurrentAf->Position(), Device.vCameraPosition);
	dir_to_artefact.normalize();
	float _ang_af = dir_to_artefact.getH();
	float _ang_cam = Device.vCameraDirection.getH();

	float _diff = angle_difference_signed(_ang_af, _ang_cam);

	ui().SetValue(_diff, dir_to_artefact);
}

//������ ��� �������� ����� � ��������� �������, � �� � ��� ������ ���
float CAdvancedDetector::DetectorFeel(CObject* item)
{
	Fvector dir, to;

	item->Center(to);
	float range = dir.sub(to, Position()).magnitude();
	CInventoryItem* invitem = smart_cast<CInventoryItem*>(item);
	CArtefact* artefact = smart_cast<CArtefact*>(item);

	float gogogo = fdetect_radius *  artefact->detect_radius_koef;
	if (range<gogogo)
	{
		//Msg("feeldetector  %s  %s", invitem->object().cNameSect_str(), invitem->object().cNameSect().c_str());
		closestart = invitem->object().cNameSect_str();
		return range;
	}
	return -10.0;
}

//---------------UI(�������� �����������)------------------
CUIArtefactDetectorAdv&  CAdvancedDetector::ui()
{
	return *((CUIArtefactDetectorAdv*)m_ui);
}

void CUIArtefactDetectorAdv::construct(CAdvancedDetector* p)
{
	m_parent = p;
	m_target_dir.set(0, 0, 0);
	m_curr_ang_speed = 0.0f;
	m_cur_y_rot = 0.0f;
	m_bid = u16(-1);
}

CUIArtefactDetectorAdv::~CUIArtefactDetectorAdv()
{
}

void CUIArtefactDetectorAdv::SetValue(const float val1, const Fvector& val2)
{
	m_target_dir = val2;
}

void CUIArtefactDetectorAdv::update()
{
	if (NULL == m_parent->HudItemData() || m_bid == u16(-1))	return;
	inherited::update();
	attachable_hud_item* itm = m_parent->HudItemData();
	R_ASSERT(itm);

	BOOL b_visible = !fis_zero(m_target_dir.magnitude());
	if (b_visible != itm->m_model->LL_GetBoneVisible(m_bid))
		itm->m_model->LL_SetBoneVisible(m_bid, b_visible, TRUE);

	if (!b_visible)
		return;


	Fvector							dest;
	Fmatrix							Mi;
	Mi.invert(itm->m_item_transform);
	Mi.transform_dir(dest, m_target_dir);

	float dest_y_rot = -dest.getH();


	/*
	m_cur_y_rot						= angle_normalize_signed(m_cur_y_rot);
	float diff						= angle_difference_signed(m_cur_y_rot, dest_y_rot);
	float a							= (diff>0.0f)?-1.0f:1.0f;

	a								*= 2.0f;

	m_curr_ang_speed				= m_curr_ang_speed + a*Device.fTimeDelta;
	clamp							(m_curr_ang_speed,-2.0f,2.0f);
	float _add						= m_curr_ang_speed*Device.fTimeDelta;

	m_cur_y_rot						+= _add;
	*/
	m_cur_y_rot = angle_inertion_var(m_cur_y_rot,
		dest_y_rot,
		PI_DIV_4,
		PI_MUL_4,
		PI_MUL_2,
		Device.fTimeDelta);

}

float CUIArtefactDetectorAdv::CurrentYRotation() const
{
	float one = PI_MUL_2 / 24.0f;
	float ret = fmod(m_cur_y_rot, one);
	return			(m_cur_y_rot - ret);
}

void CAdvancedDetector::on_a_hud_attach()
{
	inherited::on_a_hud_attach();
	ui().SetBoneCallbacks();
}

/*
void CAdvancedDetector::on_b_hud_detach()
{
	Msg("On a hud attach");
	inherited::on_b_hud_detach();
	//ui().ResetBoneCallbacks();
}
*/

void CUIArtefactDetectorAdv::BoneCallback(CBoneInstance *B)
{
	CUIArtefactDetectorAdv *P = static_cast<CUIArtefactDetectorAdv*>(B->callback_param());
	Fmatrix							rY;
	rY.rotateY(P->CurrentYRotation());
	B->mTransform.mulB_43(rY);
}

void CUIArtefactDetectorAdv::SetBoneCallbacks()
{
	attachable_hud_item* itm = m_parent->HudItemData();
	R_ASSERT(itm);
	m_bid = itm->m_model->LL_BoneID("screen_bone");

	CBoneInstance& bi = itm->m_model->LL_GetBoneInstance(m_bid);
	bi.set_callback(bctCustom, BoneCallback, this);

	float p, b;
	bi.mTransform.getHPB(m_cur_y_rot, p, b);
}

/*
void CUIArtefactDetectorAdv::ResetBoneCallbacks()
{
	Msg("On a hud attach1");
	attachable_hud_item* itm = m_parent->HudItemData();
	R_ASSERT(itm);
	Msg("On a hud attach2");
	u16 bid = itm->m_model->LL_BoneID("screen_bone");

	Msg("On a hud attach3");
	CBoneInstance& bi = itm->m_model->LL_GetBoneInstance(bid);
	Msg("On a hud attach4");
	bi.reset_callback();
}
*/
