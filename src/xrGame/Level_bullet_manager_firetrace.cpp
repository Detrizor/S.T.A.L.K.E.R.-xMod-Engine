// Level_Bullet_Manager.cpp:	��� ����������� ������ ���� �� ����������
//								��� ���� � ������� ���������� ����
//								(��� �������� ������������ � �� ������������)
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

//���������� �� �������� �������� ���� �� ������� ���� ��� �� ������
extern float gCheckHitK;

//test callback ������� 
//  object - object for testing
//return TRUE-����������� ������ / FALSE-���������� ������
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
			// � ���� ������?
			if (actor||stalker){
				// ������ � ������ ��� ��������
				Fsphere S		= cform->getSphere();
				entity->XFORM().transform_tiny	(S.P)	;
				float dist		= rd.range;
				// �������� ������ �� �� � ����������� ����� 
				if (Fsphere::rpNone!=S.intersect_full(bullet->bullet_pos, bullet->dir, dist))
				{
					// �� ������, ������ ��� �������
					bool play_whine				= true;
					CObject* initiator			= Level().Objects.net_Find	(bullet->parent_id);
					if (actor){
						// ������ � ������
						float hpf				= 1.f;
						float ahp				= actor->HitProbability();
#if 1
#	if 0
						CObject					*weapon_object = Level().Objects.net_Find	(bullet->weapon_id);
						if (weapon_object) {
							CWeapon				*weapon = smart_cast<CWeapon*>(weapon_object);
							if (weapon) {
								float fly_dist		= bullet->fly_dist+dist;
								float dist_factor	= _min(1.f,fly_dist/Level().BulletManager().m_fHPMaxDist);
								ahp					= dist_factor*weapon->hit_probability() + (1.f-dist_factor)*1.f;
							}
						}
#	else
						float					game_difficulty_hit_probability = actor->HitProbability();
						CAI_Stalker				*stalker = smart_cast<CAI_Stalker*>(initiator);
						if (stalker)
							hpf					= stalker->SpecificCharacter().hit_probability_factor();

						float					dist_factor = 1.f;
						CObject					*weapon_object = Level().Objects.net_Find	(bullet->weapon_id);
						if (weapon_object) {
							CWeapon				*weapon = smart_cast<CWeapon*>(weapon_object);
							if (weapon) {
								game_difficulty_hit_probability = weapon->hit_probability();
								float fly_dist	= bullet->fly_dist+dist;
								dist_factor		= _min(1.f,fly_dist/Level().BulletManager().m_fHPMaxDist);
							}
						}

						ahp						= dist_factor*game_difficulty_hit_probability + (1.f-dist_factor)*1.f;
#	endif
#else
						CAI_Stalker* i_stalker	= smart_cast<CAI_Stalker*>(initiator);
						// ���� ������� �������, ��������� - hit_probability_factor �������a ����� - 1.0
						if (i_stalker) {
							hpf					= i_stalker->SpecificCharacter().hit_probability_factor();
							float fly_dist		= bullet->fly_dist+dist;
							float dist_factor	= _min(1.f,fly_dist/Level().BulletManager().m_fHPMaxDist);
							ahp					= dist_factor*actor->HitProbability() + (1.f-dist_factor)*1.f;
						}
#endif
						if (Random.randF(0.f,1.f)>(ahp*hpf)){ 
							bRes				= FALSE;	// don't hit actor
							play_whine			= true;		// play whine sound
						}else{
							// real test actor CFORM
							Level().BulletManager().m_rq_results.r_clear();

							if (cform->_RayQuery(rd,Level().BulletManager().m_rq_results)){
								bRes			= TRUE;		// hit actor
								play_whine		= false;	// don't play whine sound
							}else{
								bRes			= FALSE;	// don't hit actor
								play_whine		= true;		// play whine sound
							}
						}
					}
					// play whine sound
					if (play_whine){
						Fvector					pt;
						pt.mad					(bullet->bullet_pos, bullet->dir, dist);
						Level().BulletManager().PlayWhineSound				(bullet,initiator,pt);
					}
				}else{
					// don't test this object again (return FALSE)
					bRes		= FALSE;
				}

			}
		}
	}

	
	return bRes;
}

//callback ������� 
//	result.O;		// 0-static else CObject*
//	result.range;	// range from start to element 
//	result.element;	// if (O) "num tri" else "num bone"
//	params;			// user defined abstract data
//	Device.Statistic.TEST0.End();
//return TRUE-���������� ����������� / FALSE-��������� �����������

void CBulletManager::FireShotmark (SBullet* bullet, const Fvector& vDir, const Fvector &vEnd, collide::rq_result& R, u16 target_material, const Fvector& vNormal, bool ShowMark)
{
	SGameMtlPair* mtl_pair	= GMLib.GetMaterialPair(bullet->bullet_material_idx, target_material);
	Fvector particle_dir	= vNormal;

	if (R.O)
	{
		particle_dir		 = vDir;
		particle_dir.invert	();

		//�� ������� ������ ������� �� ������
		if(Level().CurrentEntity() && Level().CurrentEntity()->ID() == R.O->ID()) return;

		if (mtl_pair && !mtl_pair->m_pCollideMarks->empty() && ShowMark)
		{
			//�������� ������� �� ���������
			Fvector p;
			p.mad(bullet->bullet_pos,bullet->dir,R.range-0.01f);
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
		//��������� ������� � ���������� �����������
		Fvector*	pVerts	= Level().ObjectSpace.GetStaticVerts();
		CDB::TRI*	pTri	= Level().ObjectSpace.GetStaticTris()+R.element;

		if (mtl_pair && !mtl_pair->m_pCollideMarks->empty() && ShowMark)
		{
			//�������� ������� �� ���������
			::Render->add_StaticWallmark	(&*mtl_pair->m_pCollideMarks, vEnd, bullet->wallmark_size, pTri, pVerts);
		}
	}

	ref_sound* pSound = (!mtl_pair || mtl_pair->CollideSounds.empty())?
						NULL:&mtl_pair->CollideSounds[::Random.randI(0,mtl_pair->CollideSounds.size())];

	//��������� ����
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
			//�������� �������� ��������� � ��������
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
	//������ ��� ������������ ��������
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
	
	//���������� ����������� ��������� �� �������
//	Fvector			hit_normal;
	FireShotmark	(&E.bullet, E.bullet.dir, E.point, E.R, E.tgt_material, E.normal, NeedShootmark);
	
	Fvector original_dir = E.bullet.dir;
	//ObjectHit(&E.bullet, E.end_point, E.R, E.tgt_material, hit_normal);

	SBullet_Hit hit_param = E.hit_result;

	// object-space
	//��������� ���������� ���������
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

	//��������� ��� ����������� �������
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
							hit_param.pierce_damage_armor);

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

bool CBulletManager::ObjectHit( SBullet_Hit* hit_res, SBullet* bullet, const Fvector& end_point, 
							    collide::rq_result& R, u16 target_material, Fvector& hit_normal )
{
	//----------- normal - start
	if ( R.O )
	{
		//������� ������� �� ������� ������ ��������
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
		//��������� ������� � �����������
		Fvector*	pVerts	=  Level().ObjectSpace.GetStaticVerts();
		CDB::TRI*	pTri	= Level().ObjectSpace.GetStaticTris()+R.element;
		hit_normal.mknormal	(pVerts[pTri->verts[0]],pVerts[pTri->verts[1]],pVerts[pTri->verts[2]]);
		if ( bullet->density_mode )
		{
			Fvector new_pos;
			new_pos.mad(bullet->bullet_pos, bullet->dir, R.range);
			float l = bullet->begin_density.distance_to(new_pos);
			float shootFactor = l * bullet->density;
			bullet->speed -= shootFactor;
			if ( bullet->speed < 0 ) bullet->speed = 0;
		}
		if ( DOT( hit_normal, bullet->dir ) < 0 )
		{
			if ( bullet->density_mode )
			{
//				Log("WARNING: Material in material found while bullet tracing. Incorrect behaviour of shooting is possible.");
			}
			bullet->density_mode = true;
			SGameMtl* mtl = GMLib.GetMaterialByIdx(target_material);
			bullet->density = mtl->fDensityFactor;
			bullet->begin_density.mad( bullet->bullet_pos, bullet->dir,R.range );
		}
		else
		{
			bullet->density_mode=false;
		}
	}		
	//----------- normal - end

	hit_res->impulse = 0.f;
	hit_res->main_damage = 0.f;
	hit_res->pierce_damage = 0.f;

	//(���� = 0, �� ���� ���� ���������(���� ������� ��� �� �����������), ���� �������� � ������� 
	//�������, ���� ������ 0, �� ���� ��������� ������)

	float bullet_energy		= bullet->bullet_mass * pow(bullet->speed, 2) / 2;
	float bullet_resist		= bullet->bullet_resist;
	float ap				= m_fBulletArmorPiercingScale * (bullet_energy / bullet_resist);
	if (bullet->hollow_point)
		ap					*= m_fBulletHollowPointAPFactor;
	float energy_lost		= bullet_energy;
	float armor_factor		= 1.f, pierce_factor = 0.f, pierce = 0.f, max_density = 0.f;

	if (bullet->hit_param.main_damage != -1.f)
	{
		bullet_energy		= 1.f;
		ap					= bullet->hit_param.main_damage * 20.f;
	}

	CEntityAlive* EntityAlive = smart_cast<CEntityAlive*>(R.O);
	SGameMtl* mtl = GMLib.GetMaterialByIdx(target_material);
	if (EntityAlive)
	{
		IKinematics* V = smart_cast<IKinematics*>(EntityAlive->Visual());
		u16 element = (u16)R.element;
		CBoneInstance& bone = V->LL_GetBoneInstance(element);
		if (bone.get_param(2) != 1.f)
			bullet->targetID = R.O->ID();
		float bone_armor = EntityAlive->conditions().GetBoneArmor(element);

		if (ap >= bone_armor)
		{
			armor_factor = m_fBulletArmorPiercingLose * (bone_armor / ap);
			float armor_energy_lost = bullet_energy * armor_factor;
			float pierce_energy_lost = bullet_energy * (1 - armor_factor);
			float ap_pierce = ap * (1 - armor_factor);
			if (bullet->hollow_point)
			{
				bullet_resist *= m_fBulletHollowPointResistFactor;
				ap_pierce /= m_fBulletHollowPointResistFactor;
			}
			float bone_density = bone.get_param(0);
			for (u16 i = 0, i_e = V->LL_BoneCount(); i < i_e; i++)
			{
				float density = V->LL_GetBoneInstance(i).get_param(0);
				if (density > max_density)
					max_density = density;
			}
			pierce = bone_density / max_density;
			pierce_factor = 1.f - armor_factor;
			if (ap_pierce < bone_density)
				pierce *= ap_pierce / bone_density;
			else
			{
				pierce_factor *= m_fBulletArmorPiercingLose * (bone_density / ap_pierce);
				pierce_energy_lost = bullet_energy * pierce_factor;
			}
			float ap_armor2 = ap * (1 - armor_factor - pierce_factor);
			if (ap_armor2 >= bone_armor)
			{
				float armor2_factor = m_fBulletArmorPiercingLose * (bone_armor / ap);
				float armor2_energy_lost = bullet_energy * armor2_factor;
				energy_lost = armor_energy_lost + pierce_energy_lost + armor2_energy_lost;
			}
		}
	}
	else
	{
		float mtl_ap = mtl->fShootFactor;
		if (fsimilar(mtl_ap, 0.0f))	//���� �������� ��������� ��������������� (�����)
			return true;
		mtl_ap = 50.f * pow(mtl_ap / 0.5f, 2);
		if ((ap >= mtl_ap) && (!bullet->hollow_point))
		{
			armor_factor = m_fBulletArmorPiercingLose * mtl_ap / ap;
			energy_lost *= armor_factor;
		}
	}

	float energy_factor = (bullet_energy - energy_lost) / bullet_energy;
	float speed_scale = pow(energy_factor, 0.5f);

	if (!bullet->flags.magnetic_beam)
	{
		//�������
		Fvector			new_dir;
		new_dir.reflect(bullet->dir, hit_normal);
		Fvector			tgt_dir;
		random_dir(tgt_dir, new_dir, deg2rad(10.0f));
		float ricoshet_factor = bullet->dir.dotproduct(tgt_dir);

		float f = Random.randF(0.5f, 0.8f); //(0.5f,1.f);
		if ((f < ricoshet_factor) && !mtl->Flags.test(SGameMtl::flNoRicoshet) && bullet->flags.allow_ricochet)
		{
			// ���������� �������� ������ � ����������� �� ���� ������� ���� (��� ������ ����, ��� ������ ������)
			bullet->flags.allow_ricochet = 0;
			float scale = 1.0f - _abs(bullet->dir.dotproduct(hit_normal)) * m_fCollisionEnergyMin;
			clamp(scale, 0.0f, m_fCollisionEnergyMax);
			speed_scale = scale;

			// ���������� ��������, �������� ������� ������, �.�. ���� �������� � ����� ������������
			// � ����� ������� �� RayQuery()
			bullet->dir.set(tgt_dir);
			bullet->bullet_pos = end_point;
			bullet->flags.ricochet_was = 1;
		}
		else if (speed_scale < EPS)	//����������� ���� � ���������
			speed_scale = 0.0f;
		else
		{
			bullet->bullet_pos.mad(bullet->bullet_pos, bullet->dir, EPS);//fake
			//������ ����������� ����������� ��� ��������������
			Fvector rand_normal;
			rand_normal.random_dir(bullet->dir, deg2rad(2.0f), Random);
			bullet->dir.set(rand_normal);
		}
	}

	float given_pulse			= (1 - speed_scale) * bullet->speed * bullet->bullet_mass;
	hit_res->impulse			= (bullet->hit_param.impulse == -1.f)		? m_fBulletHitImpulseScale * given_pulse							: bullet->hit_param.impulse * (1 - speed_scale);
	hit_res->main_damage		= (bullet->hit_param.main_damage == -1.f)	? m_fBulletArmorDamageScale * (given_pulse * armor_factor)			: bullet->hit_param.main_damage * (1 - speed_scale) * armor_factor;
	
	if (pierce > 0.f)
	{
		float pierce_speed_1				= bullet->speed * pow(1 - armor_factor, 0.5f);
		float pierce_speed_2				= bullet->speed * pow(1 - armor_factor - pierce_factor, 0.5f);
		float pierce_speed					= (pierce_speed_1 + pierce_speed_2) / 2;
		float pierce_resist_damage			= pow(1.f + bullet_resist * m_fBulletPierceDamageResistFactor, m_fBulletPierceDamageResistPower);
		float pierce_speed_damage			= pow(1.f + pierce_speed * m_fBulletPierceDamageSpeedFactor, m_fBulletPierceDamageSpeedPower);
		float pierce_density_factor			= pow(max_density * m_fBulletPierceDamageDensityFactor, m_fBulletPierceDamageDensityPower);
		float pierce_damage					= pierce_resist_damage * pierce_speed_damage;
		hit_res->pierce_damage				= m_fBulletPierceDamageScale * pierce_damage * sqrt(pierce) * pierce_density_factor;
		hit_res->pierce_damage_armor		= m_fBulletPierceDamageArmorScale * pierce_damage;
	}
	bullet->speed							*= speed_scale;

	return true;
}