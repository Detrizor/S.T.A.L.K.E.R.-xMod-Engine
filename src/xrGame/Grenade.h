#pragma once
#include "missile.h"
#include "explosive.h"
#include "../xrEngine/feel_touch.h"

class CGrenade : public CMissile, public CExplosive
{
	typedef CMissile		inherited;

public:
							CGrenade							();


	virtual void			Load								(LPCSTR section);
	
	virtual BOOL 			net_Spawn							(CSE_Abstract* DC);
	virtual void 			net_Destroy							();
	virtual void 			net_Relcase							(CObject* O );

	virtual void 			OnH_A_Independent					();
	virtual void 			OnH_A_Chield						();
	virtual void			DiscardState						();
	
	virtual void 			OnEvent								(NET_Packet& P, u16 type);
	virtual bool			DropGrenade							();			//in this case if grenade state is eReady, it should Throw
	
	virtual void 			OnAnimationEnd						(u32 state);
	virtual void 			UpdateCL							();

	virtual void 			Throw();
	virtual void 			Destroy();

	virtual bool			Useful								() const;
	virtual void			State								(u32 state, u32 old_state);

	virtual void			OnH_B_Chield						()				{inherited::OnH_B_Chield();}

	virtual	void			Hit									(SHit* pHDS);

	virtual ALife::_TIME_ID	TimePassedAfterIndependant			() const;

			void			PutNextToSlot						();

	void					DeactivateItem						(u16 slot) override;

protected:
	ALife::_TIME_ID			m_dwGrenadeRemoveTime;
	ALife::_TIME_ID			m_dwGrenadeIndependencyTime;

protected:
	ESoundTypes				m_eSoundCheckout;

private:
	float					m_grenade_detonation_threshold_hit;
	bool					m_thrown;

protected:
	virtual	void			UpdateXForm							()		{ CMissile::UpdateXForm(); };

public:

	virtual BOOL			UsedAI_Locations					();
	virtual CExplosive		*cast_explosive						()	{return this;}
	virtual CMissile		*cast_missile						()	{return this;}
	virtual CHudItem		*cast_hud_item						()	{return this;}
	virtual CGameObject		*cast_game_object					()	{return this;}
	virtual IDamageSource	*cast_IDamageSource					()	{return CExplosive::cast_IDamageSource();}

	typedef					fastdelegate::FastDelegate<void(CGrenade*)>	destroy_callback;
	void					set_destroy_callback				(destroy_callback callback) { m_destroy_callback = callback; }

private:
	destroy_callback		m_destroy_callback;

private:
	void								onMovementChanged					O$	();
};
