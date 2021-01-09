#include "stdafx.h"
#pragma once

#include "Rain.h"
#include "igame_persistent.h"
#include "environment.h"

#ifdef _EDITOR
    #include "ui_toolscustom.h"
#else
    #include "render.h"
	#include "igame_level.h"
	#include "../xrcdb/xr_area.h"
	#include "xr_object.h"
#endif

// gr1ph start

SRainParams::SRainParams() : dwReferences(1)
{
	max_desired_items		= pSettings->r_s32						(RAIN_MANAGER_LTX, "max_desired_items");	// 2500;
	source_radius			= pSettings->r_float					(RAIN_MANAGER_LTX, "source_radius");		// 12.5f;
	source_offset			= pSettings->r_float					(RAIN_MANAGER_LTX, "source_offset");		// 40.f;
	max_distance			= source_offset * pSettings->r_float	(RAIN_MANAGER_LTX, "max_distance_factor");	// 1.25f;
	sink_offset				= -(max_distance - source_offset);
	drop_length				= pSettings->r_float					(RAIN_MANAGER_LTX, "drop_length");			// 5.f;
	drop_width				= pSettings->r_float					(RAIN_MANAGER_LTX, "drop_width");			// 0.30f;
	drop_angle				= pSettings->r_float					(RAIN_MANAGER_LTX, "drop_angle");			// 3.0f;
	drop_max_angle			= deg2rad(pSettings->r_float			(RAIN_MANAGER_LTX, "drop_max_angle"));		// 80.f
	drop_max_wind_vel		= pSettings->r_float					(RAIN_MANAGER_LTX, "drop_max_wind_vel");	// 2000.0f;
	drop_speed_min			= pSettings->r_float					(RAIN_MANAGER_LTX, "drop_speed_min");		// 40.f;
	drop_speed_max			= pSettings->r_float					(RAIN_MANAGER_LTX, "drop_speed_max");		// 80.f;
	max_particles			= pSettings->r_s32						(RAIN_MANAGER_LTX, "max_particles");		// 1000;
	particles_cache			= pSettings->r_s32						(RAIN_MANAGER_LTX, "particles_cache");		// 400;
	particles_time			= pSettings->r_float					(RAIN_MANAGER_LTX, "particles_time");		// .3f;
}

SRainParams *params = NULL;
 
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEffect_Rain::CEffect_Rain()
{
	if (!params)
		params = new SRainParams();
	else
		params->dwReferences++;
	state							= stIdle;
	
	snd_Ambient.create				("ambient\\rain",st_Effect,sg_Undefined);
	snd_Wind.create					("ambient\\wind",st_Effect,sg_Undefined);
	bWindWorking					= false;

	p_create						();
}

CEffect_Rain::~CEffect_Rain()
{
	snd_Ambient.destroy				();
	snd_Wind.destroy				();

	// Cleanup
	p_destroy						();
	params->dwReferences--;
	if (!params->dwReferences)
		xr_free(params);
}

// Born
void	CEffect_Rain::Born		(Item& dest, float radius)
{
	Fvector		axis;	
    axis.set			(0,-1,0);
	float gust			= g_pGamePersistent->Environment().wind_strength_factor/10.f;
	float k				= g_pGamePersistent->Environment().CurrentEnv->wind_velocity*gust/params->drop_max_wind_vel;
	clamp				(k,0.f,1.f);
	float	pitch		= params->drop_max_angle*k-PI_DIV_2;
    axis.setHP			(g_pGamePersistent->Environment().CurrentEnv->wind_direction,pitch);
    
	Fvector&	view	= Device.vCameraPosition;
	float		angle	= ::Random.randF	(0,PI_MUL_2);
	float		dist	= ::Random.randF	(); dist = _sqrt(dist)*radius; 
	float		x		= dist*_cos		(angle);
	float		z		= dist*_sin		(angle);
	dest.D.random_dir	(axis,deg2rad(params->drop_angle));
	dest.P.set			(x+view.x-dest.D.x*params->source_offset,params->source_offset+view.y,z+view.z-dest.D.z*params->source_offset);
	dest.fSpeed			= ::Random.randF	(params->drop_speed_min,params->drop_speed_max);

	float height		= params->max_distance;
	RenewItem			(dest,height,RayPick(dest.P,dest.D,height,collide::rqtBoth));
}

BOOL CEffect_Rain::RayPick(const Fvector& s, const Fvector& d, float& range, collide::rq_target tgt)
{
	BOOL bRes 			= TRUE;
#ifdef _EDITOR
    Tools->RayPick		(s,d,range);
#else
	collide::rq_result	RQ;
	CObject* E 			= g_pGameLevel->CurrentViewEntity();
	bRes 				= g_pGameLevel->ObjectSpace.RayPick( s,d,range,tgt,RQ,E);	
    if (bRes) range 	= RQ.range;
#endif
    return bRes;
}

void CEffect_Rain::RenewItem(Item& dest, float height, BOOL bHit)
{
	dest.uv_set			= Random.randI(2);
    if (bHit){
		dest.dwTime_Life= Device.dwTimeGlobal + iFloor(1000.f*height/dest.fSpeed) - Device.dwTimeDelta;
		dest.dwTime_Hit	= Device.dwTimeGlobal + iFloor(1000.f*height/dest.fSpeed) - Device.dwTimeDelta;
		dest.Phit.mad	(dest.P,dest.D,height);
	}else{
		dest.dwTime_Life= Device.dwTimeGlobal + iFloor(1000.f*height/dest.fSpeed) - Device.dwTimeDelta;
		dest.dwTime_Hit	= Device.dwTimeGlobal + iFloor(2*1000.f*height/dest.fSpeed)-Device.dwTimeDelta;
		dest.Phit.set	(dest.P);
	}
}

void	CEffect_Rain::OnFrame	()
{
#ifndef _EDITOR
	if (!g_pGameLevel)			return;
#endif

#ifdef DEDICATED_SERVER
	return;
#endif

	// Parse states
	float	factor				= g_pGamePersistent->Environment().CurrentEnv->rain_density;
	float	wind_volume			= g_pGamePersistent->Environment().CurrentEnv->wind_volume;
	bool	rain_enabled		= (factor >= EPS_L);
	bool	wind_enabled		= (wind_volume >= EPS_L);

	static float hemi_factor	= 0.f;
#ifndef _EDITOR
	CObject* E 					= g_pGameLevel->CurrentViewEntity();
	if (E&&E->renderable_ROS())
	{
		float* hemi_cube		= E->renderable_ROS()->get_luminocity_hemi_cube();
		float hemi_val			= _max(hemi_cube[0],hemi_cube[1]);
		hemi_val				= _max(hemi_val, hemi_cube[2]);
		hemi_val				= _max(hemi_val, hemi_cube[3]);
		hemi_val				= _max(hemi_val, hemi_cube[5]);

		float f					= hemi_val;
		float t					= Device.fTimeDelta;
		clamp					(t, 0.001f, 1.0f);
		hemi_factor				= hemi_factor*(1.0f-t) + f*t;
	}
#endif

	Fvector					sndP;
	sndP.mad(Device.vCameraPosition, Fvector().set(0, 1, 0), params->source_offset);

	if (!bWindWorking)
	{
		if (wind_enabled)
		{
			snd_Wind.play(0, sm_Looped);
			snd_Wind.set_position(Fvector().set(0, 0, 0));
			snd_Wind.set_range(params->source_offset, params->source_offset*2.f);

			bWindWorking = true;
		}
	} else {
		if (wind_enabled)
		{
			// wind sound
			if (snd_Wind._feedback())
			{
				snd_Wind.set_position(sndP);
				snd_Wind.set_volume(wind_volume);
			}
		} else {
			snd_Wind.stop();
			bWindWorking = false;
		}
	}

	switch (state)
	{
	case stIdle:		
		if (!rain_enabled)		return;
		state					= stWorking;
		snd_Ambient.play		(0,sm_Looped);
		snd_Ambient.set_position(Fvector().set(0,0,0));
		snd_Ambient.set_range	(params->source_offset,params->source_offset*2.f);
	break;
	case stWorking:
		if (!rain_enabled)
		{
			state				= stIdle;
			snd_Ambient.stop	();
			return;
		}
		break;
	}

	// ambient sound
	if (snd_Ambient._feedback())
	{
		snd_Ambient.set_position(sndP);
		snd_Ambient.set_volume	(_max(0.1f,factor) * hemi_factor );
	}
}

//#include "xr_input.h"
void	CEffect_Rain::Render	()
{
#ifndef _EDITOR
	if (!g_pGameLevel)			return;
#endif

	m_pRender->Render(*this, params);
}

// startup _new_ particle system
void	CEffect_Rain::Hit		(Fvector& pos)
{
	if (0!=::Random.randI(2))	return;
	Particle*	P	= p_allocate();
	if (0==P)	return;

	const Fsphere &bv_sphere = m_pRender->GetDropBounds();

	P->time						= params->particles_time;
	P->mXForm.rotateY			(::Random.randF(PI_MUL_2));
	P->mXForm.translate_over	(pos);
	P->mXForm.transform_tiny	(P->bounds.P, bv_sphere.P);
	P->bounds.R					= bv_sphere.R;

}

// initialize particles pool
void CEffect_Rain::p_create		()
{
	// pool
	particle_pool.resize	(params->max_particles);
	for (u32 it=0; it<particle_pool.size(); it++)
	{
		Particle&	P	= particle_pool[it];
		P.prev			= it?(&particle_pool[it-1]):0;
		P.next			= (it<(particle_pool.size()-1))?(&particle_pool[it+1]):0;
	}
	
	// active and idle lists
	particle_active	= 0;
	particle_idle	= &particle_pool.front();
}

// destroy particles pool
void CEffect_Rain::p_destroy	()
{
	// active and idle lists
	particle_active	= 0;
	particle_idle	= 0;
	
	// pool
	particle_pool.clear	();
}

// _delete_ node from _list_
void CEffect_Rain::p_remove	(Particle* P, Particle* &LST)
{
	VERIFY		(P);
	Particle*	prev		= P->prev;	P->prev = NULL;
	Particle*	next		= P->next;	P->next	= NULL;
	if (prev) prev->next	= next;
	if (next) next->prev	= prev;
	if (LST==P)	LST			= next;
}

// insert node at the top of the head
void CEffect_Rain::p_insert	(Particle* P, Particle* &LST)
{
	VERIFY		(P);
	P->prev					= 0;
	P->next					= LST;
	if (LST)	LST->prev	= P;
	LST						= P;
}

// determine size of _list_
int CEffect_Rain::p_size	(Particle* P)
{
	if (0==P)	return 0;
	int cnt = 0;
	while (P)	{
		P	=	P->next;
		cnt +=	1;
	}
	return cnt;
}

// alloc node
CEffect_Rain::Particle*	CEffect_Rain::p_allocate	()
{
	Particle*	P			= particle_idle;
	if (0==P)				return NULL;
	p_remove	(P,particle_idle);
	p_insert	(P,particle_active);
	return		P;
}

// xr_free node
void	CEffect_Rain::p_free(Particle* P)
{
	p_remove	(P,particle_active);
	p_insert	(P,particle_idle);
}
