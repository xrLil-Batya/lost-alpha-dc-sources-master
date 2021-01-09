#ifndef _TextureDescrManager_included_
#define _TextureDescrManager_included_

#pragma once
#include "ETextureParams.h"

class cl_dt_scaler;

class cl_gloss_coef_and_offset : public R_constant_setup {
public:
	float				coef;
	float				offset;

	cl_gloss_coef_and_offset(float s, float x) : coef(s), offset(x) {};
	virtual void setup(R_constant* C)
	{
		RCache.set_c(C, coef, offset, 0, 0);
	}
};

class CTextureDescrMngr
{
	struct texture_assoc
	{
		shared_str			detail_name;
		u8					usage;
		u8					m_tesselation_method;
		texture_assoc() : /*cs(NULL),*/ usage(0) {}
		~texture_assoc		() { /*xr_delete(cs);*/ }

	};
	struct texture_spec
	{
		shared_str			m_bump_name;
		float				m_material;
		bool				m_use_steep_parallax;
		cl_gloss_coef_and_offset* textureglossparams;
		texture_spec() { textureglossparams = NULL; }
		~texture_spec() { xr_delete(textureglossparams); }
	};
	struct texture_desc{
		texture_assoc*		m_assoc;
		texture_spec*		m_spec;
        texture_desc            ():m_assoc(NULL),m_spec(NULL){}
	};
	DEFINE_MAP(shared_str, texture_desc,	map_TD,	map_TDIt);
	DEFINE_MAP(shared_str, cl_dt_scaler*,	map_CS,	map_CSIt);

	map_TD									m_texture_details;
	map_CS									m_detail_scalers;

	void		LoadLTX		();
	void		LoadTHM		();

public:
				~CTextureDescrMngr();
	void		Load		();
	void		UnLoad		();
public:
	shared_str	GetBumpName		(const shared_str& tex_name) const;
	float		GetMaterial		(const shared_str& tex_name) const;
	void		GetTextureUsage	(const shared_str& tex_name, BOOL& bDiffuse, BOOL& bBump) const;
	BOOL		GetDetailTexture(const shared_str& tex_name, LPCSTR& res, R_constant_setup* &CS) const;
	BOOL		UseSteepParallax(const shared_str& tex_name) const;
	u8			TessMethod		(const shared_str& tex_name) const;
	void		GetGlossParams(const shared_str& tex_name, R_constant_setup* &GlossParams) const;
};
#endif