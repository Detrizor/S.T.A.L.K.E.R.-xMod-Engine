#pragma once
#include "UIWindow.h"
#include "..\..\xrServerEntities\alife_space.h"

class CUIXml;
class CUIStatic;
class CUITextWnd;
class UIArtefactParamItem;
class CUICellItem;
class CArtefact;

enum EAbsorbationTypes
{
	eBurnImmunity,
	eShockImmunity,
	eChemBurnImmunity,
	eRadiationImmunity,
	eTelepaticImmunity,
	eAbsorbationTypeMax
};

enum EConditionRestoreTypes {
	eRadiationSpeed = 0,
	ePainkillSpeed,
	eRegenerationSpeed,
	eRecuperationSpeed,
	eRestoreTypeMax
};

class CUIArtefactParams : public CUIWindow
{
public:
					CUIArtefactParams		();
	virtual			~CUIArtefactParams		();
			void	InitFromXml				(CUIXml& xml);
			void	SetInfo					(LPCSTR section, CArtefact* art);

protected:
	UIArtefactParamItem*	m_absorbation_item[eAbsorbationTypeMax];
	UIArtefactParamItem*	m_restore_item[eRestoreTypeMax];
	UIArtefactParamItem*	m_drain_factor;
	UIArtefactParamItem*	m_weight_dump;
	UIArtefactParamItem*	m_armor;
	UIArtefactParamItem*	m_radiation;

	CUIStatic*				m_Prop_line;

private:
	void								SetInfoItem								(UIArtefactParamItem* item, float value, Fvector2& pos, float& h);
}; // class CUIArtefactParams

// -----------------------------------

class UIArtefactParamItem : public CUIWindow
{
public:
				UIArtefactParamItem	();
	virtual		~UIArtefactParamItem();
		
		void	Init				( CUIXml& xml, LPCSTR section );
		void	SetCaption			( LPCSTR name );
		void	SetValue			( float value );
	
private:
	CUIStatic*	m_caption;
	CUITextWnd*	m_value;
	float		m_magnitude;
	bool		m_sign_inverse;
	bool		m_perc_unit;
	shared_str	m_unit_str;
	shared_str	m_texture_minus;
	shared_str	m_texture_plus;

}; // class UIArtefactParamItem
