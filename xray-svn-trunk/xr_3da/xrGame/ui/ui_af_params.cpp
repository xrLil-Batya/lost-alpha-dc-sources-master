#include "stdafx.h"
#include "ui_af_params.h"
#include "UIStatic.h"
#include "../object_broker.h"
#include "UIXmlInit.h"

CUIArtefactParams::CUIArtefactParams()
{
	Memory.mem_fill			(m_info_items, 0, sizeof(m_info_items));
}

CUIArtefactParams::~CUIArtefactParams()
{
	for(u32 i=_item_start; i<_max_item_index; ++i)
	{
		CUIStatic* _s			= m_info_items[i];
		xr_delete				(_s);
	}
}

LPCSTR af_item_sect_names[] = {
	"health_restore_speed",
	"radiation_restore_speed",
	"satiety_restore_speed",
	"power_restore_speed",
	"bleeding_restore_speed",
	"additional_inventory_weight2",
	"sprint_allowed",
	"psy_health_restore_speed",
	
	"burn_immunity",
	"strike_immunity",
	"shock_immunity",
	"wound_immunity",		
	"radiation_immunity",
	"telepatic_immunity",
	"chemical_burn_immunity",
	"explosion_immunity",
	"fire_wound_immunity",
};

LPCSTR af_item_param_names[] = {
	"ui_inv_health",
	"ui_inv_radiation",
	"ui_inv_satiety",
	"ui_inv_power",
	"ui_inv_bleeding",
	"ui_inv_outfit_add_weight",
	"ui_inv_outfit_sprint_allowed",
	"ui_inv_psy_health_restore_speed",

	"ui_inv_outfit_burn_protection",			// "(burn_imm)",
	"ui_inv_outfit_strike_protection",			// "(strike_imm)",
	"ui_inv_outfit_shock_protection",			// "(shock_imm)",
	"ui_inv_outfit_wound_protection",			// "(wound_imm)",
	"ui_inv_outfit_radiation_protection",		// "(radiation_imm)",
	"ui_inv_outfit_telepatic_protection",		// "(telepatic_imm)",
	"ui_inv_outfit_chemical_burn_protection",	// "(chemical_burn_imm)",
	"ui_inv_outfit_explosion_protection",		// "(explosion_imm)",
	"ui_inv_outfit_fire_wound_protection",		// "(fire_wound_imm)",
};

LPCSTR af_actor_param_names[]={
	"satiety_health_v",
	"radiation_v",
	"satiety_v",
	"satiety_power_v",
	"wound_incarnation_v",
};
void CUIArtefactParams::InitFromXml(CUIXml& xml_doc)
{
	LPCSTR _base				= "af_params";
	if (!xml_doc.NavigateToNode(_base, 0))	return;

	string256					_buff;
	CUIXmlInit::InitWindow		(xml_doc, _base, 0, this);

	for(u32 i=_item_start; i<_max_item_index; ++i)
	{
		m_info_items[i]			= new CUIStatic();
		CUIStatic* _s			= m_info_items[i];
		_s->SetAutoDelete		(false);
		strconcat				(sizeof(_buff),_buff, _base, ":static_", af_item_sect_names[i]);
		CUIXmlInit::InitStatic	(xml_doc, _buff,	0, _s);
	}
}

bool CUIArtefactParams::Check(const shared_str& af_section)
{
	//return true;
	return !!pSettings->line_exist(af_section, "af_actor_properties");
}
#include "../string_table.h"
void CUIArtefactParams::SetInfo(const shared_str& af_section)
{

	string128					_buff;
	float						_h = 0.0f;
	DetachAll					();
	for(u32 i=_item_start; i<_max_item_index; ++i)
	{
		CUIStatic* _s			= m_info_items[i];

		float					_val;

		if (i == _item_sprint_allowed || i == _item_additional_inventory_weight2){
			if (i == _item_sprint_allowed){
				bool _bool = READ_IF_EXISTS(pSettings, r_bool, af_section, af_item_sect_names[i], true);

				if (_bool == false)
				{
					LPCSTR _color = "%c[red]";

					_s->SetTextureColor(color_rgba(255, 125, 25, 255));


					xr_sprintf(_buff, "%s%s", _color, CStringTable().translate(af_item_param_names[i]).c_str());

					_s->TextItemControl()->SetText(_buff);
					_s->SetWndPos(_s->GetWndPos().x, _h);
					_h += _s->GetWndSize().y;
					AttachChild(_s);
				}
			}
			else if (i == _item_additional_inventory_weight2){
				_val = READ_IF_EXISTS(pSettings, r_float, af_section, af_item_sect_names[i], 0.0f);

				if (_val != 0.0)
				{
					LPCSTR _color = (_val>0) ? "%c[green]" : "%c[red]";
					if (_val > 0) _s->SetTextureColor(color_rgba(25, 255, 25, 255));
					else _s->SetTextureColor(color_rgba(255, 25, 25, 255));

					xr_sprintf(_buff, "%s %s %+.0f", CStringTable().translate(af_item_param_names[i]).c_str(), _color, _val);

					_s->TextItemControl()->SetText(_buff);
					_s->SetWndPos(_s->GetWndPos().x, _h);
					_h += _s->GetWndSize().y;
					AttachChild(_s);
				}
			}

		}

		else{
			if (i < _max_item_index1)
			{
				//float _actor_val	= pSettings->r_float	("actor_condition", af_actor_param_names[i]);
				_val = pSettings->r_float(af_section, af_item_sect_names[i]);

				if (fis_zero(_val))				continue;

				//_val				= (_val/_actor_val)*100.0f;
			}
			else
			{
				shared_str _sect = pSettings->r_string(af_section, "hit_absorbation_sect");
				_val = pSettings->r_float(_sect, af_item_sect_names[i]);
				if (fsimilar(_val, 1.0f))				continue;
				_val = (1.0f - _val);
				_val *= 100.0f;

			}
			//LPCSTR _sn = "%";
			LPCSTR _sn = "";
			if (i == _item_radiation_restore_speed || i == _item_power_restore_speed || i == _item_psy_health_restore_speed)

			{
				_val /= 0.0001f;
				//_sn					= "";
			}
			if (i == _item_health_restore_speed)
			{
				_val /= 0.00015f;

			}

			LPCSTR _color = (_val > 0) ? "%c[green]" : "%c[red]";


			if (_val > 0) _s->SetTextureColor(color_rgba(25, 255, 25, 255));
			else _s->SetTextureColor(color_rgba(255, 25, 25, 255));

			if (i == _item_bleeding_restore_speed){
				_val = (_val *(-1.0f) / 0.001);
			}

			if (i == _item_satiety_restore_speed){
				_val = _val / 0.01;
			}

			if (i == _item_bleeding_restore_speed || i == _item_radiation_restore_speed){

				_color = (_val > 0) ? "%c[red]" : "%c[green]";
				if (_val > 0) _s->SetTextureColor(color_rgba(255, 25, 25, 255));
				else _s->SetTextureColor(color_rgba(25, 255, 25, 255));

			}

			xr_sprintf(_buff, "%s %s %+.0f %s",
				CStringTable().translate(af_item_param_names[i]).c_str(),
				_color,
				_val,
				_sn);
			_s->TextItemControl()->SetText(_buff);
			_s->SetWndPos(_s->GetWndPos().x, _h);
			_h += _s->GetWndSize().y;
			AttachChild(_s);
		}
	}
	SetHeight					(_h);
}
