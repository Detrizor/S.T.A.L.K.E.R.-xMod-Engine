#include "pch_script.h"
#include "trade.h"
#include "actor.h"
#include "ai/stalker/ai_stalker.h"
#include "ai/trader/ai_trader.h"
#include "artefact.h"
#include "inventory.h"
#include "xrmessages.h"
#include "character_info.h"
#include "relation_registry.h"
#include "level.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
#include "game_object_space.h"
#include "trade_parameters.h"
#include "gameobject.h"
#include "ai_object_location.h"
#include "ui\UICellItem.h"

static const float CURRENCY_RATE = pSettings->r_float("trade", "currency_rate");

bool CTrade::CanTrade()
{
	CEntity *pEntity;

	m_nearest.clear_not_free		();
	Level().ObjectSpace.GetNearest	(m_nearest,pThis.base->Position(),2.f, NULL);
	if (!m_nearest.empty()) 
	{
		for (u32 i=0, n = m_nearest.size(); i<n; ++i) 
		{
			// Может ли объект торговать
			pEntity = smart_cast<CEntity *>(m_nearest[i]);
			if (pEntity && !pEntity->g_Alive()) return false;
			if (SetPartner(pEntity)) break;
		}
	} 
	
	if (!pPartner.base) return false;

	// Объект рядом
	float dist = pPartner.base->Position().distance_to(pThis.base->Position());
	if (dist < 0.5f || dist > 4.5f)  
	{
		RemovePartner();
		return false;
	}

	// Объект смотрит на меня
	float yaw, pitch;
	float yaw2, pitch2;

	pThis.base->Direction().getHP(yaw,pitch);
	pPartner.base->Direction().getHP(yaw2,pitch2);
	yaw = angle_normalize(yaw);
	yaw2 = angle_normalize(yaw2);

	float Res = rad2deg(_abs(yaw - yaw2) < PI ? _abs(yaw - yaw2) : 
								 PI_MUL_2 - _abs(yaw - yaw2));
	if (Res < 165.f || Res > 195.f) 
	{
		RemovePartner();
		return false;
	}

	return true;
}

void CTrade::TransferItem(CUICellItem* itm, bool bBuying, bool bFree)
{
	// сумма сделки учитывая ценовой коэффициент
	// актер цену не говорит никогда, все делают за него
	int dwTransferMoney								= (int)GetItemPrice(itm, bBuying, bFree);
	PIItem pItem									= (PIItem)itm->m_pData;
	shared_str& section								= itm->m_section;

	if (bBuying)
	{
		if (pItem)
		{
			pPartner.inv_owner->on_before_sell		(pItem);
			pThis.inv_owner->on_before_buy			(pItem);
		}
		pPartner.inv_owner->GiveMoney				(dwTransferMoney, false);
		pThis.inv_owner->GiveMoney					(-dwTransferMoney, false);
	}
	else
	{
		if (pItem)
		{
			pThis.inv_owner->on_before_sell			(pItem);
			pPartner.inv_owner->on_before_buy		(pItem);
		}
		pThis.inv_owner->GiveMoney					(dwTransferMoney, false);
		pPartner.inv_owner->GiveMoney				(-dwTransferMoney, false);
	}

	if (pThis.type == TT_TRADER && !bBuying)
	{
		float cond									= 2.f * (float)!(!xr_strcmp(READ_IF_EXISTS(pSettings, r_string, section, "main_class", "nil"), "magazine") && READ_IF_EXISTS(pSettings, r_bool, section, "can_be_emptyed", TRUE));
		pPartner.inv_owner->GiveObject				(*section, cond);
	}
	else
	{
		CGameObject* O1								= smart_cast<CGameObject*>(pPartner.inv_owner);
		CGameObject* O2								= smart_cast<CGameObject*>(pThis.inv_owner);
	
		if (!bBuying)
			swap									(O1, O2);

		NET_Packet									P;
		O1->u_EventGen								(P,GE_TRADE_SELL,O1->ID());
		P.w_u16										(pItem->object().ID());
		O1->u_EventSend								(P);

		// взять у партнера
		O2->u_EventGen								(P,GE_TRADE_BUY,O2->ID());
		P.w_u16										(pItem->object().ID());
		O2->u_EventSend								(P);
	}

	if (pItem && ((pPartner.type == TT_ACTOR) || (pThis.type == TT_ACTOR)))
	{
		bool bDir									= (pThis.type != TT_ACTOR) && bBuying;
		Actor()->callback							(GameObject::eTradeSellBuyItem)(pItem->object().lua_game_object(), bDir, dwTransferMoney);
	}
}

CInventory& CTrade::GetTradeInv(SInventoryOwner owner)
{
	R_ASSERT(TT_NONE != owner.type);

	//return ((TT_TRADER == owner.type) ? (*owner.inv_owner->m_trade_storage) : (owner.inv_owner->inventory()));
	return owner.inv_owner->inventory();
}

CTrade*	CTrade::GetPartnerTrade()
{
	return pPartner.inv_owner->GetTrade();
}
CInventory*	CTrade::GetPartnerInventory()
{
	return &GetTradeInv(pPartner);
}

CInventoryOwner* CTrade::GetPartner()
{
	return pPartner.inv_owner;
}

u32	CTrade::GetItemPrice(CUICellItem* itm, bool b_buying, bool b_free)
{
	if (b_free)
		return				0;
	
	PIItem pItem			= (PIItem)itm->m_pData;
	shared_str& section		= itm->m_section;
	
	// computing condition factor
	// for "dead" weapon we use 10% from base cost, for "good" weapon we use full base cost
	float					condition_factor = ((pItem) ? pItem->GetCondition() : 1.f) * 0.9f + 0.1f;
	if (!xr_strcmp(READ_IF_EXISTS(pSettings, r_string, section, "main_class", "nil"), "magazine") ||
		(!xr_strcmp(READ_IF_EXISTS(pSettings, r_string, section, "main_class", "nil"), "artefact") && condition_factor < 1.f))
		condition_factor	= 1.f;

	// taking trade factors
	bool					is_actor = (pThis.type == TT_ACTOR) || (pPartner.type == TT_ACTOR);
	bool					buying = (is_actor) ? b_buying : true;
	const CTradeParameters*	pTP = &pThis.inv_owner->trade_parameters();
	const CTradeFactors*	pTF;
	if (buying)
	{
		typedef CTradeParameters::action_buy buy;
		if (pTP->enabled(buy(0), section))
			pTF				= &pTP->factors(buy(0), section);
		else
			return 0;
	}
	else
	{
		typedef CTradeParameters::action_sell sell;
		if (pTP->enabled(sell(0), section))
			pTF				= &pTP->factors(sell(0), section);
		else
			return 0;
	}
	const float				friend_factor = pTF->friend_factor(), enemy_factor = pTF->enemy_factor();

	// computing relation factor
	CHARACTER_GOODWILL		attitude = RELATION_REGISTRY().GetAttitude(pThis.inv_owner, pPartner.inv_owner);
	float					relation_factor = (attitude == NO_GOODWILL ) ? 0.f : (((float)attitude + 1000.f) / 2000.f);
	clamp					(relation_factor, 0.f, 1.f);

	// computing action factor
	float					action_factor;
	if (friend_factor > enemy_factor)
		action_factor		= enemy_factor + (friend_factor - enemy_factor) * (1.f - relation_factor);
	else
		action_factor		= friend_factor + (enemy_factor - friend_factor) * relation_factor;

	clamp					(action_factor, _min(enemy_factor, friend_factor), _max(enemy_factor, friend_factor));

	// total price calculation
	float					cost;
	if (pItem)
		cost				= pItem->Cost();
	else
	{
		float cost_factor	= READ_IF_EXISTS(pSettings, r_float, section, "cost_factor", 1.f);
		if (pSettings->line_exist("costs", *section))
			cost			= pSettings->r_float("costs", *section);
		else if (pSettings->line_exist(section, "cost"))
			cost			= pSettings->r_float(section, "cost");
		else
		{
			if (!xr_strcmp(READ_IF_EXISTS(pSettings, r_string, section, "main_class", "nil"), "ammo") && !xr_strcmp(READ_IF_EXISTS(pSettings, r_string, section, "subclass", "nil"), "box"))
			{
				LPCSTR ammo_section		= pSettings->r_string(section, "ammo_section");
				float uses				= pSettings->r_float(section, "max_uses");
				cost					= READ_IF_EXISTS(pSettings, r_float, ammo_section, "cost", 0.f) * uses;
				cost_factor				= READ_IF_EXISTS(pSettings, r_float, ammo_section, "cost_factor", 1.f);
			}
			else
				cost		= 0.f;
		}
		cost				*= cost_factor;
	}
	float					result = cost * condition_factor * action_factor * CURRENCY_RATE;

	// use some script discounts
	luabind::functor<float>	func;
	if(b_buying)
		R_ASSERT(ai().script_engine().functor("trade_manager.get_buy_discount", func));
	else
		R_ASSERT(ai().script_engine().functor("trade_manager.get_sell_discount", func));
	result					*= func(smart_cast<const CGameObject*>(pThis.inv_owner)->ID());

	// smart rounding
	u32						rounder = 1;
	float					divider = result * 0.05f;
	for (u8 multiplier = 1; (float)rounder <= divider; rounder *= multiplier)
		multiplier			= (multiplier == 5) ? 2 : 5;
	rounder					/= multiplier;
	u32						price = (u32)iCeil(result / (float)rounder) * rounder;

	return					price;
}