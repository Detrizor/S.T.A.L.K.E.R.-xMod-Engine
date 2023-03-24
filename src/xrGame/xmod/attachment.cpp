#include "stdafx.h"
#include "../player_hud.h"

#include "attachment.h"

CAttachment::CAttachment()
{
	m_boneName						= NULL;
	m_offset[0]						= { 0.f, 0.f, 0.f };
	m_offset[1]						= { 0.f, 0.f, 0.f };
	m_renderPos.identity			();
}

DLL_Pure* CAttachment::_construct()
{
	m_object		 				= smart_cast<CInventoryItemObject*>(this);
	return							m_object;
}

void CAttachment::Load(LPCSTR section)
{
	m_boneName						= READ_IF_EXISTS(pSettings, r_string, section, "attach_bone", "wpn_body");
}

void CAttachment::UpdateRenderPos(IRenderVisual* model, Fmatrix parent)
{
	u16 bone_id						= model->dcast_PKinematics()->LL_BoneID(m_boneName);
	Fmatrix bone_trans				= model->dcast_PKinematics()->LL_GetTransform(bone_id);

	m_renderPos.identity			();
	m_renderPos.mul					(parent, bone_trans);

	Fmatrix							hud_rotation;
	hud_rotation.identity			();
	hud_rotation.rotateX			(m_offset[1].x);

	Fmatrix							hud_rotation_y;
	hud_rotation_y.identity			();
	hud_rotation_y.rotateY			(m_offset[1].y);
	hud_rotation.mulA_43			(hud_rotation_y);

	hud_rotation_y.identity			();
	hud_rotation_y.rotateZ			(m_offset[1].z);
	hud_rotation.mulA_43			(hud_rotation_y);

	hud_rotation.translate_over		(m_offset[0]);
	m_renderPos.mulB_43				(hud_rotation);
}

void CAttachment::Update_And_Render_World(IRenderVisual* model, Fmatrix parent)
{
	Fmatrix backup					= m_renderPos;
	UpdateRenderPos					(model, parent);
	::Render->set_Transform			(&m_renderPos);
	::Render->add_Visual			(m_object->Visual());
	m_renderPos						= backup;
}

void CAttachment::Render()
{
	::Render->set_Transform			(&m_renderPos);
	::Render->add_Visual			(m_object->Visual());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DLL_Pure* CAttachmentObject::_construct()
{
	inherited::_construct			();
	CAttachment::_construct			();
	return							this;
}

void CAttachmentObject::Load(LPCSTR section)
{
	inherited::Load					(section);
	CAttachment::Load				(section);
}
