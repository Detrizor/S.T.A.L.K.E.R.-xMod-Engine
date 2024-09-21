#pragma once
#include "customdetector.h"

class CUIArtefactDetectorSimple;

class CSimpleDetector :public CCustomDetector
{
	typedef CCustomDetector	inherited;
public:
					CSimpleDetector				();
	
protected:
//.	virtual void 	UpdateZones					();
	virtual void 	UpdateAf					();
	virtual void 	CreateUI					();
	CUIArtefactDetectorSimple&	ui				();
};

