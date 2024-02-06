////////////////////////////////////////////////////////////////////////////
//	Module 		: CameraRecoil.h
//	Created 	: 26.05.2008
//	Author		: Evgeniy Sokolov
//	Description : Camera Recoil struct
////////////////////////////////////////////////////////////////////////////

#pragma once

//מעהאקא ןנט סענוכüבו 
struct CameraRecoil
{
	float		StepAngleVert;
	float		StepAngleVertInc;
	float		MaxAngleVert;

	float		StepAngleHorz;
	float		StepAngleHorzInc;
	float		MaxAngleHorz;

	CameraRecoil():
		StepAngleVert		(0.f),
		StepAngleVertInc	(0.f),
		MaxAngleVert		(EPS),
		
		StepAngleHorz		(0.f),
		StepAngleHorzInc	(0.f),
		MaxAngleHorz		(EPS)
	{};
};
