//////////////////////////////////////////////////////////////////////
// ShootingObject.h: интерфейс дл€ семейства стрел€ющих объектов 
//					 (оружие и осколочные гранаты) 	
//					 обеспечивает набор хитов, звуков рикошетп
//////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_space.h"
#include "../xrEngine/render.h"
#include "WeaponAmmo.h"

class CParticlesObject;
class IRender_Sector;

extern const Fvector zero_vel;

#define WEAPON_MATERIAL_NAME "objects\\bullet"

class CShootingObject : public IAnticheatDumpable
{
protected:
			CShootingObject			();

	void	reload	(LPCSTR section) {};
	void	Load	(LPCSTR section);

	Fvector		m_vCurrentShootDir;
	Fvector		m_vCurrentShootPos;

protected:
	//ID персонажа который иницировал действие
	u16			m_iCurrentParentID;


//////////////////////////////////////////////////////////////////////////
// Fire Params
//////////////////////////////////////////////////////////////////////////
protected:
	virtual void			LoadFireParams		(LPCSTR section); 		//сила выстрела
	virtual bool			SendHitAllowed		(CObject* pUser);
	virtual void			FireBullet			(const Fvector& pos, 
        										const Fvector& dir, 
												float fire_disp,
												const CCartridge& cartridge,
												u16 parent_id,
												u16 weapon_id,
												bool send_hit);

	virtual void			FireStart			();
	virtual void			FireEnd				();
public:
	IC BOOL					IsWorking			()	const	{return bWorking;}
	virtual BOOL			ParentMayHaveAimBullet()		{return FALSE;}

protected:
	// Weapon fires now
	bool					bWorking;
	float					fOneShotTime;

	//максимальное рассто€ние стрельбы
	float					fireDistance;

	//рассеивание во врем€ стрельбы
	float					fireDispersionBase;

	//счетчик времени, затрачиваемого на выстрел
	float					fShotTimeCounter;

public:
	struct SMuzzleKoeffs
	{
		float	bullet_speed		= 1.f;
		float	fire_dispersion		= 1.f;
	} m_muzzle_koefs;

protected:
	//дл€ сталкеров, чтоб они знали эффективные границы использовани€ 
	//оружи€
	//float					m_fMinRadius;
	//float					m_fMaxRadius;

protected:
	Fcolor					light_build_color;
	float					light_build_range;
	ref_light				light_render;
	u32						light_frame;
	float					light_time = -1.f;

protected:
	void					Light_Create		();
	void					Light_Destroy		();

	void					Light_Start			();
	void					Light_Render		(const Fvector& P);

	virtual void			RenderLight			();
	virtual void			UpdateLight			();
	virtual void			StopLight			();
	virtual bool			IsHudModeNow		()=0;
//////////////////////////////////////////////////////////////////////////
// партиклова€ система
//////////////////////////////////////////////////////////////////////////
protected:
	//функции родительского объекта
	virtual const Fvector&	get_CurrentFirePoint()		= 0;
	virtual const Fmatrix&	get_ParticlesXFORM()		= 0;
	virtual void			ForceUpdateFireParticles	(){};
	
	////////////////////////////////////////////////
	//общие функции дл€ работы с партиклами оружи€
			void			StartParticles		(CParticlesObject*& pParticles, LPCSTR particles_name, const Fvector& pos, const Fvector& vel = zero_vel, bool auto_remove_flag = false);
			void			StopParticles		(CParticlesObject*& pParticles);
			void			UpdateParticles		(CParticlesObject*& pParticles, const Fvector& pos, const  Fvector& vel = zero_vel);
	
	////////////////////////////////////////////////
	//спецефические функции дл€ партиклов
	//партиклы огн€
			void			StartFlameParticles	();
			void			StopFlameParticles	();
			void			UpdateFlameParticles();

	//партиклы дыма
			void			StartSmokeParticles	(const Fvector& play_pos, const Fvector& parent_vel);

	//партиклы полосы от пули
			void			StartShotParticles	();

	//партиклы гильз
			void			OnShellDrop			(const Fvector& play_pos, const Fvector& parent_vel);

protected:
	//объект партиклов огн€
	CParticlesObject*					m_flame_particles						= nullptr;
	float								m_barrel_len							= 1.f;
	bool								m_silencer								= false;
	bool								m_flash_hider							= false;

	CCartridge							m_shot_cartridge;
};
