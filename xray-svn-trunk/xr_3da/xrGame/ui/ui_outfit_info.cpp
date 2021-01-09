#include "stdafx.h"
#include "ui_outfit_info.h"
#include "UIStatic.h"

#include "UIXmlInit.h"

CUIOutfitParams::CUIOutfitParams()
{
	Memory.mem_fill(m_info_items, 0, sizeof(m_info_items));
}

CUIOutfitParams::~CUIOutfitParams()
{
	for (u32 i = _item_start; i<_max_item_index; ++i)
	{
		CUIStatic* _s = m_info_items[i];
		xr_delete(_s);
	}
}

LPCSTR icons_from_artifact_xml[] = {

	"burn_immunity",
	"strike_immunity",
	"shock_immunity",
	"wound_immunity",
	"radiation_immunity",
	"telepatic_immunity",
	"chemical_burn_immunity",
	"explosion_immunity",
	"fire_wound_immunity",
	"additional_inventory_weight2",
	"sprint_allowed",

	//"af_params:static_burn_immunity",
	//"af_params:static_strike_immunity",
	//"af_params:static_shock_immunity",
	//"af_params:static_wound_immunity",
	//"af_params:static_radiation_immunity",
	//"af_params:static_telepatic_immunity",
	//"af_params:static_chemical_burn_immunity",
	//"af_params:static_explosion_immunity",
	//"af_params:static_fire_wound_immunity",
};

LPCSTR outfit_property_names[] = {

	"burn_protection",
	"strike_protection",
	"shock_protection",
	"wound_protection",
	"radiation_protection",
	"telepatic_protection",
	"chemical_burn_protection",
	"explosion_protection",
	"fire_wound_protection",
	"additional_inventory_weight2",
	"sprint_allowed",
};

LPCSTR outfit_params_names[] = {
	"ui_inv_outfit_burn_protection",			
	"ui_inv_outfit_strike_protection",			
	"ui_inv_outfit_shock_protection",			
	"ui_inv_outfit_wound_protection",			
	"ui_inv_outfit_radiation_protection",		
	"ui_inv_outfit_telepatic_protection",		
	"ui_inv_outfit_chemical_burn_protection",	
	"ui_inv_outfit_explosion_protection",		
	"ui_inv_outfit_fire_wound_protection",		
	"ui_inv_outfit_add_weight",
	"ui_inv_outfit_sprint_allowed",
};

void CUIOutfitParams::InitFromXml(CUIXml& xml_doc)
{
	LPCSTR _base = "af_params";
	if (!xml_doc.NavigateToNode(_base, 0))	return;

	string256					_buff;
	CUIXmlInit::InitWindow(xml_doc, _base, 0, this);

	for (u32 i = _item_start; i<_max_item_index; ++i)
	{
		m_info_items[i] = new CUIStatic();
		CUIStatic* _s = m_info_items[i];
		_s->SetAutoDelete(false);
		//strcpy(_buff, icons_from_artifact_xml[i]);
		strconcat(sizeof(_buff), _buff, _base, ":static_", icons_from_artifact_xml[i]);
		CUIXmlInit::InitStatic(xml_doc, _buff, 0, _s);
	}
}

bool CUIOutfitParams::Check(const shared_str& af_section)
{
	//return true;
	return !!pSettings->line_exist(af_section, "burn_protection");
}

#include "../string_table.h"

void CUIOutfitParams::SetInfo(const shared_str& outfit_section)
{
	string128					_buff;
	float						_h = 0.0f;
	DetachAll();
	for (u32 i = _item_start; i<_max_item_index; ++i)
	{
		CUIStatic* _s = m_info_items[i];

		float					_val;
		bool					_bool;

		if (i == _item_sprint_allowed)
		{
			_bool = READ_IF_EXISTS(pSettings, r_bool, outfit_section, outfit_property_names[i], true);

			if (_bool == false)
			{
				LPCSTR _color = "%c[red]";

				_s->SetTextureColor(color_rgba(255, 125, 25, 255));


				xr_sprintf(_buff, "%s%s", _color, CStringTable().translate(outfit_params_names[i]).c_str());

				_s->TextItemControl()->SetText(_buff);
				_s->SetWndPos(_s->GetWndPos().x, _h);
				_h += _s->GetWndSize().y;
				AttachChild(_s);
			}
		}
		else{

			_val = READ_IF_EXISTS(pSettings, r_float, outfit_section, outfit_property_names[i], 0.0f);

			if (_val != 0.0)
			{
				LPCSTR _color = (_val>0) ? "%c[green]" : "%c[red]";
				if (_val > 0) _s->SetTextureColor(color_rgba(25, 255, 25, 255));
				else _s->SetTextureColor(color_rgba(255, 25, 25, 255));

				_val *= 100.0;

				if (i == _item_additional_inventory_weight2)
				{
					_val /= 100;
				}

				xr_sprintf(_buff, "%s %s %+.0f",
					CStringTable().translate(outfit_params_names[i]).c_str(),
					_color,
					_val);

				_s->TextItemControl()->SetText(_buff);
				_s->SetWndPos(_s->GetWndPos().x, _h);
				_h += _s->GetWndSize().y;
				AttachChild(_s);
			}
		}

	}
	SetHeight(_h);
}
