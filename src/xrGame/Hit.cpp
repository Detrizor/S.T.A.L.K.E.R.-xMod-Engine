#include "stdafx.h"
#include "alife_space.h"
#include "hit.h"
//#include "ode_include.h"
#include "../xrEngine/bone.h"
#include "xrMessages.h"
#include "Level.h"
#include "../xrphysics/mathutils.h"
SHit::SHit(float main_damageA, Fvector &dirA, CObject *whoA, u16 elementA, Fvector p_in_bone_spaceA, \
	float impulseA, ALife::EHitType hit_typeA, float pierce_damageA, float pierce_damage_armorA, ALife::EHitType pierce_hit_typeA)
{
		main_damage				= main_damageA							;
		dir						.set(dirA)								;
		who						= whoA									;
		if (whoA)
			whoID				= whoA->ID()							;
		else 
			whoID				= 0										;
		boneID					= elementA								;
		p_in_bone_space			.set(p_in_bone_spaceA)					;
		impulse					= impulseA								;

		hit_type				= hit_typeA								;
		pierce_damage			= pierce_damageA						;
		pierce_damage_armor		= pierce_damage_armorA					;
		pierce_hit_type			= pierce_hit_typeA						;
		PACKET_TYPE				= 0										;
		BulletID				= 0										;
		SenderID				= 0										;
}

SHit::SHit()
{
	invalidate();
}

void SHit::invalidate()
{
	Time					= 0											;
	PACKET_TYPE				= 0											;
	DestID					= 0											;

	main_damage				=-phInfinity								;
	dir						.set(-phInfinity,-phInfinity,-phInfinity)	;
	who						=NULL										;
	whoID					= 0											;
	weaponID				= 0											;

	boneID					=BI_NONE									;
	p_in_bone_space		.set(-phInfinity,-phInfinity,-phInfinity)		;

	impulse					=-phInfinity								;
	hit_type				=ALife::eHitTypeMax							;

	pierce_damage			= 0.f										;
	pierce_damage_armor		= 0.f										;
	pierce_hit_type			= ALife::eHitTypeMax						;
	BulletID				= 0											;
	SenderID				= 0											;
}

bool SHit::is_valide() const
{
	return hit_type!=ALife::eHitTypeMax;
}

void	SHit::GenHeader				(u16 PacketType, u16 ID)
{
	DestID = ID;
	PACKET_TYPE = PacketType;
	Time = Level().timeServer();
};

void SHit::Read_Packet				(NET_Packet	Packet)
{
	u16 type_dummy;	
	Packet.r_begin			(type_dummy);
	Packet.r_u32			(Time);
	Packet.r_u16			(PACKET_TYPE);
	Packet.r_u16			(DestID);
	Read_Packet_Cont		(Packet);
};

void SHit::Read_Packet_Cont		(NET_Packet	Packet)
{

	Packet.r_u16			(whoID);
	Packet.r_u16			(weaponID);
	Packet.r_dir			(dir);
	Packet.r_float			(main_damage);
	Packet.r_u16			(boneID);
	Packet.r_vec3			(p_in_bone_space);
	Packet.r_float			(impulse);

	hit_type				= (ALife::EHitType)Packet.r_u16();	//hit type

	if (hit_type == ALife::eHitTypeFireWound)
	{
		Packet.r_float		(pierce_damage);
		Packet.r_float		(pierce_damage_armor);
	}
	if (PACKET_TYPE == GE_HIT_STATISTIC)
	{
		Packet.r_u32(BulletID);
		Packet.r_u32(SenderID);
	}
}

void SHit::Write_Packet_Cont		(NET_Packet	&Packet)
{
	Packet.w_u16		(whoID);
	Packet.w_u16		(weaponID);
	Packet.w_dir		(dir);
	Packet.w_float		(main_damage);
	Packet.w_u16		(boneID);
	Packet.w_vec3		(p_in_bone_space);
	Packet.w_float		(impulse);
	Packet.w_u16		(u16(hit_type&0xffff));	
	if (hit_type == ALife::eHitTypeFireWound)
	{
		Packet.w_float	(pierce_damage);
		Packet.w_float	(pierce_damage_armor);
	}
	if (PACKET_TYPE == GE_HIT_STATISTIC)
	{
		Packet.w_u32(BulletID);
		Packet.w_u32(SenderID);
	}
}
void SHit::Write_Packet			(NET_Packet	&Packet)
{
	Packet.w_begin	(M_EVENT);
	Packet.w_u32		(Time);
	Packet.w_u16		(u16(PACKET_TYPE&0xffff));
	Packet.w_u16		(u16(DestID&0xffff));

	Write_Packet_Cont (Packet);	
};

#ifdef DEBUG
void SHit::_dump()
{
	Msg("SHit::_dump()---begin");
	Log("power=",power);
	Log("impulse=",impulse);
	Log("dir=",dir);
	Log("whoID=",whoID);
	Log("weaponID=",weaponID);
	Log("element=",boneID);
	Log("p_in_bone_space=",p_in_bone_space);
	Log("hit_type=",(int)hit_type);
	Log("armor_factor=",armor_factor);
	Msg("SHit::_dump()---end");
}
#endif
