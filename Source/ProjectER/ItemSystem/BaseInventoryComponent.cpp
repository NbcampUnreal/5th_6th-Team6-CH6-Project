#include "ItemSystem/BaseInventoryComponent.h"
#include "ItemSystem/BaseItemData.h"

UBaseInventoryComponent::UBaseInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    MaxSlots = 20;
}

bool UBaseInventoryComponent::AddItem(UBaseItemData* Item)
{
    if (!Item || InventoryContents.Num() >= MaxSlots) return false;

    InventoryContents.Add(Item);
    OnInventoryUpdated.Broadcast();

    // 화면 디버그 출력
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan,
            FString::Printf(TEXT("가방에 추가됨: %s (현재 %d개)"), *Item->ItemName.ToString(), InventoryContents.Num()));
    }

    return true;
}

void UBaseInventoryComponent::DebugPrintInventory()
{
    UE_LOG(LogTemp, Warning, TEXT("=== Inventory Check ==="));
    for (const UBaseItemData* Item : InventoryContents)
    {
        if (Item)
        {
            UE_LOG(LogTemp, Warning, TEXT("Item: %s"), *Item->ItemName.ToString());
        }
    }
}