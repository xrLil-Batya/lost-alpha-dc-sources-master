#include "stdafx.h"
#include "specific_character.h"

#ifdef  XRGAME_EXPORTS
#include "PhraseDialog.h"
#include "string_table.h"

SSpecificCharacterData::SSpecificCharacterData()
{
	m_sGameName.clear		();
	m_sBioText				= NULL;
	m_sVisual.clear			();
	m_sSupplySpawn.clear	();
	m_sNpcConfigSect.clear	();


	m_StartDialog			= NULL;
	m_ActorDialogs.clear	(); 

	m_Rank					= NO_RANK;
	m_Reputation			= NO_REPUTATION;

	m_bNoRandom				= false;
	m_bDefaultForCommunity	= false;
	m_fPanic_threshold		= 0.0f;
	m_fHitProbabilityFactor	= 1.f;
	m_crouch_type			= 0;
	m_can_upgrade			= "0";
}

SSpecificCharacterData::~SSpecificCharacterData()
{
}

#endif

CSpecificCharacter::CSpecificCharacter()
{
	m_OwnId = NULL;
}


CSpecificCharacter::~CSpecificCharacter()
{
}


void CSpecificCharacter::InitXmlIdToIndex()
{
	if(!id_to_index::tag_name)
		id_to_index::tag_name = "specific_character";
	if(!id_to_index::file_str)
		id_to_index::file_str = pSettings->r_string("profiles", "specific_characters_files");
}


void CSpecificCharacter::Load(shared_str id)
{
	R_ASSERT(id.size());
	m_OwnId = id;
	inherited_shared::load_shared(m_OwnId, NULL);
}

static bool IsBool(LPCSTR B)
{
	return (xr_strcmp(B,"1")==0 || xr_strcmp(B,"true")==0 || xr_strcmp(B,"on")==0 || xr_strcmp(B,"yes")==0);
}

void CSpecificCharacter::load_shared	(LPCSTR)
{
#if 0
	CTimer			timer;
	timer.Start		();
#endif
	const ITEM_DATA& item_data = *id_to_index::GetById(m_OwnId);

	CUIXml*		pXML = item_data._xml;

	pXML->SetLocalRoot		(pXML->GetRoot());


	XML_NODE* item_node = pXML->NavigateToNode(id_to_index::tag_name, item_data.pos_in_file);
	R_ASSERT3(item_node, "specific_character id=", *item_data.id);

	pXML->SetLocalRoot(item_node);



	int norandom = pXML->ReadAttribInt(item_node, "no_random", 0);
	if (1 == norandom) 
		data()->m_bNoRandom = true;
	else
		data()->m_bNoRandom = false;

	int team_default = pXML->ReadAttribInt(item_node, "team_default", 0);
	if (1 == team_default) 
		data()->m_bDefaultForCommunity = true;
	else
		data()->m_bDefaultForCommunity = false;

	R_ASSERT3(!(data()->m_bNoRandom && data()->m_bDefaultForCommunity), 
		"cannot set 'no_random' and 'team_default' flags simultaneously, profile id", *shared_str(item_data.id));
	
#ifdef  XRGAME_EXPORTS

	LPCSTR start_dialog = pXML->Read("start_dialog", 0, NULL);
	if(start_dialog)
	{
		data()->m_StartDialog	= start_dialog;
	}
	else
		data()->m_StartDialog	= NULL;

	int dialogs_num = pXML->GetNodesNum(pXML->GetLocalRoot(), "actor_dialog");
	data()->m_ActorDialogs.clear();
	for(int i=0; i<dialogs_num; ++i)
	{
		shared_str dialog_name = pXML->Read(pXML->GetLocalRoot(), "actor_dialog", i, "");
		data()->m_ActorDialogs.push_back(dialog_name);
	}

	data()->m_icon_name		= pXML->Read("icon", 0, "ui_npc_u_barman");
		
	shared_str can_upgrade	= pXML->Read("can_upgrade", 0, "false");
	data()->m_can_upgrade = can_upgrade == "true" ? "1" : "0";
	
	//������� ��� ���������
	data()->m_sGameName		= pXML->Read("name", 0, "");
	data()->m_sBioText		= CStringTable().translate(pXML->Read("bio", 0, ""));


	data()->m_fPanic_threshold		= pXML->ReadFlt("panic_threshold",0,0.f);
	data()->m_fHitProbabilityFactor	= pXML->ReadFlt("hit_probability_factor",0,1.f);
	data()->m_crouch_type			= pXML->ReadInt("crouch_type",0,0);

	data()->m_critical_wound_weights= pXML->Read("critical_wound_weights", 0, "1");

#endif

	data()->m_sVisual		= pXML->Read("visual", 0, "");
	

#ifdef  XRGAME_EXPORTS
	data()->m_sSupplySpawn	= pXML->Read("supplies", 0, "");
	
	if(!data()->m_sSupplySpawn.empty())
	{
		xr_string &str = data()->m_sSupplySpawn;
		xr_string::size_type pos = str.find("\\n");

		while (xr_string::npos != pos)
		{
			str.replace(pos, 2, "\n");
			pos = str.find("\\n", pos + 1);
		}
	}
	// phobos2077: new style of supplies
	XML_NODE* supplies = pXML->NavigateToNode("supplies");
	if (supplies)
	{
		data()->m_supplies.LoadSupplies(pXML, supplies);
	}

	data()->m_sNpcConfigSect		= pXML->Read("npc_config", 0, "");
	data()->m_sound_voice_prefix	= pXML->Read("snd_config", 0, "");

	data()->m_terrain_sect			= pXML->Read("terrain_sect", 0, "");

#endif

	data()->m_Classes.clear			();
	int classes_num					= pXML->GetNodesNum (pXML->GetLocalRoot(), "class");
	for(int i=0; i<classes_num; i++)
	{
		LPCSTR char_class			= pXML->Read	("class", 0, "");
		if(char_class)
		{
			char* buf_str			= xr_strdup(char_class);
			xr_strlwr				(buf_str);
			data()->m_Classes.push_back(buf_str);
			xr_free					(buf_str);
		}
	}


#ifdef  XRGAME_EXPORTS

	LPCSTR team = pXML->Read("community", 0, NULL);
	R_ASSERT3(team != NULL, "'community' field not fulfiled for specific character", *m_OwnId);
	
	char* buf_str = xr_strdup(team);
	xr_strlwr(buf_str);
	data()->m_Community.set(buf_str);
	xr_free(buf_str);
	
	if(data()->m_Community.index() == NO_COMMUNITY_INDEX)
		Debug.fatal(DEBUG_INFO,"wrong 'community' '%s' in specific character %s ", team, *m_OwnId);

	data()->m_Rank			= pXML->ReadInt("rank", 0, NO_RANK);
	R_ASSERT3(data()->m_Rank != NO_RANK, "'rank' field not fulfiled for specific character", *m_OwnId);
	data()->m_Reputation	= pXML->ReadInt("reputation", 0, NO_REPUTATION);
	R_ASSERT3(data()->m_Reputation != NO_REPUTATION, "'reputation' field not fulfiled for specific character", *m_OwnId);

	if(pXML->NavigateToNode(pXML->GetLocalRoot(), "money", 0))
	{
		MoneyDef().min_money	= pXML->ReadAttribInt("money", 0, "min");
		MoneyDef().max_money	= pXML->ReadAttribInt("money", 0, "max");
		MoneyDef().inf_money	= !!pXML->ReadAttribInt("money", 0, "infinitive");
		MoneyDef().max_money	= _max(MoneyDef().max_money, MoneyDef().min_money); // :)

	}else{
		MoneyDef().min_money	= 0;
		MoneyDef().max_money	= 0;
		MoneyDef().inf_money	= false;
	}

#endif

#if 0
	Msg			("CSpecificCharacter::load_shared() takes %f milliseconds",timer.GetElapsed_sec()*1000.f);
#endif
}

void SSpecificCharacterSupplies::LoadSupplies(CUIXml* pXML, XML_NODE* supplies)
{
	int itemsNum = pXML->GetNodesNum(supplies, "item");
	for (int i = 0; i < itemsNum; i++)
	{
		XML_NODE* itemNode = pXML->NavigateToNode(supplies, "item", i);
		Item item;
		item.section = pXML->ReadAttrib(itemNode, "s", "");
		if (item.section.size() == 0)
		{
			item.section = pXML->Read(itemNode, "");
		}
		if (item.section.size() > 0)
		{
			item.count = pXML->ReadAttribInt(itemNode, "qty", 1);
			item.probability = pXML->ReadAttribFlt(itemNode, "prob", 1.f);
			item.condition = pXML->ReadAttribFlt(itemNode, "cond", 1.f);
			item.hasScope = IsBool(pXML->ReadAttrib(itemNode, "scope", ""));
			item.hasLauncher = IsBool(pXML->ReadAttrib(itemNode, "launcher", ""));
			item.hasSilencer = IsBool(pXML->ReadAttrib(itemNode, "silencer", ""));
			item.ammoCount = pXML->ReadAttribInt(itemNode, "ammo", -1);
			item.ammoType = pXML->ReadAttrib(itemNode, "ammo_type", "");
			// int upgrNum = pXML->GetNodesNum(itemNode, "upgrade");
			LPCSTR upgrades = pXML->ReadAttrib(itemNode, "upgrades", "");
			int upgrNum = _GetItemCount(upgrades);
			for (int j = 0; j < upgrNum; j++)
			{
				// shared_str upgr = pXML->ReadAttrib(itemNode, "upgrade", j, "s", "");
				string256 upgr;
				_GetItem(upgrades, j, upgr);
				if (xr_strlen(upgr) > 0)
				{
					item.upgrades.push_back(upgr);
				}
			}
			m_items.push_back(item);
		}
	}

	itemsNum = pXML->GetNodesNum(supplies, "include");
	for (int i = 0; i < itemsNum; i++)
	{
		XML_NODE* itemNode = pXML->NavigateToNode(supplies, "include", i);
		Template tmplRef;
		tmplRef.name = pXML->ReadAttrib(itemNode, "id", "");
		if (tmplRef.name.size() == 0)
		{
			tmplRef.name = pXML->Read(itemNode, "");
		}
		if (tmplRef.name.size() > 0)
		{
			m_templates.push_back(tmplRef);
		}
	}
}


#ifdef  XRGAME_EXPORTS

LPCSTR CSpecificCharacter::Name() const 
{
	return	data()->m_sGameName.c_str();
}

shared_str CSpecificCharacter::Bio() const 
{
	return	data()->m_sBioText;
}

const CHARACTER_COMMUNITY& CSpecificCharacter::Community() const 
{
	return	data()->m_Community;
}

LPCSTR CSpecificCharacter::SupplySpawn	() const 
{
	return data()->m_sSupplySpawn.c_str();
}

const SSpecificCharacterSupplies& CSpecificCharacter::Supplies() const
{
	return data()->m_supplies;
}

LPCSTR CSpecificCharacter::NpcConfigSect () const 
{
	return data()->m_sNpcConfigSect.c_str();
}

LPCSTR CSpecificCharacter::sound_voice_prefix () const 
{
	return data()->m_sound_voice_prefix.c_str();
}

float CSpecificCharacter::panic_threshold () const 
{
	return data()->m_fPanic_threshold;
}

float CSpecificCharacter::hit_probability_factor () const 
{
	return data()->m_fHitProbabilityFactor;
}

int CSpecificCharacter::crouch_type	() const 
{
	return data()->m_crouch_type;
}

LPCSTR CSpecificCharacter::critical_wound_weights () const 
{
	return data()->m_critical_wound_weights.c_str();
}

#endif

shared_str CSpecificCharacter::terrain_sect		() const
{
	return	data()->m_terrain_sect;
}

CHARACTER_RANK_VALUE CSpecificCharacter::Rank() const 
{
	return	data()->m_Rank;
}

CHARACTER_REPUTATION_VALUE CSpecificCharacter::Reputation	() const 
{
	return data()->m_Reputation;
}

LPCSTR CSpecificCharacter::Visual		() const 
{
	return data()->m_sVisual.c_str();
}

const shared_str& CSpecificCharacter::CanUpgrade() const
{
	return data()->m_can_upgrade;
}

void CSpecificCharacterSupplyTemplate::InitXmlIdToIndex()
{
	if (!id_to_index::tag_name)
		id_to_index::tag_name = "template";
	if (!id_to_index::file_str)
		id_to_index::file_str = pSettings->r_string("profiles", "supplies_template_files");
}

void CSpecificCharacterSupplyTemplate::Load(shared_str id)
{
	R_ASSERT(id.size());
	m_OwnId = id;
	inherited_shared::load_shared(m_OwnId, NULL);
}

void CSpecificCharacterSupplyTemplate::load_shared(LPCSTR)
{
	const ITEM_DATA& item_data = *id_to_index::GetById(m_OwnId);
	CUIXml*		pXML = item_data._xml;
	pXML->SetLocalRoot(pXML->GetRoot());
	XML_NODE* item_node = pXML->NavigateToNode(id_to_index::tag_name, item_data.pos_in_file);
	get_sd()->m_supplies.LoadSupplies(pXML, item_node);
}

const SSpecificCharacterSupplies& CSpecificCharacterSupplyTemplate::Supplies() const
{
	return get_sd()->m_supplies;
}