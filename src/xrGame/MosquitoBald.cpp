#include "stdafx.h"
#include "mosquitobald.h"
#include "ParticlesObject.h"
#include "level.h"
#include "physicsshellholder.h"
#include "../xrengine/xr_collide_form.h"

constexpr float secondary_hit_period = 1.f / 60.f;

CMosquitoBald::CMosquitoBald(void) 
{
	m_fHitImpulseScale		= 1.f;
	m_bLastBlowoutUpdate	= false;
}

CMosquitoBald::~CMosquitoBald(void) 
{
}

void CMosquitoBald::Load(LPCSTR section) 
{
	inherited::Load(section);
}


bool CMosquitoBald::BlowoutState()
{
	bool result = inherited::BlowoutState();
	if(!result)
	{
		m_bLastBlowoutUpdate = false;
		UpdateBlowout();
	}
	else if(!m_bLastBlowoutUpdate)
	{
		m_bLastBlowoutUpdate = true;
		UpdateBlowout();
	}

	return result;
}
//bool CMosquitoBald::SecondaryHitState()
//{
//	bool result = inherited::SecondaryHitState();
//	if(!result)
//		UpdateBlowout();
//
//	return result;
//}

void CMosquitoBald::Affect(SZoneObjectInfo* O) 
{
	CPhysicsShellHolder *pGameObject = smart_cast<CPhysicsShellHolder*>(O->object);
	if(!pGameObject) return;

	if(O->zone_ignore) return;

	Fvector P; 
	XFORM().transform_tiny(P,CFORM()->getSphere().P);

	Fvector hit_dir; 
	hit_dir.set(	::Random.randF(-.5f,.5f), 
					::Random.randF(.0f,1.f), 
					::Random.randF(-.5f,.5f)); 
	hit_dir.normalize();

	Fvector position_in_bone_space;

	VERIFY(!pGameObject->getDestroy());

	float dist = pGameObject->Position().distance_to(P) - pGameObject->Radius();
	float power = Power(dist>0.f?dist:0.f, Radius());
	float impulse = m_fHitImpulseScale*power*pGameObject->GetMass();

	if(power > 0.01f) 
	{
		position_in_bone_space.set(0.f,0.f,0.f);
		CreateHit(pGameObject->ID(),ID(),hit_dir,power,0,position_in_bone_space,impulse,m_eHitTypeBlowout);
		PlayHitParticles(pGameObject);
	}
}

void CMosquitoBald::UpdateSecondaryHit()
{
	if (m_dwAffectFrameNum == Device.dwFrame)
		return;
	m_dwAffectFrameNum					= Device.dwFrame;

	float dt							= Device.fTimeGlobal - m_affect_time;
	if (dt < secondary_hit_period)
		return;
	m_affect_time						= Device.fTimeGlobal;

	if (Device.dwPrecacheFrame)
		return;

	for (auto& O : m_ObjectInfoMap)
	{
		if (O.object->getDestroy() || O.zone_ignore)
			continue;

		auto psh						= smart_cast<CPhysicsShellHolder*>(O.object);
		if (!psh)
			continue;

		Fvector							P;
		XFORM().transform_tiny			(P, CFORM()->getSphere().P);
		float dist						= O.object->Position().distance_to(P) - O.object->Radius();
		float power						= m_fSecondaryHitPower * RelativePower(max(dist, 0.f), Radius());
		if (power < 0.f)
			continue;

		float impulse					= m_fHitImpulseScale * power * psh->GetMass();

		for (float step = secondary_hit_period; step < dt; step += secondary_hit_period)
		{
			Fvector hit_dir				= {
				::Random.randF			(-.5f, .5f),
				::Random.randF			(.0f, 1.f),
				::Random.randF			(-.5f, .5f)
			};
			hit_dir.normalize			();
			CreateHit					(O.object->ID(), ID(), hit_dir, power, 0, vZero, impulse, m_eHitTypeBlowout);
		}
	}
}
