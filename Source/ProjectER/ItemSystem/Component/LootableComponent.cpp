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
	bAutoInitialize = true; // 클라이언트 테스트를 위해 기본값 true
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
	if (!CurrentItemList.IsValidIndex(SlotIndex)) return nullptr;

	int32 ItemPoolIndex = CurrentItemList[SlotIndex].ItemId;

	// -1은 빈 슬롯, 0 이상은 유효한 아이템
	if (ItemPoolIndex < 0 || ItemPoolIndex >= ItemPool.Num())
	{
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
		// 아이템 완전 소진 - 빈 슬롯은 -1
		CurrentItemList[SlotIndex].ItemId = -1;
		CurrentItemList[SlotIndex].Count = 0;
	}
	else
	{
		// 개수만 감소
		--CurrentItemList[SlotIndex].Count;
	}

	CompactItemList();

	// 서버 측 UI 갱신을 위해 브로드캐스트
	OnLootChanged.Broadcast();
	GetOwner()->ForceNetUpdate();
}

// ========================================
// 루팅 초기화
// ========================================

void ULootableComponent::InitializeRandomLoot()
{
	if (!GetOwner()->HasAuthority()) return;

	// [체크] 에디터에서 ItemPool에 아이템을 넣었는지 반드시 확인하세요!
	if (ItemPool.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] ItemPool is EMPTY! 아이템을 생성할 수 없습니다."), *GetOwner()->GetName());
		return;
	}

	CurrentItemList.Empty();
	CurrentItemList.SetNum(MaxSlots);

	// 최소~최대 개수 결정
	int32 LootCount = FMath::RandRange(MinLootCount, FMath::Min(MaxLootCount, MaxSlots));

	for (int32 i = 0; i < MaxSlots; ++i)
	{
		if (i < LootCount)
		{
			CurrentItemList[i].ItemId = FMath::RandRange(0, ItemPool.Num() - 1);
			CurrentItemList[i].Count = 1;
		}
		else
		{
			CurrentItemList[i].ItemId = -1; // 빈 슬롯 명시
			CurrentItemList[i].Count = 0;
		}
	}

	// 서버 UI 갱신 및 복제 강제
	OnLootChanged.Broadcast();
	GetOwner()->ForceNetUpdate();
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
				CurrentItemList[i].ItemId = -1;
			}
		}
	}

	// 나머지는 빈 슬롯
	for (int32 i = ItemCount; i < MaxSlots; ++i)
	{
		CurrentItemList[i].ItemId = -1;
		CurrentItemList[i].Count = 0;
	}

	OnLootChanged.Broadcast();

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
		CurrentItemList[i].ItemId = -1;
		CurrentItemList[i].Count = 0;
	}

	OnLootChanged.Broadcast();
}

bool ULootableComponent::HasLootRemaining() const
{
	for (const FLootSlot& Slot : CurrentItemList)
	{
		// 0번 아이템도 유효하므로 >= 0 체크
		if (Slot.ItemId >= 0 && Slot.Count > 0)
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

	// 빈 슬롯 체크 (0번 아이템은 유효하므로 < 0일 때만 빈 슬롯)
	if (CurrentItemList[SlotIndex].ItemId < 0 || CurrentItemList[SlotIndex].Count <= 0)
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
		// 0번 아이템도 유효하므로 >= 0 체크
		if (Slot.ItemId >= 0 && Slot.Count > 0)
		{
			ValidSlots.Add(Slot);
		}
	}

	for (int32 i = 0; i < MaxSlots; ++i)
	{
		if (i < ValidSlots.Num())
		{
			CurrentItemList[i] = ValidSlots[i];
		}
		else
		{
			CurrentItemList[i].ItemId = -1;
			CurrentItemList[i].Count = 0;
		}
	}

	OnLootChanged.Broadcast();
}

void ULootableComponent::OnRep_CurrentItemList()
{
	// 클라이언트에서 복제된 데이터를 받았을 때 UI 갱신 알림
	if (OnLootChanged.IsBound())
	{
		OnLootChanged.Broadcast();
	}
	UE_LOG(LogTemp, Log, TEXT("[LootableComponent] OnRep_CurrentItemList: Loot updated for %s"),
		*GetOwner()->GetName());
}