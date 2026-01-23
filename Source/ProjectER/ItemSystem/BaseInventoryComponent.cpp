#include "ItemSystem/BaseInventoryComponent.h"
#include "ItemSystem/BaseItemData.h"

UBaseInventoryComponent::UBaseInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UBaseInventoryComponent::AddItem(UBaseItemData* InData)
{
	if (!InData || InventoryContents.Num() >= MaxSlots)
	{
		return false;
	}

	InventoryContents.Add(InData);

	if (OnInventoryUpdated.IsBound())
	{
		OnInventoryUpdated.Broadcast();
	}

	return true;
}