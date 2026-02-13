// LootableComponent.cpp
// 위치: Source/ProjectER/ItemSystem/Component/LootableComponent.cpp
#include "ItemSystem/Component/LootableComponent.h"
#include "ItemSystem/Data/BaseItemData.h"
#include "ItemSystem/Component/BaseInventoryComponent.h"
#include "CharacterSystem/Player/BasePlayerController.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

ULootableComponent::ULootableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	MaxSlots = 10;
	MinLootCount = 1;
	MaxLootCount = 3;
	bAutoInitialize = false;
	bDestroyOwnerWhenEmpty = false;
}

void ULootableComponent::BeginPlay()
{
	Super::BeginPlay();

	// 서버에서만 자동 초기화
	if (GetOwner()->HasAuthority() && bAutoInitialize)
	{
		InitializeRandomLoot();
	}
}

void ULootableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ULootableComponent, CurrentItemList);
}

void ULootableComponent::PickupItem()
{
}

// ========================================
// BaseBoxActor 호환 인터페이스
// ========================================

UBaseItemData* ULootableComponent::GetItemData(int32 SlotIndex) const
{
	if (SlotIndex < 0 || SlotIndex >= CurrentItemList.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("[LootableComponent] GetItemData: Invalid SlotIndex %d"), SlotIndex);
		return nullptr;
	}

	int32 ItemPoolIndex = CurrentItemList[SlotIndex].ItemId;

	// 빈 슬롯 체크 (ItemId가 -1이거나 0이면 빈 슬롯)
	if (ItemPoolIndex <= 0)
	{
		return nullptr;
	}

	if (ItemPoolIndex >= ItemPool.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("[LootableComponent] GetItemData: Invalid ItemPoolIndex %d"), ItemPoolIndex);
		return nullptr;
	}

	return ItemPool[ItemPoolIndex].Get();
}

void ULootableComponent::ReduceItem(int32 SlotIndex)
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LootableComponent] ReduceItem: Client called!"));
		return;
	}

	if (SlotIndex < 0 || SlotIndex >= CurrentItemList.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("[LootableComponent] ReduceItem: Invalid SlotIndex %d"), SlotIndex);
		return;
	}

	if (CurrentItemList[SlotIndex].Count - 1 <= 0)
	{
		// 아이템 완전 소진
		CurrentItemList[SlotIndex].ItemId = 0;
		CurrentItemList[SlotIndex].Count = 0;
	}
	else
	{
		// 개수만 감소
		--CurrentItemList[SlotIndex].Count;
	}

	CompactItemList();
	GetOwner()->ForceNetUpdate();
}

// ========================================
// 루팅 초기화
// ========================================

void ULootableComponent::InitializeRandomLoot()
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LootableComponent] InitializeRandomLoot: Only call on server!"));
		return;
	}

	if (ItemPool.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LootableComponent] InitializeRandomLoot: ItemPool is empty for %s"), *GetOwner()->GetName());
		return;
	}

	CurrentItemList.Empty();
	CurrentItemList.SetNum(MaxSlots);

	int32 LootCount = FMath::RandRange(MinLootCount, FMath::Min(MaxLootCount, MaxSlots));

	for (int32 i = 0; i < MaxSlots; ++i)
	{
		if (i < LootCount)
		{
			FLootSlot& Slot = CurrentItemList[i];
			Slot.ItemId = FMath::RandRange(0, ItemPool.Num() - 1);
			Slot.Count = 1;
		}
		else
		{
			// 빈 슬롯
			CurrentItemList[i].ItemId = 0;
			CurrentItemList[i].Count = 0;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[LootableComponent] InitializeRandomLoot: Created %d items for %s"),
		LootCount, *GetOwner()->GetName());
}

void ULootableComponent::InitializeWithItems(const TArray<UBaseItemData*>& Items)
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LootableComponent] InitializeWithItems: Only call on server!"));
		return;
	}

	CurrentItemList.Empty();
	CurrentItemList.SetNum(MaxSlots);

	int32 ItemCount = FMath::Min(Items.Num(), MaxSlots);

	for (int32 i = 0; i < ItemCount; ++i)
	{
		if (Items[i])
		{
			int32 PoolIndex = ItemPool.Find(Items[i]);
			if (PoolIndex != INDEX_NONE)
			{
				CurrentItemList[i].ItemId = PoolIndex;
				CurrentItemList[i].Count = 1;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[LootableComponent] InitializeWithItems: Item not found in ItemPool"));
			}
		}
	}

	// 나머지는 빈 슬롯
	for (int32 i = ItemCount; i < MaxSlots; ++i)
	{
		CurrentItemList[i].ItemId = 0;
		CurrentItemList[i].Count = 0;
	}

	UE_LOG(LogTemp, Log, TEXT("[LootableComponent] InitializeWithItems: Created %d items for %s"),
		ItemCount, *GetOwner()->GetName());
}

void ULootableComponent::ClearLoot()
{
	if (!GetOwner()->HasAuthority())
		return;

	CurrentItemList.Empty();
	CurrentItemList.SetNum(MaxSlots);

	for (int32 i = 0; i < MaxSlots; ++i)
	{
		CurrentItemList[i].ItemId = 0;
		CurrentItemList[i].Count = 0;
	}

	OnLootChanged.Broadcast();
}

bool ULootableComponent::HasLootRemaining() const
{
	for (const FLootSlot& Slot : CurrentItemList)
	{
		if (Slot.ItemId > 0 && Slot.Count > 0)
		{
			return true;
		}
	}
	return false;
}

// ========================================
// 아이템 가져가기
// ========================================

bool ULootableComponent::TakeItem(int32 SlotIndex, APawn* Taker)
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LootableComponent] TakeItem: Only call on server!"));
		return false;
	}

	if (!Taker)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LootableComponent] TakeItem: Invalid Taker"));
		return false;
	}

	if (SlotIndex < 0 || SlotIndex >= CurrentItemList.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("[LootableComponent] TakeItem: Invalid SlotIndex %d"), SlotIndex);
		return false;
	}

	// 빈 슬롯 체크
	if (CurrentItemList[SlotIndex].ItemId <= 0 || CurrentItemList[SlotIndex].Count <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LootableComponent] TakeItem: Empty slot at index %d"), SlotIndex);
		return false;
	}

	UBaseInventoryComponent* Inventory = Taker->FindComponentByClass<UBaseInventoryComponent>();
	if (!Inventory)
	{
		UE_LOG(LogTemp, Error, TEXT("[LootableComponent] TakeItem: No InventoryComponent found on %s"), *Taker->GetName());
		return false;
	}

	UBaseItemData* ItemData = GetItemData(SlotIndex);
	if (!ItemData)
	{
		UE_LOG(LogTemp, Error, TEXT("[LootableComponent] TakeItem: Failed to get item data"));
		return false;
	}

	if (Inventory->AddItem(ItemData))
	{
		UE_LOG(LogTemp, Log, TEXT("[LootableComponent] TakeItem: %s took %s"),
			*Taker->GetName(), *ItemData->ItemName.ToString());

		ReduceItem(SlotIndex);
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[LootableComponent] TakeItem: Failed to add item to inventory (full?)"));
		return false;
	}
}

// ========================================
// 내부 함수
// ========================================

void ULootableComponent::CompactItemList()
{
	TArray<FLootSlot> ValidSlots;
	for (const FLootSlot& Slot : CurrentItemList)
	{
		if (Slot.ItemId > 0 && Slot.Count > 0)
		{
			ValidSlots.Add(Slot);
		}
	}

	CurrentItemList.Empty();
	CurrentItemList.SetNum(MaxSlots);

	for (int32 i = 0; i < ValidSlots.Num(); ++i)
	{
		CurrentItemList[i] = ValidSlots[i];
	}

	for (int32 i = ValidSlots.Num(); i < MaxSlots; ++i)
	{
		CurrentItemList[i].ItemId = 0;
		CurrentItemList[i].Count = 0;
	}

	UE_LOG(LogTemp, Log, TEXT("[LootableComponent] CompactItemList: %d items remaining"), ValidSlots.Num());

	if (ValidSlots.Num() == 0)
	{
		OnLootDepleted.Broadcast();

		if (bDestroyOwnerWhenEmpty)
		{
			UE_LOG(LogTemp, Log, TEXT("[LootableComponent] Destroying owner actor (no items left)"));
			GetOwner()->Destroy();
		}
	}
}

void ULootableComponent::OnRep_CurrentItemList()
{
	OnLootChanged.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("[LootableComponent] OnRep_CurrentItemList: Loot updated for %s"),
		*GetOwner()->GetName());
}