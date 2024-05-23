//////////////////////////////////////////////////////////////////////
// ShootingObject.h: ��������� ��� ��������� ���������� �������� 
//					 (������ � ���������� �������) 	
//					 ������������ ����� �����, ������ ��������
//////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_space.h"
#include "../xrEngine/render.h"
#include "anticheat_dumpable_object.h"

class CCartridge;
class CParticlesObject;
class IRender_Sector;

extern const Fvector zero_vel;

#define WEAPON_MATERIAL_NAME "objects\\bullet"

class CShootingObject : public IAnticheatDumpable
{
protected:
			CShootingObject			();
	virtual ~CShootingObject		();

	void	reinit	();
	void	reload	(LPCSTR section) {};
	void	Load	(LPCSTR section);

	Fvector		m_vCurrentShootDir;
	Fvector		m_vCurrentShootPos;

protected:
	//ID ��������� ������� ���������� ��������
	u16			m_iCurrentParentID;


//////////////////////////////////////////////////////////////////////////
// Fire Params
//////////////////////////////////////////////////////////////////////////
protected:
	virtual void			LoadFireParams		(LPCSTR section); 		//���� ��������
	virtual bool			SendHitAllowed		(CObject* pUser);
	virtual void			FireBullet			(const Fvector& pos, 
        										const Fvector& dir, 
												float fire_disp,
												const CCartridge& cartridge,
												u16 parent_id,
												u16 weapon_id,
												bool send_hit);
	float					GetBulletSpeed() const			{return m_fStartBulletSpeed;}

	virtual void			FireStart			();
	virtual void			FireEnd				();
public:
	IC BOOL					IsWorking			()	const	{return bWorking;}
	virtual BOOL			ParentMayHaveAimBullet()		{return FALSE;}

protected:
	// Weapon fires now
	bool					bWorking;
	float					fOneShotTime;

	//�������� ������ ���� �� ������
	float					m_fStartBulletSpeed;
	//������������ ���������� ��������
	float					fireDistance;

	//����������� �� ����� ��������
	float					fireDispersionBase;

	//������� �������, �������������� �� �������
	float					fShotTimeCounter;

public:
	struct SilencerKoeffs
	{
		float	bullet_speed;
		float	fire_dispersion;

		SilencerKoeffs() { Reset(); }
		IC void Reset()
		{
			bullet_speed = 1.0f;
			fire_dispersion = 1.0f;
		}
	} m_silencer_koef;

protected:
	//��� ���������, ���� ��� ����� ����������� ������� ������������� 
	//������
	//float					m_fMinRadius;
	//float					m_fMaxRadius;

protected:
	Fcolor					light_base_color;
	float					light_base_range;
	Fcolor					light_build_color;
	float					light_build_range;
	ref_light				light_render;
	float					light_var_color;
	float					light_var_range;
	float					light_lifetime;
	u32						light_frame;
	float					light_time;
	//��������� ��������� �� ����� ��������
	bool					m_bLightShotEnabled;
protected:
	void					Light_Create		();
	void					Light_Destroy		();

	void					Light_Start			();
	void					Light_Render		(const Fvector& P);

	virtual	void			LoadLights			(LPCSTR section);
	virtual void			RenderLight			();
	virtual void			UpdateLight			();
	virtual void			StopLight			();
	virtual bool			IsHudModeNow		()=0;
//////////////////////////////////////////////////////////////////////////
// ����������� �������
//////////////////////////////////////////////////////////////////////////
protected:
	//������� ������������� �������
	virtual const Fvector&	get_CurrentFirePoint()		= 0;
	virtual const Fmatrix&	get_ParticlesXFORM()		= 0;
	virtual void			ForceUpdateFireParticles	(){};
	
	////////////////////////////////////////////////
	//����� ������� ��� ������ � ���������� ������
			void			StartParticles		(CParticlesObject*& pParticles, LPCSTR particles_name, const Fvector& pos, const Fvector& vel = zero_vel, bool auto_remove_flag = false);
			void			StopParticles		(CParticlesObject*& pParticles);
			void			UpdateParticles		(CParticlesObject*& pParticles, const Fvector& pos, const  Fvector& vel = zero_vel);

			void			LoadShellParticles	(LPCSTR section);
			void			LoadFlameParticles	(LPCSTR section);
	
	////////////////////////////////////////////////
	//������������� ������� ��� ���������
	//�������� ����
			void			StartFlameParticles	();
			void			StopFlameParticles	();
			void			UpdateFlameParticles();

	//�������� ����
			void			StartSmokeParticles	(const Fvector& play_pos, const Fvector& parent_vel);

	//�������� ������ �� ����
			void			StartShotParticles	();

	//�������� �����
			void			OnShellDrop			(const Fvector& play_pos, const Fvector& parent_vel);

protected:
	//��� �������� 1� � 2� ����� ��������
	shared_str				m_sFlameParticles;
	//��� ��������� ��� ����
	shared_str				m_sSmokeParticles;
	//��� ��������� ����� �� ����
	shared_str				m_sShotParticles;
	//��� ��������� ��� �����
	shared_str				m_sShellParticles;

	//������ ��������� ����
	CParticlesObject*		m_pFlameParticles;
};
