#include "stdafx.h"
#pragma hdrstop
#include "TextureDescrManager.h"
#include "ETextureParams.h"

// eye-params
float					r__dtex_range	= 50;
class cl_dt_scaler		: public R_constant_setup {
public:
	float				scale;

	cl_dt_scaler		(float s) : scale(s)	{};
	virtual void setup	(R_constant* C)
	{
		RCache.set_c	(C,scale,scale,scale,1/r__dtex_range);
	}
};

void fix_texture_thm_name(LPSTR fn)
{
	LPSTR _ext = strext(fn);
	if(  _ext					&&
	  (0==stricmp(_ext,".tga")	||
		0==stricmp(_ext,".thm")	||
		0==stricmp(_ext,".dds")	||
		0==stricmp(_ext,".bmp")	||
		0==stricmp(_ext,".ogm")	) )
		*_ext = 0;
}

void CTextureDescrMngr::LoadLTX()
{
	string_path				fname;		
	FS.update_path			(fname,"$game_textures$","textures_associations.ltx");

	if (FS.exist(fname))
	{
		CInifile			ini(fname);
		if (ini.section_exist("association"))
		{
			CInifile::Sect& data	= ini.r_section("association");
			CInifile::SectCIt I		= data.Data.begin();
			CInifile::SectCIt E		= data.Data.end();
			for ( ; I!=E; ++I)	
			{
				const CInifile::Item& item	= *I;

				texture_desc& desc		= m_texture_details[item.first];
				cl_dt_scaler*&	dts		= m_detail_scalers[item.first];

				desc.m_assoc			= new texture_assoc();

				string_path				T;
				float					s;

				int res = sscanf					(*item.second,"%[^,],%f",T,&s);
				R_ASSERT(res==2);
				desc.m_assoc->detail_name = T;

				if (dts)
					dts->scale = s;
				else
					dts	= new cl_dt_scaler(s);

				desc.m_assoc->usage		= 0;
				if(strstr(item.second.c_str(),"usage[diffuse_or_bump]"))
					desc.m_assoc->usage	= (1<<0)|(1<<1);
				else
				if(strstr(item.second.c_str(),"usage[bump]"))
					desc.m_assoc->usage	= (1<<1);
				else
				if(strstr(item.second.c_str(),"usage[diffuse]"))
					desc.m_assoc->usage	= (1<<0);

				desc.m_assoc->m_tesselation_method = 0;
				if (strstr(item.second.c_str(), "tessellation_method[1]"))
					desc.m_assoc->m_tesselation_method = 1;
				if (strstr(item.second.c_str(), "tessellation_method[2]"))
					desc.m_assoc->m_tesselation_method = 2;
				if (strstr(item.second.c_str(), "tessellation_method[3]"))
					desc.m_assoc->m_tesselation_method = 3;
				//Msg("Tesselation Method for %s = %u", item.first.c_str(), desc.m_assoc->m_tesselation_method);
			}
		}//"association"
	}//file-exist

	FS.update_path			(fname,"$game_textures$","textures_specifications.ltx");
	if (FS.exist(fname))
	{
		CInifile			ini(fname);
		if (ini.section_exist("specification"))
		{
			CInifile::Sect& 	sect = ini.r_section("specification");
			for (CInifile::SectCIt I2=sect.Data.begin(); I2!=sect.Data.end(); ++I2)	
			{
				const CInifile::Item& item	= *I2;

				float gcoef = 1.0;	// поумолчанию коэфициент, чтобы глосс не пропал вообще
				float goffset = 0.0;

				texture_desc& desc		= m_texture_details[item.first];
				desc.m_spec				= new texture_spec();

				cl_gloss_coef_and_offset*&	glossparams = desc.m_spec->textureglossparams;

				string_path				bmode;
				int res = sscanf(item.second.c_str(), "bump_mode[%[^]]], material[%f], gloss_coef[%f], gloss_offset[%f]", bmode, &desc.m_spec->m_material, &gcoef, &goffset);
				R_ASSERT(res>=2);
				if ((bmode[0] == 'u') && (bmode[1] == 's') && (bmode[2] == 'e') && (bmode[3] == ':'))
				{
					// bump-map specified
					desc.m_spec->m_bump_name = bmode + 4;
				}
				if ((bmode[0] == 'u') && (bmode[1] == 's') && (bmode[2] == 'e') && (bmode[3] == '_') && (bmode[4] == 'p') && (bmode[5] == ':'))
				{
					// bump-map specified with parallax
					desc.m_spec->m_bump_name = bmode + 6;
					desc.m_spec->m_use_steep_parallax = true;
				}

				//Msg("%s,Gloss params are %f, %f", item.first.c_str(), gcoef, goffset);

				if (glossparams){
					glossparams->coef = gcoef;
					glossparams->offset = goffset;
				}
				else
					glossparams = new cl_gloss_coef_and_offset(gcoef, goffset);
			}
		}//"specification"
	}//file-exist

#ifdef _EDITOR
	FS.update_path			(fname,"$game_textures$","textures_types.ltx");

	if (FS.exist(fname))
	{
		CInifile			ini(fname);
		if (ini.section_exist("types"))
		{
			CInifile::Sect& 	data = ini.r_section("types");
			for (CInifile::SectCIt I=data.Data.begin(); I!=data.Data.end(); I++)	
			{
				CInifile::Item& item	= *I;

				texture_desc& desc		= m_texture_details[item.first];

				//desc.m_type				= (u16)atoi(item.second.c_str());
			}
		}//"types"
	}//file-exist
#endif
}

void CTextureDescrMngr::LoadTHM()
{
	FS_FileSet				flist;
	FS.file_list			(flist,"$game_textures$",FS_ListFiles,"*.thm");
	Msg						("count of .thm files=%d", flist.size());
	FS_FileSetIt It			= flist.begin();
	FS_FileSetIt It_e		= flist.end();
	STextureParams			tp;
	string_path				fn;
	for(;It!=It_e;++It)
	{
		
		FS.update_path		(fn,"$game_textures$", (*It).name.c_str());
		IReader* F			= FS.r_open(fn);
		xr_strcpy				(fn,(*It).name.c_str());
		fix_texture_thm_name(fn);

		R_ASSERT			(F->find_chunk(THM_CHUNK_TYPE));
		F->r_u32			();
		tp.Clear			();
		tp.Load				(*F);
		FS.r_close			(F);

		if (STextureParams::ttImage == tp.type ||
			STextureParams::ttTerrain == tp.type ||
			STextureParams::ttNormalMap == tp.type)
		{

			texture_desc& desc = m_texture_details[fn];
			cl_dt_scaler*&	dts = m_detail_scalers[fn];

			if (tp.detail_name.size() &&
				tp.flags.is_any(STextureParams::flDiffuseDetail | STextureParams::flBumpDetail))
			{
				u8 bumpdiffuse_ltx = 0;
				shared_str detail_name_ltx;
				if (desc.m_assoc){
					bumpdiffuse_ltx = desc.m_assoc->usage;
					detail_name_ltx = desc.m_assoc->detail_name;
					xr_delete(desc.m_assoc);
				}

				desc.m_assoc = new texture_assoc();
				desc.m_assoc->detail_name = tp.detail_name;
				if (dts)
					dts->scale = tp.detail_scale;
				else
					dts = new cl_dt_scaler(tp.detail_scale);

				desc.m_assoc->usage = 0;

				if (tp.flags.is(STextureParams::flDiffuseDetail))
					desc.m_assoc->usage |= (1 << 0);

				if (tp.flags.is(STextureParams::flBumpDetail))
					desc.m_assoc->usage |= (1 << 1);

				if (bumpdiffuse_ltx != 0){
					desc.m_assoc->usage = bumpdiffuse_ltx;
					Msg("BumpDiffuse Flag is assigned in LTX, skipping THM (%s) BumpDiffuse Flag setting", fn);
				}
				if (detail_name_ltx.size()>0){
					desc.m_assoc->detail_name = detail_name_ltx;
					Msg("Detail Name is assigned in LTX, skipping THM (%s) Detail Name setting", fn);
				}

			}
			bool use_p_ltx = false;
			shared_str bump_name_ltx;
			float material_ltx = -1.0;
			if (desc.m_spec){
				use_p_ltx = desc.m_spec->m_use_steep_parallax;
				bump_name_ltx = desc.m_spec->m_bump_name;
				material_ltx = desc.m_spec->m_material;
				xr_delete(desc.m_spec);
			}

			desc.m_spec = new texture_spec();
			desc.m_spec->m_material = tp.material + tp.material_weight;

			desc.m_spec->m_use_steep_parallax = false;
			if(tp.bump_mode==STextureParams::tbmUse)
			{
				desc.m_spec->m_bump_name	= tp.bump_name;
			}
			else if (tp.bump_mode==STextureParams::tbmUseParallax)
			{
				desc.m_spec->m_bump_name	= tp.bump_name;
				desc.m_spec->m_use_steep_parallax = true;
			}

			if (material_ltx != -1.0){
				desc.m_spec->m_material = material_ltx;
				Msg("Material is assigned in LTX, skipping THM (%s) Material setting", fn);
			}
			if (use_p_ltx){//tatarinrafa: if already true from LTX, dont overwrite
				desc.m_spec->m_use_steep_parallax = use_p_ltx;
				Msg("Parallax is assigned true in LTX, skipping THM (%s) Parallax setting", fn);
			}
			if (bump_name_ltx.size()>0){
				desc.m_spec->m_bump_name = bump_name_ltx;
				Msg("Bump Name is assigned in LTX, skipping THM (%s) Bump Name setting", fn);
			}
		}
	}
}

void CTextureDescrMngr::Load()
{
#ifdef DEBUG
	CTimer					TT;
	TT.Start				();
#endif // #ifdef DEBUG

	LoadLTX					();
	LoadTHM					();

#ifdef DEBUG
	Msg("load time=%d ms",TT.GetElapsed_ms());
#endif // #ifdef DEBUG
}

void CTextureDescrMngr::UnLoad()
{
	map_TD::iterator I = m_texture_details.begin();
	map_TD::iterator E = m_texture_details.end();
	for(;I!=E;++I)
	{
		xr_delete(I->second.m_assoc);
		xr_delete(I->second.m_spec);
	}
	m_texture_details.clear	();
}

CTextureDescrMngr::~CTextureDescrMngr()
{
	map_CS::iterator I = m_detail_scalers.begin();
	map_CS::iterator E = m_detail_scalers.end();

	for(;I!=E;++I)
		xr_delete(I->second);

	m_detail_scalers.clear	();
}

shared_str CTextureDescrMngr::GetBumpName(const shared_str& tex_name) const
{
	map_TD::const_iterator I = m_texture_details.find	(tex_name);
	if (I!=m_texture_details.end())
	{
		if(I->second.m_spec)
		{
			return I->second.m_spec->m_bump_name;
		}	
	}
	return "";
}

BOOL CTextureDescrMngr::UseSteepParallax(const shared_str& tex_name) const
{
	map_TD::const_iterator I = m_texture_details.find	(tex_name);
	if (I!=m_texture_details.end())
	{
		if(I->second.m_spec)
		{
			return I->second.m_spec->m_use_steep_parallax;
		}	
	}
	return FALSE;
}

void CTextureDescrMngr::GetGlossParams(const shared_str& tex_name, R_constant_setup* &GlossParams) const
{
	map_TD::const_iterator I = m_texture_details.find(tex_name);
	if (I != m_texture_details.end())
	{
		if (I->second.m_spec)
		{
			GlossParams = I->second.m_spec->textureglossparams;
		}
	}

}

u8 CTextureDescrMngr::TessMethod(const shared_str& tex_name) const
{
	map_TD::const_iterator I = m_texture_details.find(tex_name);
	if (I != m_texture_details.end())
	{
		if (I->second.m_assoc)
		{
			return I->second.m_assoc->m_tesselation_method;
		}
	}
	return 0;
}

float CTextureDescrMngr::GetMaterial(const shared_str& tex_name) const
{
	map_TD::const_iterator I = m_texture_details.find	(tex_name);
	if (I!=m_texture_details.end())
	{
		if(I->second.m_spec)
		{
			return I->second.m_spec->m_material;
		}
	}
	return 1.0f;
}

void CTextureDescrMngr::GetTextureUsage	(const shared_str& tex_name, BOOL& bDiffuse, BOOL& bBump) const
{
	map_TD::const_iterator I = m_texture_details.find	(tex_name);
	if (I!=m_texture_details.end())
	{
		if(I->second.m_assoc)
		{
			u8 usage	= I->second.m_assoc->usage;
			bDiffuse	= !!(usage & (1<<0));
			bBump		= !!(usage & (1<<1));
		}	
	}
}

BOOL CTextureDescrMngr::GetDetailTexture(const shared_str& tex_name, LPCSTR& res, R_constant_setup* &CS) const
{
	map_TD::const_iterator I = m_texture_details.find	(tex_name);
	if (I!=m_texture_details.end())
	{
		if(I->second.m_assoc)
		{
            texture_assoc* TA = I->second.m_assoc;
			res	= TA->detail_name.c_str();
			map_CS::const_iterator It2 = m_detail_scalers.find(tex_name);
			CS	= It2==m_detail_scalers.end()?0:It2->second;//TA->cs;
			return TRUE;
		}
	}
	return FALSE;
}

