// Level_Bullet_Manager.cpp:	для обеспечения полета пули по траектории
//								все пули и осколки передаются сюда
//								(для просчета столкновений и их визуализации)
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Level_Bullet_Manager.h"
#include "entity.h"
#include "../xrEngine/gamemtllib.h"
#include "level.h"
#include "gamepersistent.h"
#include "game_cl_base.h"
#include "xrmessages.h"
#include "../Include/xrRender/Kinematics.h"
#include "Actor.h"
#include "AI/Stalker/ai_stalker.h"
#include "character_info.h"
#include "game_cl_base_weapon_usage_statistic.h"
#include "../xrcdb/xr_collide_defs.h"
#include "../xrengine/xr_collide_form.h"
#include "weapon.h"
#include "ik/math3d.h"
#include "actor.h"
#include "ai/monsters/basemonster/base_monster.h"

#define BULLET_MANAGER_SECTION						"bullet_manager"

#define RICOCHET_THRESHOLD								0.1
#define STUCK_THRESHOLD									0.4

//расстояния не пролетев которого пуля не трогает того кто ее пустил
extern float gCheckHitK;

//test callback функция 
//  object - object for testing
//return TRUE-тестировать объект / FALSE-пропустить объект
BOOL CBulletManager::test_callback(const collide::ray_defs& rd, CObject* object, LPVOID params)
{
	if (!object)
		return TRUE;

	bullet_test_callback_data* pData	= (bullet_test_callback_data*)params;
	SBullet* bullet = pData->pBullet;

	if( (object->ID() == bullet->parent_id)		&&  
		(bullet->fly_dist<parent_ignore_distance)	&&
		(!bullet->flags.ricochet_was))			return FALSE;

	BOOL bRes						= TRUE;
	CEntity*	entity			= smart_cast<CEntity*>(object);
	if (entity&&entity->g_Alive()&&(entity->ID()!=bullet->parent_id)){
		ICollisionForm*	cform	= entity->collidable.model;
		if ((NULL!=cform) && (cftObject==cform->Type())){
			CActor* actor		= smart_cast<CActor*>(entity);
			CAI_Stalker* stalker= smart_cast<CAI_Stalker*>(entity);
			// в кого попали?
			if (actor||stalker){
				// попали в актера или сталкера
				Fsphere S		= cform->getSphere();
				entity->XFORM().transform_tiny	(S.P)	;
				float dist		= rd.range;
				// проверим попали ли мы в описывающую сферу 
				if (Fsphere::rpNone!=S.intersect_full(bullet->pos, bullet->dir, dist))
				{
					// да попали, найдем кто стрелял
					bool play_whine				= true;
					CObject* initiator			= Level().Objects.net_Find	(bullet->parent_id);
					if (actor){
						// попали в актера
						float hpf				= 1.f;
						float hit_probability	= 1.f;
						CAI_Stalker				*stalker = smart_cast<CAI_Stalker*>(initiator);
						if (stalker)
							hpf					= stalker->SpecificCharacter().hit_probability_factor();

						float					dist_factor = 1.f;
						CObject					*weapon_object = Level().Objects.net_Find	(bullet->weapon_id);
						if (weapon_object)
						{
							CWeapon* weapon		= smart_cast<CWeapon*>(weapon_object);
							if (weapon)
							{
								hit_probability	= weapon->hit_probability();
								float fly_dist	= bullet->fly_dist+dist;
								dist_factor		= _min(1.f,fly_dist/Level().BulletManager().m_fHPMaxDist);
							}
						}

						float ahp				= dist_factor * hit_probability + (1.f - dist_factor) * 1.f;
						if (Random.randF(0.f, 1.f) > (ahp * hpf))
						{ 
							bRes				= FALSE;	// don't hit actor
							play_whine			= true;		// play whine sound
						}
						else
						{
							// real test actor CFORM
							Level().BulletManager().m_rq_results.r_clear();

							if (cform->_RayQuery(rd,Level().BulletManager().m_rq_results))
							{
								bRes			= TRUE;		// hit actor
								play_whine		= false;	// don't play whine sound
							}
							else
							{
								bRes			= FALSE;	// don't hit actor
								play_whine		= true;		// play whine sound
							}
						}
					}
					// play whine sound
					if (play_whine)
					{
						Fvector					pt;
						pt.mad					(bullet->pos, bullet->dir, dist);
						Level().BulletManager().PlayWhineSound(bullet,initiator,pt);
					}
				}
				else
					// don't test this object again (return FALSE)
					bRes						= FALSE;
			}
		}
	}
	return bRes;
}

//callback функция 
//	result.O;		// 0-static else CObject*
//	result.range;	// range from start to element 
//	result.element;	// if (O) "num tri" else "num bone"
//	params;			// user defined abstract data
//	Device.Statistic.TEST0.End();
//return TRUE-продолжить трассировку / FALSE-закончить трассировку

void CBulletManager::FireShotmark (SBullet* bullet, const Fvector& vDir, const Fvector &vEnd, collide::rq_result& R, u16 target_material, const Fvector& vNormal, bool ShowMark)
{
	SGameMtlPair* mtl_pair	= GMLib.GetMaterialPair(bullet->bullet_material_idx, target_material);
	Fvector particle_dir	= vNormal;

	if (R.O)
	{
		particle_dir		 = vDir;
		particle_dir.invert	();

		//на текущем актере отметок не ставим
		if(Level().CurrentEntity() && Level().CurrentEntity()->ID() == R.O->ID()) return;

		if (mtl_pair && !mtl_pair->m_pCollideMarks->empty() && ShowMark)
		{
			//добавить отметку на материале
			Fvector p;
			p.mad(bullet->pos,bullet->dir,R.range-0.01f);
			if(!g_dedicated_server)
				::Render->add_SkeletonWallmark	(	&R.O->renderable.xform, 
													PKinematics(R.O->Visual()), 
													&*mtl_pair->m_pCollideMarks,
													p, 
													bullet->dir, 
													bullet->wallmark_size);
		}
	} 
	else 
	{
		//вычислить нормаль к пораженной поверхности
		Fvector*	pVerts	= Level().ObjectSpace.GetStaticVerts();
		CDB::TRI*	pTri	= Level().ObjectSpace.GetStaticTris()+R.element;

		if (mtl_pair && !mtl_pair->m_pCollideMarks->empty() && ShowMark)
		{
			//добавить отметку на материале
			::Render->add_StaticWallmark	(&*mtl_pair->m_pCollideMarks, vEnd, bullet->wallmark_size, pTri, pVerts);
		}
	}

	ref_sound* pSound = (!mtl_pair || mtl_pair->CollideSounds.empty())?
						NULL:&mtl_pair->CollideSounds[::Random.randI(0,mtl_pair->CollideSounds.size())];

	//проиграть звук
	if(pSound && ShowMark)
	{
		CObject* O			= Level().Objects.net_Find(bullet->parent_id );
		bullet->m_mtl_snd	= *pSound;
		bullet->m_mtl_snd.play_at_pos(O, vEnd, 0);
	}

	LPCSTR ps_name = ( !mtl_pair || mtl_pair->CollideParticles.empty() ) ? NULL : 
		*mtl_pair->CollideParticles[ ::Random.randI(0,mtl_pair->CollideParticles.size()) ];

	SGameMtl*	tgt_mtl = GMLib.GetMaterialByIdx(target_material);
	BOOL bStatic = !tgt_mtl->Flags.test(SGameMtl::flDynamic);

	if( (ps_name && ShowMark) || (bullet->flags.explosive && bStatic) )
	{
		VERIFY2					(
			(particle_dir.x*particle_dir.x+particle_dir.y*particle_dir.y+particle_dir.z*particle_dir.z) > flt_zero,
			make_string("[%f][%f][%f]", VPUSH(particle_dir))
		);
		Fmatrix pos;
		pos.k.normalize(particle_dir);
		Fvector::generate_orthonormal_basis(pos.k, pos.j, pos.i);
		pos.c.set(vEnd);
		if(ps_name && ShowMark)
		{
			//отыграть партиклы попадания в материал
			CParticlesObject* ps = CParticlesObject::Create(ps_name,TRUE);

			ps->UpdateParent( pos, zero_vel );
			GamePersistent().ps_needtoplay.push_back( ps );
		}

		if( bullet->flags.explosive && bStatic )
		{
			PlayExplodePS( pos );
		}
	}
}

void CBulletManager::StaticObjectHit	(CBulletManager::_event& E)
{
//	Fvector hit_normal;
	FireShotmark(&E.bullet, E.bullet.dir,	E.point, E.R, E.tgt_material, E.normal);
//	ObjectHit	(&E.bullet,					E.point, E.R, E.tgt_material, hit_normal);
}

static bool g_clear = false;
void CBulletManager::DynamicObjectHit	(CBulletManager::_event& E)
{
	//только для динамических объектов
	VERIFY(E.R.O);

	if ( CEntity* entity = smart_cast<CEntity*>(E.R.O) )
	{
		if ( !entity->in_solid_state() )
		{
			return;
		}
	}

	bool NeedShootmark = true;
	
	if (smart_cast<CActor*>(E.R.O))
	{
		game_PlayerState* ps = Game().GetPlayerByGameID(E.R.O->ID());
		if (ps && ps->testFlag(GAME_PLAYER_FLAG_INVINCIBLE))
		{
			NeedShootmark = false;
		};
	}
	else if ( CBaseMonster * monster = smart_cast<CBaseMonster *>(E.R.O) )
	{
		NeedShootmark	=	monster->need_shotmark();
	}
	
	//визуальное обозначение попадание на объекте
//	Fvector			hit_normal;
	FireShotmark	(&E.bullet, E.bullet.dir, E.point, E.R, E.tgt_material, E.normal, NeedShootmark);
	
	Fvector original_dir = E.bullet.dir;
	//ObjectHit(&E.bullet, E.end_point, E.R, E.tgt_material, hit_normal);

	SBullet_Hit hit_param = E.hit_result;

	// object-space
	//вычислить координаты попадания
	Fvector				p_in_object_space,position_in_bone_space;
	Fmatrix				m_inv;
	m_inv.invert		(E.R.O->XFORM());
	m_inv.transform_tiny(p_in_object_space, E.point);

	// bone-space
	IKinematics* V = smart_cast<IKinematics*>(E.R.O->Visual());

	if(V)
	{
		VERIFY3(V->LL_GetBoneVisible(u16(E.R.element)),*E.R.O->cNameVisual(),V->LL_BoneName_dbg(u16(E.R.element)));
		Fmatrix& m_bone = (V->LL_GetBoneInstance(u16(E.R.element))).mTransform;
		Fmatrix  m_inv_bone;
		m_inv_bone.invert(m_bone);
		m_inv_bone.transform_tiny(position_in_bone_space, p_in_object_space);
	}
	else
	{
		position_in_bone_space.set(p_in_object_space);
	}

	//отправить хит пораженному объекту
	if (E.bullet.flags.allow_sendhit)
	{
		SHit	Hit = SHit(	hit_param.main_damage,
							original_dir,
							NULL,
							u16(E.R.element),
							position_in_bone_space,
							hit_param.impulse,
							E.bullet.hit_type,
							hit_param.pierce_damage,
							hit_param.armor_pierce_damage);

		Hit.GenHeader(u16(GE_HIT)&0xffff, E.R.O->ID());
		Hit.whoID			= E.bullet.parent_id;
		Hit.weaponID		= E.bullet.weapon_id;
		Hit.BulletID		= E.bullet.m_dwID;

		NET_Packet			np;
		Hit.Write_Packet	(np);
		
//		Msg("Hit sended: %d[%d,%d]", Hit.whoID, Hit.weaponID, Hit.BulletID);
		CGameObject::u_EventSend(np);
	}
}

#ifdef DEBUG
FvectorVec g_hit[3];
#endif

extern void random_dir	(Fvector& tgt_dir, const Fvector& src_dir, float dispersion);

bool CBulletManager::ObjectHit(SBullet_Hit* hit_res, SBullet* bullet, const Fvector& end_point,
	collide::rq_result& R, u16 target_material, Fvector& hit_normal)
{
	//----------- normal - start
	if ( R.O )
	{
		//вернуть нормаль по которой играть партиклы
		CCF_Skeleton* skeleton = smart_cast<CCF_Skeleton*>(R.O->CFORM());
		if ( skeleton )
		{
			Fvector			e_center;
			hit_normal.set	(0,0,0);
			if ( skeleton->_ElementCenter( (u16)R.element,e_center ) )
				hit_normal.sub							(end_point, e_center);
			float len		= hit_normal.square_magnitude();
			if ( !fis_zero(len) )	hit_normal.div		(_sqrt(len));
			else				hit_normal.invert	(bullet->dir);
		}
	}
	else
	{
		//вычислить нормаль к поверхности
		Fvector*	pVerts	=  Level().ObjectSpace.GetStaticVerts();
		CDB::TRI*	pTri	= Level().ObjectSpace.GetStaticTris()+R.element;
		hit_normal.mknormal	(pVerts[pTri->verts[0]],pVerts[pTri->verts[1]],pVerts[pTri->verts[2]]);
	}		
	//----------- normal - end

	hit_res->impulse				= 0.f;
	hit_res->main_damage			= 0.f;
	hit_res->pierce_damage			= 0.f;
	hit_res->armor_pierce_damage	= 0.f;
	SGameMtl* mtl					= GMLib.GetMaterialByIdx(target_material);
	CEntityAlive* ea				= smart_cast<CEntityAlive*>(R.O);

	if (bullet->hit_param.main_damage != -1.f)
	{
		if (fLess(mtl->fShootFactor, 1.f) || ea)
		{
			hit_res->impulse		= bullet->hit_param.impulse;
			hit_res->main_damage	= bullet->hit_param.main_damage;
			bullet->speed			= 0.f;
			if (ea)
				bullet->targetID	= R.O->ID();
		}
		return						true;
	}

	float k_speed_in		= 0.f;
	float k_speed_bone		= 0.f;
	float k_speed_out		= 0.f;
	float pierce			= 0.f;
	bool ricoshet			= false;
	bool inwards			= DOT(hit_normal, bullet->dir) < 0.f;

	float bullet_energy					= bullet->mass * _sqr(bullet->speed) * .5f;
	float bullet_ap						= m_fBulletGlobalAPScale * bullet->k_ap * bullet_energy / bullet->resist;

	float armor							= 0.f;
	float bone_density					= 0.f;
	if (ea)
	{
		IKinematics* V					= smart_cast<IKinematics*>(ea->Visual());
		u16 element						= (u16)R.element;
		CBoneInstance& bone				= V->LL_GetBoneInstance(element);
		if (bone.get_param(2) != 1.f)
			bullet->targetID			= R.O->ID();
		armor							= ea->GetBoneArmor(element);
		bone_density					= bone.get_param(0);

		if (!bullet->parent_id || !R.O->ID())
			Msg("--xd CBulletManager::ObjectHit bullet_mass [%f] speed [%f] bullet_ap [%f] bone [%s] armor [%f] inwards [%d] bullet density [%f] bone_density [%f]",
				bullet->mass, bullet->speed, bullet_ap, V->LL_BoneName_dbg(element), armor, inwards, bullet->density, bone_density);		//--xd отладка
	}
	else
	{
		float d_density					= 2000.f * _sqr(mtl->fShootFactor);
		if (!inwards)
			d_density					*= -1.f;
		bullet->density					+= d_density;
		armor							= d_density * .25f;
		k_speed_out = k_speed_in		= 1.f;

		//if (!bullet->parent_id)
			//Msg("--xd CBulletManager::ObjectHit bullet_mass [%f] speed [%f] bullet_ap [%f] fShootFactor [%f] inwards [%d] bullet density [%f]",
				//bullet->mass, bullet->speed, bullet_ap, mtl->fShootFactor, inwards, bullet->density);
	}

	if (fMoreOrEqual(bullet_ap, armor))
	{
		if (ea)
		{
			k_speed_in					= sqrt(1.f - m_fBulletAPLossOnPierce * armor / bullet_ap);
			bullet_ap					-= m_fBulletAPLossOnPierce * armor;
			if (!bullet->flags.piercing_was)
			{
				if (bullet->hollow_point)
				{
					bullet->resist		*= m_fBulletHollowPointResistFactor;
					bullet_ap			/= m_fBulletHollowPointResistFactor;
				}
				bullet->flags.piercing_was = 1;
			}

			if (fMoreOrEqual(bullet_ap, bone_density))
			{
				k_speed_bone			= k_speed_in * sqrt(1.f - m_fBulletAPLossOnPierce * bone_density / bullet_ap);
				bullet_ap				-= m_fBulletAPLossOnPierce * bone_density;
				if (fMoreOrEqual(bullet_ap, armor))
					k_speed_out			= k_speed_bone * sqrt(1.f - m_fBulletAPLossOnPierce * armor / bullet_ap);
				pierce					= 1.f;
			}
			else
				pierce					= bullet_ap / bone_density;
		}
	}
	else if (fLessOrEqual(bullet_ap, armor * .5f) && !bullet->flags.magnetic_beam && !mtl->Flags.test(SGameMtl::flNoRicoshet) && bullet->flags.allow_ricochet)
	{
		//рикошет
		Fvector							new_dir;
		new_dir.reflect					(bullet->dir, hit_normal);
		Fvector							tgt_dir;
		random_dir						(tgt_dir, new_dir, deg2rad(10.f));
		float ricoshet_factor			= bullet->dir.dotproduct(tgt_dir);
		float f							= Random.randF(.5f, .8f); //(0.5f,1.f);
		if (f < ricoshet_factor)
		{
			// уменьшение скорости полета в зависимости от угла падения пули (чем прямее угол, тем больше потеря)
			float scale					= 1.f - _abs(bullet->dir.dotproduct(hit_normal)) * m_fCollisionEnergyMin;
			clamp						(scale, 0.f, m_fCollisionEnergyMax);
			k_speed_out = k_speed_in	= scale;

			// вычисление рикошета, делается немного фейком, т.к. пуля остается в точке столкновения
			// и сразу выходит из RayQuery()
			bullet->dir.set				(tgt_dir);
			bullet->pos					= end_point;
			bullet->flags.ricochet_was	= 1;
			ricoshet					= true;
			bullet->density				= 1.f;
		}
	}

	//ввести коэффициент случайности при простреливании
	if (!ricoshet && fMore(armor, 0.f))
	{
		Fvector							rand_normal;
		rand_normal.random_dir			(bullet->dir, deg2rad(2.f), Random);
		bullet->dir.set					(rand_normal);
	}

	if (fMore(pierce, 0.f))
	{
		float speed						= bullet->speed * k_speed_in;
		float resist_factor				= m_fBulletPierceDamageFromResist.Calc(bullet->resist);
		float kap_factor				= m_fBulletPierceDamageFromKAP.Calc(bullet->k_ap);
		float speed_factor				= m_fBulletPierceDamageFromSpeed.Calc(speed);

		float speed_scale_factor		= m_fBulletPierceDamageFromSpeedScale.Calc((k_speed_in + k_speed_bone) / 2.f);
		float hydroshock_factor			= m_fBulletPierceDamageFromHydroshock.Calc(speed);
		float stability_factor			= m_fBulletPierceDamageFromStability.Calc(bullet->mass);
		float pierce_factor				= m_fBulletPierceDamageFromPierce.Calc(pierce);

		float base_pierce_damage		= resist_factor * kap_factor * speed_factor;
		float flesh_pierce_damage		= speed_scale_factor * hydroshock_factor * stability_factor * pierce_factor;

		hit_res->pierce_damage			= m_fBulletPierceDamageScale * base_pierce_damage * flesh_pierce_damage;
		hit_res->armor_pierce_damage	= m_fBulletArmorPierceDamageScale * base_pierce_damage;
	}
	
	hit_res->impulse					= m_fBulletHitImpulseScale * bullet->mass * bullet->speed * (1.f - k_speed_out);
	hit_res->main_damage				= m_fBulletArmorDamageScale * sqrt(bullet->mass * bullet->speed * (1.f - k_speed_in));

	if (ea && (!bullet->parent_id || !R.O->ID()))
		Msg("--xd CBulletManager::ObjectHit main_damage [%.5f] k_speed_in [%.5f] k_speed_bone [%.5f] k_speed_out [%.5f] pierce [%.5f] ricoshet [%d]",
		hit_res->main_damage, k_speed_in, k_speed_bone, k_speed_out, pierce, ricoshet);

	bullet->speed						*= k_speed_out;

	return true;
}
