// Level_Bullet_Manager.h:  для обеспечения полета пули по траектории
//							все пули и осколки передаются сюда
//////////////////////////////////////////////////////////////////////

#pragma once

#include "weaponammo.h"
#include "tracer.h"

struct SPowerDependency
{
	Fvector3 p1;
	float t;
	Fvector3 p2;
	SPowerDependency() :p1({ 0.f, 0.f, 0.f }), t(0.f), p2({ 0.f, 0.f, 0.f }) {}
	void Load(LPCSTR sect, LPCSTR line) { LPCSTR C = pSettings->r_string(sect, line); int r = sscanf(C, "%f,%f,%f,%f,%f,%f,%f", &p1.x, &p1.y, &p1.z, &t, &p2.x, &p2.y, &p2.z); if (r < 7) p2 = p1; }
	float Calc(float param) const { const Fvector3& p = (param < t) ? p1 : p2; return pow(p[0] + p[1] * param, p[2]); }
};

//коэфициенты и параметры патрона
struct SBullet_Hit 
{
	float	impulse;
	float	main_damage;
	float	pierce_damage;
	float	armor_pierce_damage;
};

//структура, описывающая пулю и ее свойства в полете
struct SBullet
{
	u32				init_frame_num		;			//номер кадра на котором была запущена пуля
	union			{
		struct			{
			u16			ricochet_was	: 1	;			//пуля срикошетила
			u16			explosive		: 1	;			//special explosive mode for particles
			u16			allow_tracer	: 1	;
			u16			allow_ricochet	: 1	;			//разрешить рикошет
			u16			allow_sendhit	: 1	;			//statistics
			u16			magnetic_beam	: 1 ;			//магнитный луч (нет отклонения после пробивания, не падает скорость после пробивания)
			u16			piercing_was	: 1	;
		};
		u16				_storage			;
	}				flags				;
	u16				bullet_material_idx	;

	Fvector			pos					;			//текущая позиция
	Fvector			dir					;
	float			speed				;			//текущая скорость
	
	u16				parent_id			;			//ID персонажа который иницировал действие
	u16				weapon_id			;			//ID оружия из которого была выпущены пуля
	
	float			fly_dist			;			//дистанция которую пуля пролетела
	Fvector			tracer_start_position;

	//коэфициенты и параметры патрона
	SBullet_Hit     hit_param;
	//-------------------------------------------------------------------
	float			air_resistance		;
	//-------------------------------------------------------------------
	u16				updates				;
	float			wallmark_size		;
	float			bullet_mass			;
	float			bullet_resist		;
	bool			hollow_point		;
	bool			armor_piercing		;
	//-------------------------------------------------------------------
	u8				m_u8ColorID			;
	
	//тип наносимого хита
	ALife::EHitType hit_type			;
	//---------------------------------
	u32				m_dwID				;
	ref_sound		m_whine_snd			;
	ref_sound		m_mtl_snd			;
	//---------------------------------
	u16				targetID			;
	//---------------------------------
	float			density				;
	bool			operator	==		(u32 ID){return	ID == m_dwID;}
public:
					SBullet				();
					~SBullet			();

	bool			CanBeRenderedNow	() const { return (Device.dwFrame > init_frame_num);}

	void			Init				(const	Fvector& position,
										const	Fvector& direction,
										float	start_speed,
										u16		sender_id,
										u16		sendersweapon_id,
										ALife::EHitType e_hit_type,
										float	maximum_distance,
										const	CCartridge& cartridge,
										bool	SendHit,
										float	power,
										float	impulse,
										int iShotNum = 0);
};

class CLevel;

class CBulletManager
{
private:
	static float const parent_ignore_distance;

private:
	collide::rq_results		rq_storage;
	collide::rq_results		m_rq_results;

private:
	DEFINE_VECTOR						(ref_sound,SoundVec,SoundVecIt);
	DEFINE_VECTOR						(SBullet,BulletVec,BulletVecIt);
	friend	CLevel;

	enum EventType {
		EVENT_HIT	= u8(0),
		EVENT_REMOVE,

		EVENT_DUMMY = u8(-1),
	};
	struct	_event			{
		EventType			Type;
		BOOL				dynamic		;
		SBullet_Hit			hit_result	;
		SBullet				bullet		;
		Fvector				normal		;
		Fvector				point		;
		collide::rq_result	R			;
		u16					tgt_material;
	};
	static void CalculateNewVelocity(Fvector & dest_new_vel, Fvector const & old_velocity, float ar, float life_time);
protected:
	SoundVec				m_WhineSounds		;
	RStringVec				m_ExplodeParticles	;

	//список пуль находящихся в данный момент на уровне
//.	xrCriticalSection		m_Lock				;

	BulletVec				m_Bullets			;	// working set, locked
	BulletVec				m_BulletsRendered	;	// copy for rendering
	xr_vector<_event>		m_Events			;

#ifdef DEBUG
	u32						m_thread_id;

	typedef xr_vector<Fvector>	BulletPoints;
	BulletPoints			m_bullet_points;
#endif // #ifdef DEBUG

	//отрисовка трассеров от пуль
	CTracer					tracers;

	//минимальная скорость, на которой пуля еще считается
	static float			m_fMinBulletSpeed;

	float					m_fHPMaxDist;

	//константа G
	float					m_fGravityConst;
	//cколько процентов энергии потеряет пуля при столкновении с материалом (при падении под прямым углом)
	float					m_fCollisionEnergyMin;
	//сколькол процентов энергии устанется у пули при любом столкновении
	float					m_fCollisionEnergyMax;

	//параметры отрисовки трассеров
	float					m_fTracerWidth;
	float 					m_fTracerLengthMax;
	float 					m_fTracerLengthMin;

protected:
	void					PlayWhineSound		(SBullet* bullet, CObject* object, const Fvector& pos);
	void					PlayExplodePS		(const Fmatrix& xf);
	//функция обработки хитов объектов
	static BOOL 			test_callback		(const collide::ray_defs& rd, CObject* object, LPVOID params);
	static BOOL				firetrace_callback	(collide::rq_result& result, LPVOID params);

	// Deffer event
	void					RegisterEvent		(EventType Type, BOOL _dynamic, SBullet* bullet, const Fvector& end_point, collide::rq_result& R, u16 target_material);
	
	//попадание по динамическому объекту
	void					DynamicObjectHit	(_event& E);
	
	//попадание по статическому объекту
	void					StaticObjectHit		(_event& E);

	//попадание по любому объекту, на выходе - импульс и сила переданные пулей объекту
	bool					ObjectHit			(SBullet_Hit* hit_res, SBullet* bullet, const Fvector& end_point, 
												collide::rq_result& R, u16 target_material, Fvector& hit_normal);
	//отметка на пораженном объекте
	void					FireShotmark		(SBullet* bullet, const Fvector& vDir, 
												const Fvector &vEnd,    collide::rq_result& R,  u16 target_material,
												const Fvector& vNormal, bool ShowMark = true);
	//просчет полета пули за некоторый промежуток времени
	//принимается что на этом участке пуля движется прямолинейно
	//и равномерно, а после просчета также изменяется текущая
	//скорость и положение с учетом гравитации и ветра
	//возвращаем true если пуля продолжает полет
	bool					update_bullet(
								collide::rq_results& rq_storage, 
								SBullet& bullet,
								float time_delta,
								Fvector const& gravity
							);
	bool					process_bullet		(
								collide::rq_results& rq_storage,
								SBullet& bullet,
								float time_delta
							);
	void 		__stdcall	UpdateWorkload		();

public:
							CBulletManager		();
	virtual					~CBulletManager		();

	void 					Load				();
	void 					Clear				();
	void 					AddBullet			(const Fvector& position, const Fvector& direction, float starting_speed, 
												u16	sender_id, u16 sendersweapon_id,
												ALife::EHitType e_hit_type, float maximum_distance, 
												const CCartridge& cartridge, bool SendHit,
												float power, float impulse, int iShotNum = 0);

	void					CommitEvents		();	// @ the start of frame
	void					CommitRenderSet		();	// @ the end of frame
	void 					Render				();

	float					m_fBulletAirResistanceScale;
	float					m_fBulletWallMarkSizeScale;

	float					m_fZeroingAirResistCorrectionK1;
	float					m_fZeroingAirResistCorrectionK2;
	float					m_fZeroingAirResistCorrectionK3;

	float					m_fBulletAPScale;
	float					m_fBulletArmorPiercingAPFactor;
	float					m_fBulletHollowPointAPFactor;
	float					m_fBulletHollowPointResistFactor;
	float					m_fBulletAPLossOnPierce;

	float					m_fBulletHitImpulseScale;
	float					m_fBulletArmorDamageScale;
	float					m_fBulletPierceDamageScale;
	float					m_fBulletArmorPierceDamageScale;

	SPowerDependency		m_fBulletPierceDamageFromResist;
	SPowerDependency		m_fBulletPierceDamageFromKAP;
	SPowerDependency		m_fBulletPierceDamageFromSpeed;

	SPowerDependency		m_fBulletPierceDamageFromSpeedScale;
	SPowerDependency		m_fBulletPierceDamageFromHydroshock;
	SPowerDependency		m_fBulletPierceDamageFromStability;
	SPowerDependency		m_fBulletPierceDamageFromPierce;

	float								GravityConst						C$	()		{ return m_fGravityConst; }
	float								CalcZeroingCorrection				C$	(float k, float z);
};

struct bullet_test_callback_data
{
	SBullet*		pBullet;
	float			collide_time;
	float			dt;
	Fvector			start_velocity;
	Fvector			end_velocity;
	Fvector			avg_velocity;
};
