#include "ItemSystem/BaseInventoryComponent.h"
#include "ItemSystem/BaseItemData.h"

UBaseInventoryComponent::UBaseInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

//bool UBaseInventoryComponent::AddItem(UBaseItemData* InData)
//{
//	if (!InData || InventoryContents.Num() >= MaxSlots)
//	{
//		return false;
//	}
//
//	InventoryContents.Add(InData);
//
//	if (OnInventoryUpdated.IsBound())
//	{
//		OnInventoryUpdated.Broadcast();
//	}
//
//	return true;
//}

bool UBaseInventoryComponent::AddItem(UBaseItemData* Item)
{
	if (!Item || InventoryContents.Num() >= MaxSlots) return false;

	InventoryContents.Add(Item);
	OnInventoryUpdated.Broadcast();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan,
			FString::Printf(TEXT("가방에 추가됨: %s (현재 %d개)"), *Item->ItemName.ToString(), InventoryContents.Num()));
	}

	return true;
}

void UBaseInventoryComponent::DebugPrintInventory()
{
	UE_LOG(LogTemp, Warning, TEXT("=== 현재 인벤토리 목록 (%d/%d) ==="), InventoryContents.Num(), MaxSlots);

	for (const UBaseItemData* Item : InventoryContents)
	{
		if (Item)
		{
			UE_LOG(LogTemp, Warning, TEXT("- 아이템: %s"), *Item->ItemName.ToString());
		}
	}
}