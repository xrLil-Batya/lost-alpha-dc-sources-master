#include "stdafx.h"
#include "UIInventoryWnd.h"
#include "../level.h"
#include "../actor.h"
#include "../ActorCondition.h"
#include "../hudmanager.h"
#include "../inventory.h"
#include "UIInventoryUtilities.h"

#include "UICellItem.h"
#include "UICellItemFactory.h"
#include "UIDragDropListEx.h"
#include "UI3tButton.h"
#include "../customoutfit.h"

CUICellItem* CUIInventoryWnd::CurrentItem()
{
	return m_pCurrentCellItem;
}

PIItem CUIInventoryWnd::CurrentIItem()
{
	return	(m_pCurrentCellItem)?(PIItem)m_pCurrentCellItem->m_pData : NULL;
}

void CUIInventoryWnd::SetCurrentItem(CUICellItem* itm)
{
	if(m_pCurrentCellItem == itm) return;

	m_pCurrentCellItem				= itm;

	UIItemInfo.InitItem			(CurrentIItem());
}

void CUIInventoryWnd::SendMessage(CUIWindow *pWnd, s16 msg, void *pData)
{
	if(pWnd == &UIPropertiesBox &&	msg==PROPERTY_CLICKED)
	{
		ProcessPropertiesBoxClicked	();
	}else 
	if (UIExitButton == pWnd && BUTTON_CLICKED == msg)
	{
		HideDialog();
	}

	CUIWindow::SendMessage(pWnd, msg, pData);
}


void CUIInventoryWnd::InitInventory_delayed()
{
	m_b_need_reinit = true;
}

void CUIInventoryWnd::InitInventory() 
{
	CInventoryOwner *pInvOwner	= smart_cast<CInventoryOwner*>(Level().CurrentEntity());
	if(!pInvOwner)				return;

	m_pInv						= &pInvOwner->inventory();

	UIPropertiesBox.Hide		();
	ClearAllLists				();
	m_pMouseCapturer			= NULL;
	SetCurrentItem				(NULL);

	//Slots
	PIItem _itm							= m_pInv->m_slots[PISTOL_SLOT].m_pIItem;
	if(_itm)
	{
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIPistolList->SetItem		(itm);
	}


	_itm								= m_pInv->m_slots[RIFLE_SLOT].m_pIItem;
	if(_itm)
	{
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIAutomaticList->SetItem		(itm);
	}

	_itm								= m_pInv->m_slots[KNIFE_SLOT].m_pIItem;
	if(_itm)
	{
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIKnifeList->SetItem		(itm);
	}

	_itm								= m_pInv->m_slots[APPARATUS_SLOT].m_pIItem;
	if(_itm)
	{
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIBinocularList->SetItem		(itm);
	}
	_itm								= m_pInv->m_slots[TORCH_SLOT].m_pIItem;
	if(_itm)
	{
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUITorchList->SetItem		(itm);
	}

	_itm							= m_pInv->m_slots[DETECTOR_SLOT].m_pIItem;
	if(_itm)
	{
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIDetectorList->SetItem		(itm);
	}

	_itm							= m_pInv->m_slots[HELMET_SLOT].m_pIItem;
	if(_itm)
	{
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIHelmetList->SetItem		(itm);
	}

	_itm							= m_pInv->m_slots[PNV_SLOT].m_pIItem;
	if(_itm)
	{
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIPNVList->SetItem		(itm);
	}

	_itm							= m_pInv->m_slots[ANOM_DET_SLOT].m_pIItem;
	if(_itm)
	{
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIAnomDetectorList->SetItem		(itm);
	}

	_itm							= m_pInv->m_slots[OUTFIT_SLOT].m_pIItem;
	if(_itm)
	{
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIOutfitList->SetItem		(itm);
	}

	//PIItem _outfit						= m_pInv->m_slots[OUTFIT_SLOT].m_pIItem;
	//CUICellItem* outfit					= (_outfit)?create_cell_item(_outfit):NULL;
	//m_pUIOutfitList->SetItem			(outfit);

	TIItemContainer::iterator it, it_e;
	for(it=m_pInv->m_belt.begin(),it_e=m_pInv->m_belt.end(); it!=it_e; ++it) 
	{
		CUICellItem* itm			= create_cell_item(*it);
		m_pUIBeltList->SetItem		(itm);
	}


	
	ruck_list		= m_pInv->m_ruck;
	std::sort		(ruck_list.begin(),ruck_list.end(),InventoryUtilities::GreaterRoomInRuck);

	int i=1;
	for(it=ruck_list.begin(),it_e=ruck_list.end(); it!=it_e; ++it,++i) 
	{
		CUICellItem* itm			= create_cell_item(*it);
		m_pUIBagList->SetItem		(itm);
	}
	//fake
	_itm								= m_pInv->m_slots[GRENADE_SLOT].m_pIItem;
	if(_itm)
	{
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIBagList->SetItem			(itm);
	}

	//InventoryUtilities::UpdateWeight					(UIBagWnd, true);

	m_b_need_reinit					= false;
}  

void CUIInventoryWnd::DropCurrentItem(bool b_all)
{

	CActor *pActor			= smart_cast<CActor*>(Level().CurrentEntity());
	if(!pActor)				return;

	if(!b_all && CurrentIItem() && !CurrentIItem()->IsQuestItem())
	{
		SendEvent_Item_Drop		(CurrentIItem());
		SetCurrentItem			(NULL);
		//InventoryUtilities::UpdateWeight			(UIBagWnd, true);
		return;
	}

	if(b_all && CurrentIItem() && !CurrentIItem()->IsQuestItem())
	{
		u32 cnt = CurrentItem()->ChildsCount();

		for(u32 i=0; i<cnt; ++i)
		{
			CUICellItem*	itm				= CurrentItem()->PopChild(NULL);
			PIItem			iitm			= (PIItem)itm->m_pData;
			SendEvent_Item_Drop				(iitm);
		}

		SendEvent_Item_Drop					(CurrentIItem());
		SetCurrentItem						(NULL);
		//InventoryUtilities::UpdateWeight	(UIBagWnd, true);
		return;
	}
}

//------------------------------------------

bool CUIInventoryWnd::ToSlot(CUICellItem* itm, bool force_place)
{
	CUIDragDropListEx*	old_owner			= itm->OwnerList();
	PIItem	iitem							= (PIItem)itm->m_pData;
	u32 _slot								= iitem->GetSlot();

	if(GetInventory()->CanPutInSlot(iitem)){
		CUIDragDropListEx* new_owner		= GetSlotList(_slot);
		if(_slot==GRENADE_SLOT && !new_owner )return true; //fake, sorry (((

		//tatarinrafa: block putting pnv or helmet in slot if outfit does not allow that
		if (_slot == HELMET_SLOT || _slot == PNV_SLOT){

			PIItem	itemformoutfitslot = GetInventory()->ItemFromSlot(OUTFIT_SLOT);
			if (itemformoutfitslot)
			{
				CCustomOutfit* outfit = smart_cast<CCustomOutfit*>(itemformoutfitslot);//�� ������ ������ �������� ���� ��� ����� � ����� �����, � ��..
				if (outfit)
				{

					if (iitem->GetSlot() == HELMET_SLOT && outfit->block_helmet_slot == 1)
						return false;

					if (iitem->GetSlot() == PNV_SLOT && outfit->block_pnv_slot == 1)
						return false;
				}
			}
		}

		CCustomOutfit* outfit = smart_cast<CCustomOutfit*>(iitem);
		if (outfit){

			if (outfit->block_pnv_slot == 1){
				CInventoryItem* pnv = GetInventory()->ItemFromSlot(PNV_SLOT);
				if (pnv){

					CUICellItem* slot_cell = GetSlotList(PNV_SLOT)->GetItemIdx(0);
					ToBag(slot_cell, false);
				}
			}

			if (outfit->block_helmet_slot == 1){
				CInventoryItem* helmet = GetInventory()->ItemFromSlot(HELMET_SLOT);
				if (helmet){

					CUICellItem* slot_cell = GetSlotList(HELMET_SLOT)->GetItemIdx(0);
					ToBag(slot_cell, false);
				}
			}
		}
		//end tatarinrafa: block putting pnv or helmet in slot if outfit does not allow that

		bool result							= GetInventory()->Slot(iitem);
		VERIFY								(result);

		CUICellItem* i						= old_owner->RemoveItem(itm, (old_owner==new_owner) );
		if (!new_owner) return false;
		else new_owner->SetItem					(i);
		SendEvent_Item2Slot					(iitem);

		SendEvent_ActivateSlot				(iitem);
		
		return								true;
	}else
	{ // in case slot is busy
		if(!force_place ||  _slot==NO_ACTIVE_SLOT || GetInventory()->m_slots[_slot].m_bPersistent) return false;


		PIItem	_iitem						= GetInventory()->m_slots[_slot].m_pIItem;
		CUIDragDropListEx* slot_list		= GetSlotList(_slot);
		VERIFY								(slot_list->ItemsCount()==1);

		CUICellItem* slot_cell				= slot_list->GetItemIdx(0);
		VERIFY								(slot_cell && ((PIItem)slot_cell->m_pData)==_iitem);

		bool result							= ToBag(slot_cell, false);
		VERIFY								(result);

		return ToSlot						(itm, false);
	}
}

bool CUIInventoryWnd::ToBag(CUICellItem* itm, bool b_use_cursor_pos)
{
	PIItem	iitem						= (PIItem)itm->m_pData;

	if(GetInventory()->CanPutInRuck(iitem))
	{
		CUIDragDropListEx*	old_owner		= itm->OwnerList();
		CUIDragDropListEx*	new_owner		= NULL;
		if(b_use_cursor_pos){
				new_owner					= CUIDragDropListEx::m_drag_item->BackList();
				VERIFY						(new_owner==m_pUIBagList);
		}else
				new_owner					= m_pUIBagList;


		bool result							= GetInventory()->Ruck(iitem);
		VERIFY								(result);
		CUICellItem* i						= old_owner->RemoveItem(itm, (old_owner==new_owner) );
		
		if(b_use_cursor_pos)
			new_owner->SetItem				(i,old_owner->GetDragItemPosition());
		else
			new_owner->SetItem				(i);

		if (!result) return true;
		SendEvent_Item2Ruck					(iitem);
		return true;
	}
	return false;
}

bool CUIInventoryWnd::ToBelt(CUICellItem* itm, bool b_use_cursor_pos)
{
	PIItem	iitem						= (PIItem)itm->m_pData;

	if(GetInventory()->CanPutInBelt(iitem))
	{
		CUIDragDropListEx*	old_owner		= itm->OwnerList();
		CUIDragDropListEx*	new_owner		= NULL;
		if(b_use_cursor_pos){
				new_owner					= CUIDragDropListEx::m_drag_item->BackList();
				VERIFY						(new_owner==m_pUIBeltList);
		}else
				new_owner					= m_pUIBeltList;

		bool result							= GetInventory()->Belt(iitem);
		VERIFY								(result);
		CUICellItem* i						= old_owner->RemoveItem(itm, (old_owner==new_owner) );
		
	//.	UIBeltList.RearrangeItems();
		if(b_use_cursor_pos)
			new_owner->SetItem				(i,old_owner->GetDragItemPosition());
		else
			new_owner->SetItem				(i);

		SendEvent_Item2Belt					(iitem);
		return								true;
	}
	return									false;
}

void CUIInventoryWnd::AddItemToBag(PIItem pItem)
{
	CUICellItem* itm						= create_cell_item(pItem);
	m_pUIBagList->SetItem					(itm);
}

bool CUIInventoryWnd::OnItemStartDrag(CUICellItem* itm)
{
	return false; //default behaviour
}

bool CUIInventoryWnd::OnItemSelected(CUICellItem* itm)
{
	if (m_pCurrentCellItem) m_pCurrentCellItem->Mark(false);
	SetCurrentItem		(itm);
	ColorizeAmmo		(itm);
	if (itm) itm->Mark(true);

	return				false;
}

void CUIInventoryWnd::ColorizeAmmo(CUICellItem* itm)
{
	CInventoryItem* inventoryitem = (CInventoryItem*) itm->m_pData;
	if (!inventoryitem) return;

	//clear texture color
	//for bag
	u32 item_count = m_pUIBagList->ItemsCount();
		for (u32 i=0;i<item_count;++i) {
			CUICellItem* bag_item = m_pUIBagList->GetItemIdx(i);
			PIItem invitem = (PIItem) bag_item->m_pData;

			bag_item->SetTextureColor				(0xffffffff);
		}
	//for belt
	u32 belt_item_count = m_pUIBeltList->ItemsCount();
		for (u32 i=0;i<belt_item_count;++i) {
			CUICellItem* belt_item = m_pUIBeltList->GetItemIdx(i);
			PIItem invitem = (PIItem) belt_item->m_pData;

			belt_item->SetTextureColor				(0xffffffff);
		}

	CWeaponMagazined* weapon = smart_cast<CWeaponMagazined*>(inventoryitem);
	if (!weapon) return;

	xr_vector<shared_str> ammo_types = weapon->m_ammoTypes;
	
	u32 color = pSettings->r_color("inventory_color_ammo","color");

	//for bag
	for (size_t id = 0;id<ammo_types.size();++id) {
	u32 item_count = m_pUIBagList->ItemsCount();
		for (u32 i=0;i<item_count;++i) {
			CUICellItem* bag_item = m_pUIBagList->GetItemIdx(i);
			PIItem invitem = (PIItem) bag_item->m_pData;

			if (invitem && xr_strcmp(invitem->object().cNameSect(), ammo_types[id])==0 && invitem->Useful()) {
				bag_item->SetTextureColor				(color);
				break;										//go out from loop, because we can't have 2 CUICellItem's with same section
			}

		}
	}

	//for belt
	for (size_t id = 0;id<ammo_types.size();++id) {
	u32 belt_item_count = m_pUIBeltList->ItemsCount();
		for (u32 i=0;i<belt_item_count;++i) {
			CUICellItem* belt_item = m_pUIBeltList->GetItemIdx(i);
			PIItem invitem = (PIItem) belt_item->m_pData;

			if (invitem && xr_strcmp(invitem->object().cNameSect(), ammo_types[id])==0 && invitem->Useful()) {
				belt_item->SetTextureColor				(color);
			}

		}
	}
}

void CUIInventoryWnd::SumAmmoByDrop(CUICellItem* cell_itm, CUIDragDropListEx* owner)
{
	u32 idx = owner->GetItemIdx				(owner->GetDragItemPosition());
	if (idx==u32(-1)) return;
	if (!(idx<owner->ItemsCount())) return;

	CUICellItem* itemTo = owner->GetItemIdx(idx);

	if (!itemTo) return;

	CWeaponAmmo* ammoCurrent =	 smart_cast<CWeaponAmmo*>((CInventoryItem*)cell_itm->m_pData);
	CWeaponAmmo* ammoTo		 =	 smart_cast<CWeaponAmmo*>((CInventoryItem*)itemTo->m_pData);

	if (!ammoCurrent || !ammoTo) return;

	u16 freeSpaceTo = ammoTo->m_boxSize - ammoTo->m_boxCurr;

	if (freeSpaceTo>=ammoCurrent->m_boxCurr)
	{
		ammoTo->m_boxCurr+=ammoCurrent->m_boxCurr;
		DeleteFromInventory			((CInventoryItem*)cell_itm->m_pData);
	} else {
		ammoCurrent->m_boxCurr-=freeSpaceTo;
		ammoTo->m_boxCurr=ammoTo->m_boxSize;
	}

}

bool CUIInventoryWnd::OnItemDrop(CUICellItem* itm)
{
	CUIDragDropListEx*	old_owner		= itm->OwnerList();
	CUIDragDropListEx*	new_owner		= CUIDragDropListEx::m_drag_item->BackList();

	if(!old_owner || !new_owner)
		return false;
	
	EListType t_new		= GetType(new_owner);
	EListType t_old		= GetType(old_owner);
	
	if (old_owner==new_owner)
	{
		if (t_new==iwBelt)
			SumAmmoByDrop(itm, old_owner);

		return false;
	}

	if(t_new == t_old) return true;

	switch(t_new){
		case iwSlot:{
			if(GetSlotList(CurrentIItem()->GetSlot())==new_owner)
				ToSlot	(itm, true);
		}break;
		case iwBag:{
			ToBag	(itm, true);
		}break;
		case iwBelt:{
			ToBelt	(itm, true);
		}break;
	};

	DropItem				(CurrentIItem(), new_owner);

	return true;
}

bool CUIInventoryWnd::OnItemDbClick(CUICellItem* itm)
{
	if(TryUseItem((PIItem)itm->m_pData))		
		return true;

	CUIDragDropListEx*	old_owner		= itm->OwnerList();
	EListType t_old						= GetType(old_owner);

	switch(t_old){
		case iwSlot:{
			ToBag	(itm, false);
		}break;

		case iwBag:{
			if(!ToSlot(itm, false)){
				if( !ToBelt(itm, false) )
					ToSlot	(itm, true);
			}
		}break;

		case iwBelt:{
			ToBag	(itm, false);
		}break;
	};

	return true;
}


bool CUIInventoryWnd::OnItemRButtonClick(CUICellItem* itm)
{
	SetCurrentItem				(itm);
	ActivatePropertiesBox		();
	return						false;
}

CUIDragDropListEx* CUIInventoryWnd::GetSlotList(u32 slot_idx)
{
	if(slot_idx == NO_ACTIVE_SLOT || GetInventory()->m_slots[slot_idx].m_bPersistent)	return NULL;
	switch (slot_idx)
	{
		case PISTOL_SLOT:
			return m_pUIPistolList;
			break;

		case RIFLE_SLOT:
			return m_pUIAutomaticList;
			break;

		case KNIFE_SLOT:
			return m_pUIKnifeList;
			break;

		case APPARATUS_SLOT:
			return m_pUIBinocularList;
			break;

		case TORCH_SLOT:
			return m_pUITorchList;
			break;

		case OUTFIT_SLOT:
			return m_pUIOutfitList;
			break;

		case DETECTOR_SLOT:
			return m_pUIDetectorList;
			break;

		case HELMET_SLOT:
			return m_pUIHelmetList;
			break;

		case PNV_SLOT:
			return m_pUIPNVList;
			break;

		case ANOM_DET_SLOT:
			return m_pUIAnomDetectorList;
			break;


	};
	return NULL;
}



void CUIInventoryWnd::ClearAllLists()
{
	m_pUIBagList->ClearAll(true);
	m_pUIBeltList->ClearAll(true);
	m_pUIOutfitList->ClearAll(true);
	m_pUIPistolList->ClearAll(true);
	m_pUIAutomaticList->ClearAll(true);
	m_pUIKnifeList->ClearAll(true);
	m_pUIBinocularList->ClearAll(true);
	m_pUITorchList->ClearAll(true);
	m_pUIDetectorList->ClearAll(true);
	m_pUIHelmetList->ClearAll(true);
	m_pUIPNVList->ClearAll(true);
	m_pUIAnomDetectorList->ClearAll(true);
}