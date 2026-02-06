#include "ItemSystem/BaseBoxActor.h"
#include "ItemSystem/BaseItemData.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "BaseInventoryComponent.h"
#include "ItemSystem/W_LootingPopup.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameplayAbilitySpec.h"

ABaseBoxActor::ABaseBoxActor()
{
	BoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
	SetRootComponent(BoxMesh);
	BoxMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// 멀티플레이어 설정
	bReplicates = true;
}

void ABaseBoxActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaseBoxActor, CurrentItemList);
}

void ABaseBoxActor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		CurrentItemList.Empty();
		CurrentItemList.SetNum(10);
		if (ItemPool.Num() > 0)
		{
			int32 LootCount = FMath::RandRange(MinLootCount, MaxLootCount);
			for (int32 i = 0; i < 10; ++i)
			{
				if (i < LootCount)
				{
					FLootSlot& Slot = CurrentItemList[i];
					Slot.ItemId = FMath::RandRange(0, ItemPool.Num() - 1);
					Slot.Count = 1;
					
				}
				else
				{
					// 범위 안이 아니면 넣는 아이템 풀 인덱스는 -1
					FLootSlot& Slot = CurrentItemList[i];
					Slot.ItemId = -1;
					Slot.Count = 0;
				}

			}
		}
	}
}

// 상호작용(우클릭) 처리
void ABaseBoxActor::PickupItem(APawn* InHandler)
{

}


void ABaseBoxActor::OnRep_GetCurrentItemList()
{
	OnLootChanged.Broadcast();
}

void ABaseBoxActor::ReduceItem(int32 SlotIndex)
{
	if (CurrentItemList[SlotIndex].Count - 1 <= 0)
	{
		CurrentItemList[SlotIndex].ItemId = -1;
		CurrentItemList[SlotIndex].Count = 0;
		ForceNetUpdate();
	}
	else
	{
		--CurrentItemList[SlotIndex].Count;
		ForceNetUpdate();
	}
}

UBaseItemData* ABaseBoxActor::GetItemData(int32 SlotIndex) const
{
	int32 ItemPoolIdex = CurrentItemList[SlotIndex].ItemId;

	return ItemPool[ItemPoolIdex].Get();
}

