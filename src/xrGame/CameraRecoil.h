////////////////////////////////////////////////////////////////////////////
//	Module 		: CameraRecoil.h
//	Created 	: 26.05.2008
//	Author		: Evgeniy Sokolov
//	Description : Camera Recoil struct
////////////////////////////////////////////////////////////////////////////

#ifndef CAMERA_RECOIL_H_INCLUDED
#define CAMERA_RECOIL_H_INCLUDED

//מעהאקא ןנט סענוכüבו 
struct CameraRecoil
{
	float		RelaxSpeed;
	float		RelaxSpeed_AI;
	float		Dispersion;
	float		DispersionInc;
	float		DispersionFrac;
	float		MaxAngleVert;
	float		MaxAngleHorz;
	float		StepAngleHorz;
	bool		ReturnMode;
	bool		StopReturn;

	CameraRecoil():
		MaxAngleVert	( EPS   ),
		RelaxSpeed		( EPS_L ),
		RelaxSpeed_AI	( EPS_L ),
		Dispersion		( EPS   ),
		DispersionInc	( 0.0f  ),
		DispersionFrac	( 1.0f  ),
		MaxAngleHorz	( EPS   ),
		StepAngleHorz	( 0.0f  ),
		ReturnMode		( false ),
		StopReturn		( false )
	{};

	CameraRecoil( const CameraRecoil& clone )		{	Clone( clone );	}

	IC void Clone(CameraRecoil CR$ clone, float recoil_modifier = 1.f)
	{
		// *this = clone;
		RelaxSpeed		= clone.RelaxSpeed / recoil_modifier;
		RelaxSpeed_AI	= clone.RelaxSpeed_AI / recoil_modifier;
		Dispersion		= clone.Dispersion * recoil_modifier;
		DispersionInc	= clone.DispersionInc * recoil_modifier;
		DispersionFrac	= clone.DispersionFrac * recoil_modifier;
		MaxAngleVert	= clone.MaxAngleVert * recoil_modifier;
		MaxAngleHorz	= clone.MaxAngleHorz * recoil_modifier;
		StepAngleHorz	= clone.StepAngleHorz * recoil_modifier;

		ReturnMode		= clone.ReturnMode;
		StopReturn		= clone.StopReturn;
		
		VERIFY( !fis_zero(RelaxSpeed)    );
		VERIFY( !fis_zero(RelaxSpeed_AI) );
		VERIFY( !fis_zero(MaxAngleVert)  );
		VERIFY( !fis_zero(MaxAngleHorz)  );
	}
}; //struct CameraRecoil

#endif // CAMERA_RECOIL_H_INCLUDED
