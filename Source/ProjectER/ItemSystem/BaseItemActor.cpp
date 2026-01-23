#include "ItemSystem/BaseItemActor.h"
#include "ItemSystem/BaseItemData.h"
#include "ItemSystem/BaseInventoryComponent.h"
#include "Components/StaticMeshComponent.h"

ABaseItemActor::ABaseItemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	RootComponent = ItemMesh;
}

void ABaseItemActor::PickupItem(APawn* InHandler)
{
	if (!InHandler || !ItemData) return;

	// 인벤토리 컴포넌트가 있는지 확인
	UBaseInventoryComponent* Inventory = InHandler->FindComponentByClass<UBaseInventoryComponent>();

	if (Inventory)
	{
		if (Inventory->AddItem(ItemData))
		{
			// 성공 시 아이템 파괴 (획득 처리)
			UE_LOG(LogTemp, Log, TEXT("Item Picked Up: %s"), *ItemData->ItemName.ToString());
			Destroy();
		}
	}
}