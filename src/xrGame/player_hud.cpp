#include "stdafx.h"
#include "player_hud.h"
#include "HudItem.h"
#include "ui_base.h"
#include "actor.h"
#include "physic_item.h"
#include "static_cast_checked.hpp"
#include "actoreffector.h"
#include "../xrEngine/IGame_Persistent.h"
#include "WeaponMagazined.h"
#include "weapon_hud.h"

#define TENDTO_SPEED         1.0f     // Модификатор силы инерции (больше - чувствительней)
#define TENDTO_SPEED_AIM     1.0f     // (Для прицеливания)
#define TENDTO_SPEED_RET     5.0f     // Модификатор силы отката инерции (больше - быстрее)
#define TENDTO_SPEED_RET_AIM 5.0f     // (Для прицеливания)
#define INERT_MIN_ANGLE      0.0f     // Минимальная сила наклона, необходимая для старта инерции
#define INERT_MIN_ANGLE_AIM  3.5f     // (Для прицеливания)

// Пределы смещения при инерции (лево / право / верх / низ)
#define ORIGIN_OFFSET        0.04f,  0.04f,  0.04f, 0.02f
#define ORIGIN_OFFSET_AIM    0.015f, 0.015f, 0.01f, 0.005f

player_hud* g_player_hud = NULL;

float CalcMotionSpeed(const shared_str& anim_name)
{
	return 1.0f;
}

player_hud_motion* player_hud_motion_container::find_motion(const shared_str& name)
{
	xr_vector<player_hud_motion>::iterator it	= m_anims.begin();
	xr_vector<player_hud_motion>::iterator it_e = m_anims.end();
	for(;it!=it_e;++it)
	{
		const shared_str& s = (true)?(*it).m_alias_name:(*it).m_base_name;
		if( s == name)
			return &(*it);
	}
	return NULL;
}

void player_hud_motion_container::load(IKinematicsAnimated* model, LPCSTR sect)
{
	CInifile::Sect& _sect		= pSettings->r_section(sect);
	CInifile::SectCIt _b		= _sect.Data.begin();
	CInifile::SectCIt _e		= _sect.Data.end();
	player_hud_motion* pm		= NULL;
	
	string512					buff;
	MotionID					motion_ID;

	for(;_b!=_e;++_b)
	{
		if(strstr(_b->first.c_str(), "anm_")==_b->first.c_str())
		{
			const shared_str& anm	= _b->second;
			m_anims.resize			(m_anims.size()+1);
			pm						= &m_anims.back();
			//base and alias name
			pm->m_alias_name		= _b->first;
			
			if(_GetItemCount(anm.c_str())==1)
			{
				pm->m_base_name			= anm;
				pm->m_additional_name	= anm;
				pm->m_anim_speed = 1.f;
			}else
			{
				R_ASSERT2(_GetItemCount(anm.c_str())<=3, anm.c_str());
				string512				str_item;
				_GetItem(anm.c_str(),0,str_item);
				pm->m_base_name			= str_item;

				_GetItem(anm.c_str(),1,str_item);
				pm->m_additional_name = (strlen(str_item) > 0) ? pm->m_additional_name = str_item : pm->m_base_name;
				
				_GetItem(anm.c_str(), 2, str_item);
				pm->m_anim_speed = strlen(str_item) > 0	 ? atof(str_item) : 1.f;
			}

			//and load all motions for it

			for(u32 i=0; i<=8; ++i)
			{
				if(i==0)
					xr_strcpy				(buff,pm->m_base_name.c_str());		
				else
					xr_sprintf				(buff,"%s%d",pm->m_base_name.c_str(),i);		

				motion_ID				= model->ID_Cycle_Safe(buff);
				if(motion_ID.valid())
				{
					pm->m_animations.resize			(pm->m_animations.size()+1);
					pm->m_animations.back().mid		= motion_ID;
					pm->m_animations.back().name	= buff;
#ifdef DEBUG
//					Msg(" alias=[%s] base=[%s] name=[%s]",pm->m_alias_name.c_str(), pm->m_base_name.c_str(), buff);
#endif // #ifdef DEBUG
				}
			}
			R_ASSERT2(pm->m_animations.size(),make_string("motion not found [%s]", pm->m_base_name.c_str()).c_str());
		}
	}
}

void attachable_hud_item::set_bone_visible(const shared_str& bone_name, BOOL bVisibility, BOOL bSilent)
{
	u16  bone_id;
	BOOL bVisibleNow;
	bone_id			= m_model->LL_BoneID			(bone_name);
	if(bone_id==BI_NONE)
	{
		if (bSilent)
			return;
		R_ASSERT2(0, make_string("model [%s] has no bone [%s]", pSettings->r_string(m_object_section, "visual"), bone_name.c_str()).c_str());
	}
	bVisibleNow		= m_model->LL_GetBoneVisible	(bone_id);
	if(bVisibleNow!=bVisibility)
		m_model->LL_SetBoneVisible	(bone_id,bVisibility, TRUE);
}

void attachable_hud_item::update(bool bForce)
{
	if (!bForce && m_upd_firedeps_frame == Device.dwFrame) return;

	Fvector ypr						= m_measures.m_item_attach[1];
	ypr.mul							(PI/180.f);
	m_attach_offset.setHPB			(ypr.x,ypr.y,ypr.z);
	m_attach_offset.translate_over	(m_measures.m_item_attach[0]);

	m_parent->calc_transform		(m_attach_place_idx, m_attach_offset, m_item_transform);

	m_parent_hud_item->UpdateSlotsTransform();

	m_upd_firedeps_frame			= Device.dwFrame;

	IKinematicsAnimated* ka			=	m_model->dcast_PKinematicsAnimated();
	if(ka)
	{
		ka->UpdateTracks									();
		ka->dcast_PKinematics()->CalculateBones_Invalidate	();
		ka->dcast_PKinematics()->CalculateBones				(TRUE);
	}
}

void attachable_hud_item::update_hud_additional(Fmatrix& trans)
{
	if (m_parent_hud_item)
		m_parent_hud_item->UpdateHudAdditional(trans);
}

void attachable_hud_item::setup_firedeps(firedeps& fd)
{
	update							(false);
	// fire point&direction
	if(m_measures.m_prop_flags.test(hud_item_measures::e_fire_point))
	{
		Fmatrix& fire_mat								= m_model->LL_GetTransform(m_measures.m_fire_bone);
		fire_mat.transform_tiny							(fd.vLastFP, m_measures.m_fire_point_offset);
		m_item_transform.transform_tiny					(fd.vLastFP);

		fd.vLastFD.set									(0.f,0.f,1.f);
		m_item_transform.transform_dir					(fd.vLastFD);
		VERIFY(_valid(fd.vLastFD));
		VERIFY(_valid(fd.vLastFD));

		fd.m_FireParticlesXForm.identity				();
		fd.m_FireParticlesXForm.k.set					(fd.vLastFD);
		Fvector::generate_orthonormal_basis_normalized	(	fd.m_FireParticlesXForm.k,
															fd.m_FireParticlesXForm.j, 
															fd.m_FireParticlesXForm.i);
		VERIFY(_valid(fd.m_FireParticlesXForm));
	}

	if(m_measures.m_prop_flags.test(hud_item_measures::e_fire_point2))
	{
		Fmatrix& fire_mat			= m_model->LL_GetTransform(m_measures.m_fire_bone2);
		fire_mat.transform_tiny		(fd.vLastFP2,m_measures.m_fire_point2_offset);
		m_item_transform.transform_tiny	(fd.vLastFP2);
		VERIFY(_valid(fd.vLastFP2));
		VERIFY(_valid(fd.vLastFP2));
	}

	if(m_measures.m_prop_flags.test(hud_item_measures::e_shell_point))
	{
		Fmatrix& fire_mat			= m_model->LL_GetTransform(m_measures.m_shell_bone);
		fire_mat.transform_tiny		(fd.vLastSP,m_measures.m_shell_point_offset);
		m_item_transform.transform_tiny	(fd.vLastSP);
		VERIFY(_valid(fd.vLastSP));
		VERIFY(_valid(fd.vLastSP));
	}
}

bool  attachable_hud_item::need_renderable()
{
	return m_parent_hud_item->need_renderable();
}

void attachable_hud_item::render()
{
	::Render->set_Transform		(&m_item_transform);
	::Render->add_Visual		(m_model->dcast_RenderVisual());
	debug_draw_firedeps			();
	m_parent_hud_item->render_hud_mode();
}

bool attachable_hud_item::render_item_ui_query()
{
	return m_parent_hud_item->render_item_3d_ui_query();
}

void attachable_hud_item::render_item_ui()
{
	m_parent_hud_item->render_item_3d_ui();
}

void hud_item_measures::load(LPCSTR hud_section, IKinematics* K)
{
	R_ASSERT2(pSettings->line_exist(hud_section, "fire_point") == pSettings->line_exist(hud_section, "fire_bone"), hud_section);
	R_ASSERT2(pSettings->line_exist(hud_section, "fire_point2") == pSettings->line_exist(hud_section, "fire_bone2"), hud_section);
	R_ASSERT2(pSettings->line_exist(hud_section, "shell_point") == pSettings->line_exist(hud_section, "shell_bone"), hud_section);

	m_hands_attach[0] = pSettings->r_fvector3(hud_section, "hands_position");
	m_hands_attach[1] = pSettings->r_fvector3(hud_section, "hands_orientation");

	m_item_attach[0] = pSettings->r_fvector3(hud_section, "item_position");
	m_item_attach[1] = pSettings->r_fvector3(hud_section, "item_orientation");

	shared_str bone_name;
	m_prop_flags.set(e_fire_point, pSettings->line_exist(hud_section, "fire_bone"));
	if (m_prop_flags.test(e_fire_point))
	{
		bone_name = pSettings->r_string(hud_section, "fire_bone");
		m_fire_bone = K->LL_BoneID(bone_name);
		m_fire_point_offset = pSettings->r_fvector3(hud_section, "fire_point");
		//--xd m_fire_direction = READ_IF_EXISTS(pSettings, r_fvector3, sect_name, "fire_direction", Fvector().set(0.f, 0.f, 1.0f));
	}
	else
		m_fire_point_offset.set(0, 0, 0);

	m_prop_flags.set(e_fire_point2, pSettings->line_exist(hud_section, "fire_bone2"));
	if (m_prop_flags.test(e_fire_point2))
	{
		bone_name = pSettings->r_string(hud_section, "fire_bone2");
		m_fire_bone2 = K->LL_BoneID(bone_name);
		m_fire_point2_offset = pSettings->r_fvector3(hud_section, "fire_point2");
	}
	else
		m_fire_point2_offset.set(0, 0, 0);

	m_prop_flags.set(e_shell_point, pSettings->line_exist(hud_section, "shell_bone"));
	if (m_prop_flags.test(e_shell_point))
	{
		bone_name = pSettings->r_string(hud_section, "shell_bone");
		m_shell_bone = K->LL_BoneID(bone_name);
		m_shell_point_offset = pSettings->r_fvector3(hud_section, "shell_point");
	}
	else
		m_shell_point_offset.set(0, 0, 0);

	// Настройки стрейфа (боковая ходьба)
	Fvector vDefStrafeValue;

	//--> Смещение в стрейфе
	m_strafe_offset[0][0] = READ_IF_EXISTS(pSettings, r_fvector3, hud_section, "strafe_hud_offset_pos", vDefStrafeValue);
	m_strafe_offset[1][0] = READ_IF_EXISTS(pSettings, r_fvector3, hud_section, "strafe_hud_offset_rot", vDefStrafeValue);

	//--> Поворот в стрейфе
	m_strafe_offset[0][1] = READ_IF_EXISTS(pSettings, r_fvector3, hud_section, "strafe_aim_hud_offset_pos", vDefStrafeValue);
	m_strafe_offset[1][1] = READ_IF_EXISTS(pSettings, r_fvector3, hud_section, "strafe_aim_hud_offset_rot", vDefStrafeValue);

	//--> Параметры стрейфа
	bool bStrafeEnabled = READ_IF_EXISTS(pSettings, r_bool, hud_section, "strafe_enabled", false);
	bool bStrafeEnabled_aim = READ_IF_EXISTS(pSettings, r_bool, hud_section, "strafe_aim_enabled", false);
	float fFullStrafeTime = READ_IF_EXISTS(pSettings, r_float, hud_section, "strafe_transition_time", .5f);
	float fFullStrafeTime_aim = READ_IF_EXISTS(pSettings, r_float, hud_section, "strafe_aim_transition_time", .5f);
	float fStrafeCamLFactor = READ_IF_EXISTS(pSettings, r_float, hud_section, "strafe_cam_limit_factor", 0.5f);
	float fStrafeCamLFactor_aim = READ_IF_EXISTS(pSettings, r_float, hud_section, "strafe_cam_limit_aim_factor", 1.0f);
	float fStrafeMinAngle = READ_IF_EXISTS(pSettings, r_float, hud_section, "strafe_cam_min_angle", 0.0f);
	float fStrafeMinAngle_aim = READ_IF_EXISTS(pSettings, r_float, hud_section, "strafe_cam_aim_min_angle", 7.0f);

	//--> (Data 1)
	m_strafe_offset[2][0].set((bStrafeEnabled ? 1.0f : 0.0f), fFullStrafeTime, NULL); // normal
	m_strafe_offset[2][1].set((bStrafeEnabled_aim ? 1.0f : 0.0f), fFullStrafeTime_aim, NULL); // aim-GL

	//--> (Data 2)
	m_strafe_offset[3][0].set(fStrafeCamLFactor, fStrafeMinAngle, NULL); // normal
	m_strafe_offset[3][1].set(fStrafeCamLFactor_aim, fStrafeMinAngle_aim, NULL); // aim-GL

	// Загрузка параметров смещения / инерции
	m_inertion_params.m_tendto_speed = READ_IF_EXISTS(pSettings, r_float, hud_section, "inertion_tendto_speed", TENDTO_SPEED);
	m_inertion_params.m_tendto_speed_aim = READ_IF_EXISTS(pSettings, r_float, hud_section, "inertion_tendto_aim_speed", TENDTO_SPEED_AIM);
	m_inertion_params.m_tendto_ret_speed = READ_IF_EXISTS(pSettings, r_float, hud_section, "inertion_tendto_ret_speed", TENDTO_SPEED_RET);
	m_inertion_params.m_tendto_ret_speed_aim = READ_IF_EXISTS(pSettings, r_float, hud_section, "inertion_tendto_ret_aim_speed", TENDTO_SPEED_RET_AIM);

	m_inertion_params.m_min_angle = READ_IF_EXISTS(pSettings, r_float, hud_section, "inertion_min_angle", INERT_MIN_ANGLE);
	m_inertion_params.m_min_angle_aim = READ_IF_EXISTS(pSettings, r_float, hud_section, "inertion_min_angle_aim", INERT_MIN_ANGLE_AIM);

	m_inertion_params.m_offset_LRUD = READ_IF_EXISTS(pSettings, r_fvector4, hud_section, "inertion_offset_LRUD", Fvector4().set(ORIGIN_OFFSET));
	m_inertion_params.m_offset_LRUD_aim = READ_IF_EXISTS(pSettings, r_fvector4, hud_section, "inertion_offset_LRUD_aim", Fvector4().set(ORIGIN_OFFSET_AIM));
}

attachable_hud_item::~attachable_hud_item()
{
	IRenderVisual* v			= m_model->dcast_RenderVisual();
	::Render->model_Delete		(v);
	m_model						= NULL;
}

void attachable_hud_item::load(LPCSTR hud_section, LPCSTR object_section)
{
	m_hud_section				= hud_section;
	m_object_section			= object_section;

	// Visual
	LPCSTR visual_name			= READ_IF_EXISTS(pSettings, r_string, hud_section, "item_visual", pSettings->r_string(object_section, "visual"));
	m_model						= smart_cast<IKinematics*>(::Render->model_Create(visual_name));

	m_attach_place_idx			= pSettings->r_u16(hud_section, "attach_place_idx");
	m_measures.load				(hud_section, m_model);
}

u32 attachable_hud_item::anim_play(const shared_str& anm_name_b, BOOL bMixIn, const CMotionDef*& md, u8& rnd_idx)
{
	R_ASSERT				(strstr(anm_name_b.c_str(),"anm_")==anm_name_b.c_str());
	string256				anim_name_r;
	bool is_16x9			= UI().is_widescreen();
	xr_sprintf				(anim_name_r,"%s%s",anm_name_b.c_str(),((m_attach_place_idx==1)&&is_16x9)?"_16x9":"");

	player_hud_motion* anm	= m_hand_motions.find_motion(anim_name_r);
	R_ASSERT2				(anm, make_string("model [%s] has no motion alias defined [%s]", m_hud_section.c_str(), anim_name_r).c_str());
	R_ASSERT2				(anm->m_animations.size(), make_string("model [%s] has no motion defined in motion_alias [%s]", pSettings->r_string(m_object_section, "visual"), anim_name_r).c_str());
	
	float speed = anm->m_anim_speed;

	rnd_idx					= (u8)Random.randI(anm->m_animations.size()) ;
	const motion_descr& M	= anm->m_animations[ rnd_idx ];

	u32 ret					= g_player_hud->anim_play(m_attach_place_idx, M.mid, bMixIn, md, speed);
	
	if(m_model->dcast_PKinematicsAnimated())
	{
		IKinematicsAnimated* ka			= m_model->dcast_PKinematicsAnimated();

		shared_str item_anm_name;
		if(anm->m_base_name!=anm->m_additional_name)
			item_anm_name = anm->m_additional_name;
		else
			item_anm_name = M.name;

		MotionID M2						= ka->ID_Cycle_Safe(item_anm_name);
		if(!M2.valid())
			M2							= ka->ID_Cycle_Safe("idle");
		else
			if(bDebug)
				Msg						("playing item animation [%s]",item_anm_name.c_str());

		R_ASSERT3(M2.valid(), "model has no motion [idle] ", pSettings->r_string(m_object_section, "visual"));

		u16 root_id						= m_model->LL_GetBoneRoot();
		CBoneInstance& root_binst		= m_model->LL_GetBoneInstance(root_id);
		root_binst.set_callback_overwrite(TRUE);
		root_binst.mTransform.identity	();

		u16 pc							= ka->partitions().count();
		for(u16 pid=0; pid<pc; ++pid)
		{
			CBlend* B					= ka->PlayCycle(pid, M2, bMixIn);
			R_ASSERT					(B);
			B->speed					*= speed;
		}

		m_model->CalculateBones_Invalidate	();
	}

	R_ASSERT2		(m_parent_hud_item, "parent hud item is NULL");
	CPhysicItem&	parent_object = m_parent_hud_item->object();
	//R_ASSERT2		(parent_object, "object has no parent actor");
	//CObject*		parent_object = static_cast_checked<CObject*>(&m_parent_hud_item->object());

	if (parent_object.H_Parent() == Level().CurrentControlEntity())
	{
		CActor* current_actor	= static_cast_checked<CActor*>(Level().CurrentControlEntity());
		VERIFY					(current_actor);
		CEffectorCam* ec		= current_actor->Cameras().GetCamEffector(eCEWeaponAction);

		if (ec)
			current_actor->Cameras().RemoveCamEffector(eCEWeaponAction);
	
		string_path			ce_path;
		string_path			anm_name;
		strconcat			(sizeof(anm_name),anm_name,"camera_effects\\weapon\\", M.name.c_str(),".anm");
		if (FS.exist( ce_path, "$game_anims$", anm_name))
		{
			CAnimatorCamEffector* e		= xr_new<CAnimatorCamEffector>();
			e->SetType					(eCEWeaponAction);
			e->SetHudAffect				(false);
			e->SetCyclic				(false);
			e->Start					(anm_name);
			current_actor->Cameras().AddCamEffector(e);
		}
	}
	return ret;
}

player_hud::player_hud()
{
	m_model					= NULL;
	m_attached_items[0]		= NULL;
	m_attached_items[1]		= NULL;
	m_transform.identity();
	m_attach_offset.identity();

	//Movement Layers
	m_movement_layers.reserve(move_anms_end);

	for (int i = 0; i < move_anms_end; i++)
	{
		movement_layer* anm = xr_new<movement_layer>();

		char temp[20];
		string512 tmp;
		strconcat(sizeof(temp), temp, "movement_layer_", std::to_string(i).c_str());
		R_ASSERT2(pSettings->line_exist("hud_movement_layers", temp), make_string("Missing definition for [hud_movement_layers] %s", temp));
		LPCSTR layer_def = pSettings->r_string("hud_movement_layers", temp);
		R_ASSERT2(_GetItemCount(layer_def) > 0, make_string("Wrong definition for [hud_movement_layers] %s", temp));

		_GetItem(layer_def, 0, tmp);
		anm->Load(tmp);
		_GetItem(layer_def, 1, tmp);
		anm->anm->Speed() = (atof(tmp) ? atof(tmp) : 1.f);
		_GetItem(layer_def, 2, tmp);
		anm->m_power = (atof(tmp) ? atof(tmp) : 1.f);
		m_movement_layers.push_back(anm);
	}
}

player_hud::~player_hud()
{
	IRenderVisual* v			= m_model->dcast_RenderVisual();
	::Render->model_Delete		(v);
	m_model						= NULL;

	xr_vector<attachable_hud_item*>::iterator it	= m_pool.begin();
	xr_vector<attachable_hud_item*>::iterator it_e	= m_pool.end();
	for(;it!=it_e;++it)
	{
		attachable_hud_item* a	= *it;
		xr_delete				(a);
	}
	m_pool.clear				();

	delete_data(m_script_layers);
	delete_data(m_movement_layers);
}

void player_hud::load(const shared_str& player_hud_sect)
{
	if(player_hud_sect ==m_sect_name)	return;
	bool b_reload = (m_model!=NULL);
	if(m_model)
	{
		IRenderVisual* v			= m_model->dcast_RenderVisual();
		::Render->model_Delete		(v);
	}

	m_sect_name					= player_hud_sect;
	const shared_str& model_name= pSettings->r_string(player_hud_sect, "visual");
	m_model						= smart_cast<IKinematicsAnimated*>(::Render->model_Create(model_name.c_str()));

	CInifile::Sect& _sect		= pSettings->r_section(player_hud_sect);
	CInifile::SectCIt _b		= _sect.Data.begin();
	CInifile::SectCIt _e		= _sect.Data.end();
	for(;_b!=_e;++_b)
	{
		if(strstr(_b->first.c_str(), "ancor_")==_b->first.c_str())
		{
			const shared_str& _bone	= _b->second;
			m_ancors.push_back		(m_model->dcast_PKinematics()->LL_BoneID(_bone));
		}
	}
	
//	Msg("hands visual changed to[%s] [%s] [%s]", model_name.c_str(), b_reload?"R":"", m_attached_items[0]?"Y":"");

	if(!b_reload)
	{
		m_model->PlayCycle("hand_idle_doun");
	}else
	{
		if(m_attached_items[1])
			m_attached_items[1]->m_parent_hud_item->on_a_hud_attach();

		if(m_attached_items[0])
			m_attached_items[0]->m_parent_hud_item->on_a_hud_attach();
	}
	m_model->dcast_PKinematics()->CalculateBones_Invalidate	();
	m_model->dcast_PKinematics()->CalculateBones(TRUE);
}

bool player_hud::render_item_ui_query()
{
	bool res = false;
	if(m_attached_items[0])
		res |= m_attached_items[0]->render_item_ui_query();

	if(m_attached_items[1])
		res |= m_attached_items[1]->render_item_ui_query();

	return res;
}

void player_hud::render_item_ui()
{
	if(m_attached_items[0])
		m_attached_items[0]->render_item_ui();

	if(m_attached_items[1])
		m_attached_items[1]->render_item_ui();
}

void player_hud::render_hud()
{
	if(!m_attached_items[0] && !m_attached_items[1])	return;

	bool b_r0 = (m_attached_items[0] && m_attached_items[0]->need_renderable());
	bool b_r1 = (m_attached_items[1] && m_attached_items[1]->need_renderable());

	if(!b_r0 && !b_r1)									return;

	::Render->set_Transform		(&m_transform);
	::Render->add_Visual		(m_model->dcast_RenderVisual());
	
	if(m_attached_items[0])
		m_attached_items[0]->render();
	
	if(m_attached_items[1])
		m_attached_items[1]->render();
}


#include "../xrEngine/motion.h"

u32 player_hud::motion_length(const shared_str& anim_name, const shared_str& hud_name, const shared_str& section, const CMotionDef*& md)
{
	float speed						= CalcMotionSpeed(anim_name);
	attachable_hud_item* pi			= create_hud_item(*hud_name, *section);
	player_hud_motion*	pm			= pi->m_hand_motions.find_motion(anim_name);
	if(!pm)
		return						100; // ms TEMPORARY
	R_ASSERT2						(pm, 
		make_string	("hudItem model [%s] has no motion with alias [%s]", hud_name.c_str(), anim_name.c_str() ).c_str() 
		);
	return motion_length			(pm->m_animations[0].mid, md, speed);
}

u32 player_hud::motion_length(const MotionID& M, const CMotionDef*& md, float speed)
{
	md					= m_model->LL_GetMotionDef(M);
	VERIFY				(md);
	if (md->flags & esmStopAtEnd) 
	{
		CMotion*			motion		= m_model->LL_GetRootMotion(M);
		return				iFloor( 0.5f + 1000.f*motion->GetLength() / (md->Dequantize(md->speed) * speed) );
	}
	return					0;
}
const Fvector& player_hud::attach_rot() const
{
	if(m_attached_items[0])
		return m_attached_items[0]->hands_attach_rot();
	else
	if(m_attached_items[1])
		return m_attached_items[1]->hands_attach_rot();
	else
		return Fvector().set(0,0,0);
}

const Fvector& player_hud::attach_pos() const
{
	if(m_attached_items[0])
		return m_attached_items[0]->hands_attach_pos();
	else
	if(m_attached_items[1])
		return m_attached_items[1]->hands_attach_pos();
	else
		return Fvector().set(0,0,0);
}

void player_hud::update(const Fmatrix& cam_trans)
{
	Fmatrix trans = cam_trans;
	Fmatrix trans_b = cam_trans;

	Fvector m1pos = attach_pos();
	Fvector m1rot = attach_rot();

	Fmatrix trans_2 = trans;

	if (m_attached_items[0])
		m_attached_items[0]->m_parent_hud_item->UpdateHudAdditional(trans);
		
	if (m_attached_items[1])
	{
		m_attached_items[1]->m_parent_hud_item->UpdateHudAdditional(trans_2);

		CWeaponMagazined CPC wep = smart_cast<CWeaponMagazined CP$>(Actor()->inventory().ActiveItem());
		if (wep && wep->IsZoomed())
			trans_2.mulB_43(wep->Hud().shoot_shake_mat());
	}
	else
		trans_2 = trans;

	m1rot.mul(PI / 180.f);
	m_attach_offset.setHPB(m1rot.x, m1rot.y, m1rot.z);
	m_attach_offset.translate_over(m1pos);

	m_transform.mul(trans, m_attach_offset);
	
	m_model->UpdateTracks();
	m_model->dcast_PKinematics()->CalculateBones_Invalidate();
	m_model->dcast_PKinematics()->CalculateBones(TRUE);

	for (script_layer* anm : m_script_layers)
	{
		if (!anm || !anm->anm || (!anm->active && anm->blend_amount == 0.f))
			continue;

		if (anm->active)
			anm->blend_amount += Device.fTimeDelta / .4f;
		else
			anm->blend_amount -= Device.fTimeDelta / .4f;

		clamp(anm->blend_amount, 0.f, 1.f);

		if (anm->blend_amount > 0.f)
		{
			if (anm->anm->bLoop || anm->anm->anim_param().t_current < anm->anm->anim_param().max_t)
				anm->anm->Update(Device.fTimeDelta);
			else
				anm->Stop(false);
		}
		else
		{
			anm->Stop(true);
			continue;
		}

		Fmatrix blend = anm->XFORM();

		if (anm->m_part == 0 || anm->m_part == 2)
			m_transform.mulB_43(blend);
	}

	bool need_blend[2];
	need_blend[0] = (m_attached_items[0] && m_attached_items[0]->m_parent_hud_item->NeedBlendAnm());
	need_blend[1] = (m_attached_items[1] && m_attached_items[1]->m_parent_hud_item->NeedBlendAnm());

	for (movement_layer* anm : m_movement_layers)
	{
		if (!anm || !anm->anm || (!anm->active && anm->blend_amount[0] == 0.f && anm->blend_amount[1] == 0.f))
			continue;

		if (anm->active && (need_blend[0] || need_blend[1]))
		{
			if (need_blend[0])
			{
				anm->blend_amount[0] += Device.fTimeDelta / .4f;

				if (!m_attached_items[1])
					anm->blend_amount[1] += Device.fTimeDelta / .4f;
				else if (!need_blend[1])
					anm->blend_amount[1] -= Device.fTimeDelta / .4f;
			}

			if (need_blend[1])
			{
				anm->blend_amount[1] += Device.fTimeDelta / .4f;

				if (!m_attached_items[0])
					anm->blend_amount[0] += Device.fTimeDelta / .4f;
				else if (!need_blend[0])
					anm->blend_amount[0] -= Device.fTimeDelta / .4f;
			}
		}
		else
		{
			anm->blend_amount[0] -= Device.fTimeDelta / .4f;
			anm->blend_amount[1] -= Device.fTimeDelta / .4f;
		}

		clamp(anm->blend_amount[0], 0.f, 1.f);
		clamp(anm->blend_amount[1], 0.f, 1.f);

		if (anm->blend_amount[0] == 0.f && anm->blend_amount[1] == 0.f)
		{
			anm->Stop(true);
			continue;
		}

		anm->anm->Update(Device.fTimeDelta);

		if (anm->blend_amount[0] == anm->blend_amount[1])
		{
			Fmatrix blend = anm->XFORM(0);
			m_transform.mulB_43(blend);
		}
		else
		{
			if (anm->blend_amount[0] > 0.f)
				m_transform.mulB_43(anm->XFORM(0));
		}
	}

	if (m_attached_items[0])
		m_attached_items[0]->update(true);

	if (m_attached_items[1])
		m_attached_items[1]->update(true);
}

u32 player_hud::anim_play(u16 part, const MotionID& M, BOOL bMixIn, const CMotionDef*& md, float speed)
{

	u16 part_id							= u16(-1);
	if(attached_item(0) && attached_item(1))
		part_id = m_model->partitions().part_id((part==0)?"right_hand":"left_hand");

	u16 pc					= m_model->partitions().count();
	for(u16 pid=0; pid<pc; ++pid)
	{
		if(pid==0 || pid==part_id || part_id==u16(-1))
		{
			CBlend* B	= m_model->PlayCycle(pid, M, bMixIn);
			R_ASSERT	(B);
			B->speed	*= speed;
		}
	}
	m_model->dcast_PKinematics()->CalculateBones_Invalidate	();

	return				motion_length(M, md, speed);
}

void player_hud::update_additional	(Fmatrix& trans)
{
	if(m_attached_items[0])
		m_attached_items[0]->update_hud_additional(trans);

	if(m_attached_items[1])
		m_attached_items[1]->update_hud_additional(trans);
}

attachable_hud_item* player_hud::create_hud_item(LPCSTR hud_section, LPCSTR object_section)
{
	for (auto itm : m_pool)
	{
		if (itm->m_hud_section == hud_section && itm->m_object_section == object_section)
			return itm;
	}
	attachable_hud_item* res	= xr_new<attachable_hud_item>(this);
	res->load					(hud_section, object_section);
	res->m_hand_motions.load	(m_model, hud_section);
	m_pool.push_back			(res);

	return	res;
}

bool player_hud::allow_activation(CHudItem* item)
{
	if(m_attached_items[1])
		return m_attached_items[1]->m_parent_hud_item->CheckCompatibility(item);
	else
		return true;
}

void player_hud::attach_item(CHudItem* item)
{
	attachable_hud_item* pi			= create_hud_item(*item->HudSection(), *item->object().cNameSect());
	int item_idx					= pi->m_attach_place_idx;
	
	if (m_attached_items[item_idx] != pi)
	{
		if (m_attached_items[item_idx])
			m_attached_items[item_idx]->m_parent_hud_item->on_b_hud_detach();

		m_attached_items[item_idx]						= pi;
		pi->m_parent_hud_item							= item;

		if(item_idx==0 && m_attached_items[1])
			m_attached_items[1]->m_parent_hud_item->CheckCompatibility(item);

		item->on_a_hud_attach();

		updateMovementLayerState();
		item->UpdateHudBonesVisibility();
	}
	pi->m_parent_hud_item							= item;
}

void player_hud::detach_item_idx(u16 idx)
{
	if( NULL==attached_item(idx) )					return;

	m_attached_items[idx]->m_parent_hud_item->on_b_hud_detach();

	m_attached_items[idx]->m_parent_hud_item		= NULL;

	for (xr_vector<attachable_hud_item*>::iterator it = m_pool.begin(), it_e = m_pool.end(); it != it_e; ++it)
	{
		attachable_hud_item* itm = *it;
		if (itm == m_attached_items[idx])
		{
			m_pool.erase(it);
			break;
		}
	}

	m_attached_items[idx]							= NULL;

	if(idx==1 && attached_item(0))
	{
		u16 part_idR			= m_model->partitions().part_id("right_hand");
		u32 bc					= m_model->LL_PartBlendsCount(part_idR);
		for(u32 bidx=0; bidx<bc; ++bidx)
		{
			CBlend* BR			= m_model->LL_PartBlend(part_idR, bidx);
			if(!BR)
				continue;

			MotionID M			= BR->motionID;

			u16 pc					= m_model->partitions().count();
			for(u16 pid=0; pid<pc; ++pid)
			{
				if(pid!=part_idR)
				{
					CBlend* B			= m_model->PlayCycle(pid, M, TRUE);//this can destroy BR calling UpdateTracks !
					if( BR->blend_state() != CBlend::eFREE_SLOT )
					{
						u16 bop				= B->bone_or_part;
						*B					= *BR;
						B->bone_or_part		= bop;
					}
				}
			}
		}
	}else
	if(idx==0 && attached_item(1))
	{
		OnMovementChanged(mcAnyMove);
	}
}

void player_hud::detach_item(CHudItem* item)
{
	if( NULL==item->HudItemData() )		return;
	u16 item_idx						= item->HudItemData()->m_attach_place_idx;

	if( m_attached_items[item_idx]==item->HudItemData() )
	{
		detach_item_idx	(item_idx);
	}
}

void player_hud::calc_transform(u16 attach_slot_idx, const Fmatrix& offset, Fmatrix& result)
{
	Fmatrix ancor_m			= m_model->dcast_PKinematics()->LL_GetTransform(m_ancors[attach_slot_idx]);
	result.mul				(m_transform, ancor_m);
	result.mulB_43			(offset);
}

bool player_hud::inertion_allowed()
{
	attachable_hud_item* hi = m_attached_items[0];
	if(hi)
	{
		bool res = ( hi->m_parent_hud_item->HudInertionEnabled() && hi->m_parent_hud_item->HudInertionAllowed() );
		return	res;
	}
	return true;
}

void player_hud::OnMovementChanged(ACTOR_DEFS::EMoveCommand cmd)
{
	if(cmd==0)
	{
		if(m_attached_items[0])
		{
			if(m_attached_items[0]->m_parent_hud_item->GetState()==CHUDState::eIdle)
				m_attached_items[0]->m_parent_hud_item->PlayAnimIdle();
		}
		if(m_attached_items[1])
		{
			if(m_attached_items[1]->m_parent_hud_item->GetState()==CHUDState::eIdle)
				m_attached_items[1]->m_parent_hud_item->PlayAnimIdle();
		}
	}else
	{
		if(m_attached_items[0])
			m_attached_items[0]->m_parent_hud_item->OnMovementChanged(cmd);

		if(m_attached_items[1])
			m_attached_items[1]->m_parent_hud_item->OnMovementChanged(cmd);
	}

	updateMovementLayerState();
}

void player_hud::updateMovementLayerState()
{
	CActor* pActor = Actor();

	if (!pActor)
		return;

	for (movement_layer* anm : m_movement_layers)
	{
		anm->Stop(false);
	}

	bool need_blend = (m_attached_items[0] && m_attached_items[0]->m_parent_hud_item->NeedBlendAnm() || m_attached_items[1] && m_attached_items[1]->m_parent_hud_item->NeedBlendAnm());

	if (pActor->AnyMove() && need_blend)
	{
		CEntity::SEntityState state;
		pActor->g_State(state);

		CWeapon* wep = nullptr;

		if (m_attached_items[0] && m_attached_items[0]->m_parent_hud_item->object().cast_weapon())
			wep = m_attached_items[0]->m_parent_hud_item->object().cast_weapon();

		if (wep && wep->IsZoomed())
			state.bCrouch ? m_movement_layers[eAimCrouch]->Play() : m_movement_layers[eAimWalk]->Play();
		else if (state.bCrouch)
			m_movement_layers[eCrouch]->Play();
		else if (state.bSprint)
			m_movement_layers[eSprint]->Play();
		else if (!isActorAccelerated(pActor->MovingState(), false))
			m_movement_layers[eWalk]->Play();
		else
			m_movement_layers[eRun]->Play();
	}
}

void player_hud::PlayBlendAnm(LPCSTR name, u8 part, float speed, float power, bool bLooped, bool no_restart)
{
	for (script_layer* anm : m_script_layers)
	{
		if (!xr_strcmp(*anm->m_name, name))
		{
			if (!no_restart)
			{
				anm->anm->Stop();
				anm->blend_amount = 0.f;
				anm->blend.identity();
			}

			if (!anm->anm->IsPlaying())
				anm->anm->Play(bLooped);

			anm->anm->bLoop = bLooped;
			anm->m_part = part;
			anm->anm->Speed() = speed;
			anm->m_power = power;
			anm->active = true;
			return;
		}
	}

	script_layer* anm = xr_new<script_layer>(name, part, speed, power, bLooped);
	m_script_layers.push_back(anm);
}

void player_hud::StopBlendAnm(LPCSTR name, bool bForce)
{
	for (script_layer* anm : m_script_layers)
	{
		if (!xr_strcmp(*anm->m_name, name))
		{
			anm->Stop(bForce);
			return;
		}
	}
}
