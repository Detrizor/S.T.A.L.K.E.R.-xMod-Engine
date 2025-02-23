#include "stdafx.h"
#include <dinput.h>
#include "Actor.h"
#include "Torch.h"
#include "trade.h"
#include "../xrEngine/CameraBase.h"

#ifdef DEBUG
#	include "PHDebug.h"
#endif

#include "hit.h"
#include "PHDestroyable.h"
#include "Car.h"
#include "UIGameSP.h"
#include "inventory.h"
#include "level.h"
#include "game_cl_base.h"
#include "xr_level_controller.h"
#include "UsableScriptObject.h"
#include "actorcondition.h"
#include "actor_input_handler.h"
#include "string_table.h"
#include "UI/UIStatic.h"
#include "UI/UIActorMenu.h"
#include "UI/UIDragDropReferenceList.h"
#include "CharacterPhysicsSupport.h"
#include "InventoryBox.h"
#include "player_hud.h"
#include "../xrEngine/xr_input.h"
#include "flare.h"
#include "CustomDetector.h"
#include "clsid_game.h"
#include "hudmanager.h"
#include "Weapon.h"
#include "addon.h"
#include "addon_owner.h"

extern u32 hud_adj_mode;

void CActor::IR_OnKeyboardPress(int cmd)
{
	if(hud_adj_mode && pInput->iGetAsyncKeyState(DIK_LSHIFT))	return;

	if (Remote())		return;

	if (IsTalking())	return;
	if (m_input_external_handler && !m_input_external_handler->authorized(cmd))	return;
	
	switch (cmd)
	{
	case kWPN_FIRE:
	{
		u16 slot						= inventory().GetActiveSlot();
		if (slot == KNIFE_SLOT || slot == PISTOL_SLOT || slot == PRIMARY_SLOT || slot == SECONDARY_SLOT)
			mstate_wishful				&=~mcSprint;
		//-----------------------------
		if (OnServer())
		{
			NET_Packet P;
			P.w_begin(M_PLAYER_FIRE); 
			P.w_u16(ID());
			u_EventSend(P);
		}
		}break;
	default:
		break;
	}

	if (!g_Alive()) return;

	if(m_holder && kUSE != cmd)
	{
		m_holder->OnKeyboardPress			(cmd);
		if(m_holder->allowWeapon() && inventory().Action((u16)cmd, CMD_START))		return;
		return;
	}else
		if(inventory().Action((u16)cmd, CMD_START))					return;

#ifdef DEBUG
	if(psActorFlags.test(AF_NO_CLIP))
	{
		NoClipFly(cmd);
		return;
	}
#endif //DEBUG
	switch(cmd)
	{
	case kJUMP:		
		{
			mstate_wishful |= mcJump;
		}break;
	case kSPRINT_TOGGLE:
	case kCROUCH:
		m_state_toggle_tg = Device.dwTimeGlobal;
		break;
	case kCAM_1:	cam_Set			(eacFirstEye);				break;
	case kCAM_2:	cam_Set			(eacLookAt);				break;
	case kCAM_3:	cam_Set			(eacFreeLook);				break;
	case kNIGHT_VISION:
			SwitchNightVision();
			break;
	case kTORCH:
			SwitchTorch();
			break;
	case kUSE:
		ActorUse		();
		m_bPickupMode	= true;
		break;
	case kDROP:
		b_DropActivated			= TRUE;
		f_DropPower				= 0;
		break;
	case kNEXT_SLOT:
		OnNextWeaponSlot();
		break;
	case kPREV_SLOT:
		OnPrevWeaponSlot();
		break;
	case kQUICK_USE_1:
	case kQUICK_USE_2:
	case kQUICK_USE_3:
	case kQUICK_USE_4:
	case kQUICK_USE_5:
	case kQUICK_USE_6:
	case kQUICK_USE_7:
	case kQUICK_USE_8:
	case kQUICK_USE_9:
	case kQUICK_USE_10:
	{
		u8 idx									= u8(cmd - kQUICK_USE_1);
		const shared_str& item_name				= g_quick_use_slots[idx];
		if (item_name.size())
		{
			PIItem tmp							= NULL;

			PIItem active_item					= inventory().ActiveItem();
			PIItem left_item					= inventory().LeftItem();
			if (active_item && active_item->Section(true) == item_name)
				tmp								= active_item;
			else if (left_item && left_item->Section(true) == item_name)
				tmp								= left_item;
			else
				tmp								= inventory().Get(*item_name, idx, true);

			if (tmp)
			{
				if (tmp->InHands() || active_item && tmp->BaseSlot() == active_item->BaseSlot() || left_item && tmp->HandSlot() == LEFT_HAND_SLOT)
					inventory().ActivateItem	(tmp, eItemPlacePocket, idx);
				else if (inventory().CanPutInSlot(tmp, tmp->HandSlot()))
					inventory().ActivateItem	(tmp);
				else
					tmp->tryCustomUse			();
				
			}
		}
		break;
	}
	case kKICK:
		actor_kick();
		break;
	}
}

BOOL g_invert_zoom = 0;
void CActor::IR_OnMouseWheel(int direction)
{
	if(hud_adj_mode)
	{
		g_player_hud->tune	(Ivector().set(0,0,direction));
		return;
	}

	inventory().Action((direction > 0 != g_invert_zoom) ? kWPN_ZOOM_INC : kWPN_ZOOM_DEC, CMD_START);
}

extern BOOL g_hud_adjusment_mode;
void CActor::IR_OnKeyboardRelease(int cmd)
{
	if (hud_adj_mode && pInput->iGetAsyncKeyState(DIK_LSHIFT))	return;

	if (Remote())	return;

	if (m_input_external_handler && !m_input_external_handler->authorized(cmd))	return;

	if (g_Alive())	
	{
		if (m_holder)
		{
			m_holder->OnKeyboardRelease(cmd);
			
			if(m_holder->allowWeapon() && inventory().Action((u16)cmd, CMD_STOP))		return;
			return;
		}
		else if(inventory().Action((u16)cmd, CMD_STOP))
			return;

		if (g_hud_adjusment_mode)
			return;

		switch(cmd)
		{
		case kJUMP:		mstate_wishful &=~mcJump;		break;
		case kDROP:		if(GAME_PHASE_INPROGRESS == Game().Phase()) g_PerformDrop();				break;
		case kSPRINT_TOGGLE:{
			if (Device.dwTimeGlobal - m_state_last_tg > m_state_toggle_delay)
				mstate_wishful ^= mcAccel;
			}break;
		case kCROUCH:{
			if (Device.dwTimeGlobal - m_state_last_tg > m_state_toggle_delay)
				mstate_wishful ^= mcCrouch;
			}break;
		}
	}
}

void CActor::IR_OnKeyboardHold(int cmd)
{
	if (hud_adj_mode && pInput->iGetAsyncKeyState(DIK_LSHIFT))	return;

	if (Remote() || !g_Alive())					return;
	if (m_input_external_handler && !m_input_external_handler->authorized(cmd))	return;
	if (IsTalking())							return;

	if(m_holder)
	{
		m_holder->OnKeyboardHold(cmd);
		return;
	}

#ifdef DEBUG
	if(psActorFlags.test(AF_NO_CLIP) && (cmd==kFWD || cmd==kBACK || cmd==kL_STRAFE || cmd==kR_STRAFE 
		|| cmd==kJUMP || cmd==kCROUCH))
	{
		NoClipFly(cmd);
		return;
	}
#endif //DEBUG

	if (g_hud_adjusment_mode)
		return;

	float LookFactor = GetLookFactor();
	switch(cmd)
	{
	case kUP:
	case kDOWN: 
		cam_Active()->Move( (cmd==kUP) ? kDOWN : kUP, 0, LookFactor);									break;
	case kCAM_ZOOM_IN: 
	case kCAM_ZOOM_OUT: 
		cam_Active()->Move(cmd);												break;
	case kLEFT:
	case kRIGHT:
		if (eacFreeLook!=cam_active) cam_Active()->Move(cmd, 0, LookFactor);	break;

	case kSPRINT_TOGGLE:{
		if (Device.dwTimeGlobal - m_state_toggle_tg > m_state_toggle_delay)
		{
			if (!isActorAccelerated(mstate_wishful, IsZoomADSMode()))
				mstate_wishful ^= mcAccel;
			if (mstate_wishful&mcCrouch && AnyMove())
				mstate_wishful ^= mcCrouch;
			mstate_wishful |= mcSprint;
			m_state_last_tg = Device.dwTimeGlobal;
		}
		}break;
	case kL_STRAFE:		mstate_wishful |= mcLStrafe;								break;
	case kR_STRAFE:		mstate_wishful |= mcRStrafe;								break;
	case kL_LOOKOUT:	mstate_wishful |= mcLLookout;								break;
	case kR_LOOKOUT:	mstate_wishful |= mcRLookout;								break;
	case kFWD:			mstate_wishful |= mcFwd;									break;
	case kBACK:			mstate_wishful |= mcBack;									break;
	case kCROUCH:
		if (Device.dwTimeGlobal - m_state_toggle_tg > m_state_toggle_delay)
		{
			if (!(mstate_wishful&mcCrouch))
				mstate_wishful ^= mcCrouch;
			mstate_wishful |= mcCrouchLow;
			m_state_last_tg = Device.dwTimeGlobal;
		}
		break;
	}
}

void CActor::IR_OnMouseMove(int dx, int dy)
{

	if(hud_adj_mode)
	{
		g_player_hud->tune	(Ivector().set(dx,dy,0));
		return;
	}

	PIItem iitem = inventory().ActiveItem();
	if(iitem && iitem->cast_hud_item())
		iitem->cast_hud_item()->ResetSubStateTime();

	if (Remote())		return;

	if(m_holder) 
	{
		m_holder->OnMouseMove(dx,dy);
		return;
	}

	float fov		= (Device.SVP.isActive()) ? Device.SVP.getFOV() : cameras[cam_active]->f_fov;
	float scale		= (fov / Device.gFOV) * GetLookFactor() * psMouseSensScale / psMouseSens;
	if (IsZoomADSMode())
		scale		*= psADSSensScale;
	else if (IsZoomAimingMode())
		scale		*= psAimSensScale;

	if (dx)
	{
		float d				= float(dx) * scale;
		cam_Active()->Move	((d < 0) ? kLEFT : kRIGHT, _abs(d));
	}
	if (dy)
	{
		float d				= ((psMouseInvert.test(1)) ? -1 : 1) * float(dy) * scale * psVertSensScale;
		cam_Active()->Move	((d > 0) ? kUP : kDOWN, _abs(d));
	}
}

#include "HudItem.h"
bool CActor::use_Holder				(CHolderCustom* holder)
{

	if(m_holder){
		bool b = false;
		CGameObject* holderGO			= smart_cast<CGameObject*>(m_holder);
		
		if(smart_cast<CCar*>(holderGO))
			b = use_Vehicle(0);
		else
			b = use_HolderEx(0,false);

		if(inventory().ActiveItem()){
			CHudItem* hi = smart_cast<CHudItem*>(inventory().ActiveItem());
			if(hi) hi->OnAnimationEnd(hi->GetState());
		}

		return b;
	}else{
		bool b = false;
		//CGameObject* holderGO			= smart_cast<CGameObject*>(holder);
		if(smart_cast<CCar*>(holder))
			b = use_Vehicle(holder);
		else
			b = use_HolderEx(holder,false);
		
		if(b){//used succesfully
			// switch off torch...
			CAttachableItem *I = CAttachmentOwner::attachedItem(CLSID_DEVICE_TORCH);
			if (I){
				CTorch* torch = smart_cast<CTorch*>(I);
				if (torch) torch->Switch(false);
			}
		}

		if(inventory().ActiveItem()){
			CHudItem* hi = smart_cast<CHudItem*>(inventory().ActiveItem());
			if(hi) hi->OnAnimationEnd(hi->GetState());
		}

		return b;
	}
}

void CActor::ActorUse()
{
	if (CurrentGameUI()->GetActorMenu().GetMenuMode() != mmUndefined)
		return;

	if (m_holder)
	{
		CGameObject*				GO = smart_cast<CGameObject*>(m_holder);
		NET_Packet					P;
		CGameObject::u_EventGen		(P, GEG_PLAYER_DETACH_HOLDER, ID());
		P.w_u16						(GO->ID());
		CGameObject::u_EventSend	(P);
		return;
	}

	if (character_physics_support()->movement()->PHCapture())
		character_physics_support()->movement()->PHReleaseObject();

	if (m_pUsableObject && !m_pObjectWeLookingAt->cast_inventory_item())
		m_pUsableObject->use(this);

	if (m_pInvBoxWeLookingAt && m_pInvBoxWeLookingAt->nonscript_usable())
	{
		CUIGameSP* pGameSP				= smart_cast<CUIGameSP*>(CurrentGameUI());
		if (pGameSP && !m_pInvBoxWeLookingAt->closed())
		{
			pGameSP->StartCarBody		(mmInvBoxSearch, static_cast<void*>(m_pInvBoxWeLookingAt));
			return;
		}
	}

	if (m_pContainerWeLookingAt && !Level().IR_GetKeyState(DIK_LSHIFT))
	{
		CUIGameSP* pGameSP				= smart_cast<CUIGameSP*>(CurrentGameUI());
		if (pGameSP)
		{
			pGameSP->StartCarBody		(mmContainerSearch, static_cast<void*>(m_pContainerWeLookingAt));
			return;
		}
	}

	if (m_pUsableObject && !m_pUsableObject->nonscript_usable())
		return;

	bool bCaptured						= false;
	collide::rq_result& RQ				= HUD().GetCurrentRayQuery();
	CPhysicsShellHolder* object			= smart_cast<CPhysicsShellHolder*>(RQ.O);
	if (object)
	{
		u16 element						= (u16)RQ.element;
		if (Level().IR_GetKeyState(DIK_LSHIFT))
		{
			bool b_allow				= !!pSettings->line_exist("ph_capture_visuals", object->cNameVisual());
			if (b_allow && !character_physics_support()->movement()->PHCapture())
			{
				character_physics_support()->movement()->PHCaptureObject(object, element);
				bCaptured				= true;
			}
		}
		else if (smart_cast<CHolderCustom*>(object))
		{
			NET_Packet					P;
			CGameObject::u_EventGen		(P, GEG_PLAYER_ATTACH_HOLDER, ID());
			P.w_u16						(object->ID());
			CGameObject::u_EventSend	(P);
			return;
		}
	}
	
	auto ea								= smart_cast<CEntityAlive*>(m_pObjectWeLookingAt);
	if (m_pPersonWeLookingAt)
	{
		VERIFY							(ea);
		if (ea->g_Alive())
		{
			TryToTalk					();
			return;
		}

		if (!bCaptured && !m_pPersonWeLookingAt->deadbody_closed_status() && ea->AlreadyDie())
		{
			CUIGameSP* pGameSP			= smart_cast<CUIGameSP*>(CurrentGameUI());
			pGameSP->StartCarBody		(mmDeadBodySearch, static_cast<void*>(m_pPersonWeLookingAt));
			return;
		}
	}

	if (ea)
		return;

	::luabind::functor<bool> funct;
	if (ai().script_engine().functor("xmod_action_manager.on_actor_use", funct))
		funct							();
}

BOOL CActor::HUDview				( )const 
{ 
	return IsFocused() && (cam_active==eacFirstEye)&&
		((!m_holder) || (m_holder && m_holder->allowWeapon() && m_holder->HUDView() ) ); 
}

static	u16 SlotsToCheck [] = {
		KNIFE_SLOT		,
		PISTOL_SLOT		,
		PRIMARY_SLOT	,
		SECONDARY_SLOT	,
		GRENADE_SLOT	,
};

void	CActor::OnNextWeaponSlot()
{
	u32 ActiveSlot = inventory().GetActiveSlot();
	if (ActiveSlot == NO_ACTIVE_SLOT) 
		ActiveSlot = inventory().GetPrevActiveSlot();

	if (ActiveSlot == NO_ACTIVE_SLOT) 
		ActiveSlot = KNIFE_SLOT;
	
	u32 NumSlotsToCheck = sizeof(SlotsToCheck)/sizeof(SlotsToCheck[0]);	
	
	u32 CurSlot			= 0;
	for (; CurSlot<NumSlotsToCheck; CurSlot++)
	{
		if (SlotsToCheck[CurSlot] == ActiveSlot) break;
	};

	if (CurSlot >= NumSlotsToCheck) 
		return;

	for (u32 i=CurSlot+1; i<NumSlotsToCheck; i++)
	{
		if (inventory().ItemFromSlot(SlotsToCheck[i]))
		{
			IR_OnKeyboardPress(kWPN_1 + i);
			return;
		}
	}
};

void	CActor::OnPrevWeaponSlot()
{
	u32 ActiveSlot = inventory().GetActiveSlot();
	if (ActiveSlot == NO_ACTIVE_SLOT) 
		ActiveSlot = inventory().GetPrevActiveSlot();

	if (ActiveSlot == NO_ACTIVE_SLOT) 
		ActiveSlot = KNIFE_SLOT;

	u32 NumSlotsToCheck = sizeof(SlotsToCheck)/sizeof(SlotsToCheck[0]);	
	u32 CurSlot		= 0;

	for (; CurSlot<NumSlotsToCheck; CurSlot++)
	{
		if (SlotsToCheck[CurSlot] == ActiveSlot) break;
	};

	if (CurSlot >= NumSlotsToCheck) 
		CurSlot	= NumSlotsToCheck-1; //last in row

	for (s32 i=s32(CurSlot-1); i>=0; i--)
	{
		if (inventory().ItemFromSlot(SlotsToCheck[i]))
		{
			IR_OnKeyboardPress(kWPN_1 + i);
			return;
		}
	}
};

float CActor::GetLookFactor()
{
	if (m_input_external_handler) 
		return m_input_external_handler->mouse_scale_factor();
	return 1.f;
}

void CActor::set_input_external_handler(CActorInputHandler *handler) 
{
	// clear state
	if (handler) 
		mstate_wishful			= 0;

	// release fire button
	if (handler)
		IR_OnKeyboardRelease	(kWPN_FIRE);

	// set handler
	m_input_external_handler	= handler;
}

void CActor::SwitchNightVision()
{
	CWeapon* wpn1 = NULL;
	CWeapon* wpn2 = NULL;
	CWeapon* wpn3 = NULL;
	if (inventory().ItemFromSlot(PISTOL_SLOT))
		wpn1 = smart_cast<CWeapon*>(inventory().ItemFromSlot(PISTOL_SLOT));
	if (inventory().ItemFromSlot(PRIMARY_SLOT))
		wpn2 = smart_cast<CWeapon*>(inventory().ItemFromSlot(PRIMARY_SLOT));
	if (inventory().ItemFromSlot(SECONDARY_SLOT))
		wpn3 = smart_cast<CWeapon*>(inventory().ItemFromSlot(SECONDARY_SLOT));

	xr_vector<CAttachableItem*> const& all = CAttachmentOwner::attached_objects();
	for (xr_vector<CAttachableItem*>::const_iterator it = all.begin(), it_e = all.end(); it != it_e; ++it)
	{
		CTorch* torch = smart_cast<CTorch*>(*it);
		if (torch)
		{	
			if (wpn1 && wpn1->IsZoomed())
				return;
			if (wpn2 && wpn2->IsZoomed())
				return;
			if (wpn3 && wpn3->IsZoomed())
				return;

			torch->SwitchNightVision();
			return;
		}
	}
}

void CActor::SwitchTorch()
{
	if (auto pTorch = inventory().ItemFromSlot(TORCH_SLOT)->O.scast<CTorch*>())
	{
		if (inventory().ItemFromSlot(HEADLAMP_SLOT))
			pTorch->Switch();
		else if (auto left_item = inventory().ItemFromSlot(LEFT_HAND_SLOT))
			if (left_item->Category("equipment", "device", "torch"))
				if (auto hi = left_item->O.scast<CHudItem*>())
					if (!hi->IsHiding() && !hi->IsShowing())
						pTorch->Switch();
	}
}

void CActor::actor_kick()
{
	CGameObject *O = ObjectWeLookingAt();
	if (!O)
		return;
		
	float mass_f = 100.f;

	CEntityAlive *EA = smart_cast<CEntityAlive*>(O);
	if (EA)
	{
		if (EA->character_physics_support())
			mass_f = EA->character_physics_support()->movement()->GetMass();
		else
			mass_f = EA->GetMass();

		if (EA->g_Alive() && mass_f > 20.0f) //ability to kick tuskano and rat
			return;
	}
	else
	{
		CPhysicsShellHolder *sh = smart_cast<CPhysicsShellHolder*>(O);
		if (sh)
			mass_f = sh->GetMass();

		PIItem itm = smart_cast<PIItem>(O);
		if (itm)
			mass_f = itm->Weight();
	}

	CInventoryOwner *io = smart_cast<CInventoryOwner*> (O);
	if (io)
		mass_f += io->inventory().TotalWeight();

	static float kick_impulse = READ_IF_EXISTS(pSettings, r_float, "actor", "kick_impulse", 250.f);
	Fvector dir = Direction();
	dir.y = sin(15.f * PI / 180.f);
	dir.normalize();

	u16 bone_id = 0;
	collide::rq_result& RQ = HUD().GetCurrentRayQuery();
	if (RQ.O == O && RQ.element != 0xffff)
		bone_id = (u16)RQ.element;

	clamp<float>(mass_f, 1.0f, 100.f); // ���������� ��������� ����

	// The smaller the mass, the more damage given capped at 60 mass. 60+ mass take 0 damage
	float hit_power = 100.f * ( (mass_f/100.f) - 0.6f) / (0.f - 0.6f);
	clamp<float>(hit_power, 0.f, 100.f);
	hit_power /= 100;

	//shell->applyForce(dir, kick_power * conditions().GetPower());
	Fvector h_pos = O->Position();
	SHit hit = SHit(hit_power, dir, this, bone_id, h_pos, kick_impulse, ALife::eHitTypeStrike);
	O->Hit(&hit);
	if (EA)
	{
		static float alive_kick_power = 3.f;
		float real_imp = kick_impulse / mass_f;
		dir.mul(pow(real_imp, alive_kick_power));
		if (EA->character_physics_support())
		{
			EA->character_physics_support()->movement()->AddControlVel(dir);
			EA->character_physics_support()->movement()->ApplyImpulse(dir.normalize(), kick_impulse * alive_kick_power);
		}
	}

	conditions().ConditionJump(mass_f / 50);
}

#ifdef DEBUG
void CActor::NoClipFly(int cmd)
{
	Fvector cur_pos;// = Position();
	cur_pos.set(0,0,0);
	float scale = 1.0f;
	if(pInput->iGetAsyncKeyState(DIK_LSHIFT))
		scale = 0.25f;
	else if(pInput->iGetAsyncKeyState(DIK_LMENU))
		scale = 4.0f;

	switch(cmd)
	{
	case kJUMP:		
		cur_pos.y += 0.1f;
		break;
	case kCROUCH:	
		cur_pos.y -= 0.1f;
		break;
	case kFWD:	
		cur_pos.z += 0.1f;
		break;
	case kBACK:
		cur_pos.z -= 0.1f;
		break;
	case kL_STRAFE:
		cur_pos.x -= 0.1f;
		break;
	case kR_STRAFE:
		cur_pos.x += 0.1f;
		break;
	case kCAM_1:	
		cam_Set(eacFirstEye);				
		break;
	case kCAM_2:	
		cam_Set(eacLookAt);				
		break;
	case kCAM_3:	
		cam_Set(eacFreeLook);
		break;
	case kNIGHT_VISION:
		SwitchNightVision();
		break;
	case kTORCH:
		SwitchTorch();
		break;
	case kDETECTOR:
		{
			PIItem det_active = inventory().ItemFromSlot(DETECTOR_SLOT);
			if(det_active)
			{
				CCustomDetector* det = smart_cast<CCustomDetector*>(det_active);
				if (det)
					det->ToggleDetector(g_player_hud->attached_item(0)!=NULL);
				return;
			}
		}
		break;
	case kUSE:
		ActorUse();
		break;
	}
	cur_pos.mul(scale);
	Fmatrix	mOrient;
	mOrient.rotateY(-(cam_Active()->GetWorldYaw()));
	mOrient.transform_dir(cur_pos);
	Position().add(cur_pos);
	character_physics_support()->movement()->SetPosition(Position());
}
#endif //DEBUG