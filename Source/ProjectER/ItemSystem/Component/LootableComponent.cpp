// LootableComponent.cpp
#include "ItemSystem/Component/LootableComponent.h"
#include "ItemSystem/Data/BaseItemData.h"
#include "ItemSystem/Component/BaseInventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

ULootableComponent::ULootableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	MaxSlots = 10;
	MinLootCount = 1;
	MaxLootCount = 3;
	bCanLoot = false;
	bDestroyOwnerWhenEmpty = false;
	bDeactivateWhenEmpty = true;
}

void ULootableComponent::BeginPlay()
{
	Super::BeginPlay();

	// 서버에서만 초기화하지 않음 - 명시적 호출 대기
	// InitializeRandomLoot()는 몬스터 사망 시 등 외부에서 호출
}

void ULootableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ULootableComponent, CurrentLootSlots);
	DOREPLIFETIME(ULootableComponent, bCanLoot);
}

// ========================================
// 루팅 초기화
// ========================================

void ULootableComponent::InitializeRandomLoot()
{
	// 서버에서만 실행
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("InitializeRandomLoot: Only call on server!"));
		return;
	}

	// ItemPool이 비어있으면 경고
	if (ItemPool.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("InitializeRandomLoot: ItemPool is empty for %s"), *GetOwner()->GetName());
		return;
	}

	// 슬롯 초기화
	CurrentLootSlots.Empty();
	CurrentLootSlots.SetNum(MaxSlots);

	// 랜덤 개수 결정
	int32 LootCount = FMath::RandRange(MinLootCount, FMath::Min(MaxLootCount, MaxSlots));

	// 아이템 배치
	for (int32 i = 0; i < MaxSlots; ++i)
	{
		if (i < LootCount)
		{
			// 유효한 아이템 배치
			FLootSlotData& Slot = CurrentLootSlots[i];
			Slot.ItemId = FMath::RandRange(0, ItemPool.Num() - 1);
			Slot.Count = 1; // TODO: 랜덤 개수로 확장 가능
		}
		else
		{
			// 빈 슬롯
			CurrentLootSlots[i].ItemId = -1;
			CurrentLootSlots[i].Count = 0;
		}
	}

	bCanLoot = true;

	UE_LOG(LogTemp, Log, TEXT("InitializeRandomLoot: Created %d items for %s"),
		LootCount, *GetOwner()->GetName());
}

void ULootableComponent::InitializeWithItems(const TArray<UBaseItemData*>& Items)
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("InitializeWithItems: Only call on server!"));
		return;
	}

	CurrentLootSlots.Empty();
	CurrentLootSlots.SetNum(MaxSlots);

	int32 ItemCount = FMath::Min(Items.Num(), MaxSlots);

	for (int32 i = 0; i < ItemCount; ++i)
	{
		if (Items[i])
		{
			// ItemPool에서 인덱스 찾기
			int32 PoolIndex = ItemPool.Find(Items[i]);
			if (PoolIndex != INDEX_NONE)
			{
				CurrentLootSlots[i].ItemId = PoolIndex;
				CurrentLootSlots[i].Count = 1;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("InitializeWithItems: Item not found in ItemPool"));
			}
		}
	}

	// 나머지는 빈 슬롯
	for (int32 i = ItemCount; i < MaxSlots; ++i)
	{
		CurrentLootSlots[i].ItemId = -1;
		CurrentLootSlots[i].Count = 0;
	}

	bCanLoot = true;

	UE_LOG(LogTemp, Log, TEXT("InitializeWithItems: Created %d items for %s"),
		ItemCount, *GetOwner()->GetName());
}

void ULootableComponent::ClearLoot()
{
	if (!GetOwner()->HasAuthority())
		return;

	CurrentLootSlots.Empty();
	CurrentLootSlots.SetNum(MaxSlots);

	for (int32 i = 0; i < MaxSlots; ++i)
	{
		CurrentLootSlots[i].ItemId = -1;
		CurrentLootSlots[i].Count = 0;
	}

	bCanLoot = false;
	OnLootChanged.Broadcast();
}

// ========================================
// 루팅 상호작용
// ========================================

void ULootableComponent::BeginLoot(APawn* Looter)
{
	if (!bCanLoot)
	{
		UE_LOG(LogTemp, Warning, TEXT("BeginLoot: Component is not active"));
		return;
	}

	if (!Looter)
	{
		UE_LOG(LogTemp, Warning, TEXT("BeginLoot: Invalid Looter"));
		return;
	}

	// 이 함수는 BasePlayerController에서 Server_BeginLoot로 호출될 것
	// 여기서는 검증만 수행
	UE_LOG(LogTemp, Log, TEXT("BeginLoot: %s is looting %s"),
		*Looter->GetName(), *GetOwner()->GetName());
}

bool ULootableComponent::TakeItem(int32 SlotIndex, APawn* Taker)
{
	// 서버에서만 실행
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("TakeItem: Only call on server!"));
		return false;
	}

	// 검증
	if (!bCanLoot)
	{
		UE_LOG(LogTemp, Warning, TEXT("TakeItem: Component is not active"));
		return false;
	}

	if (!Taker)
	{
		UE_LOG(LogTemp, Warning, TEXT("TakeItem: Invalid Taker"));
		return false;
	}

	if (SlotIndex < 0 || SlotIndex >= CurrentLootSlots.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("TakeItem: Invalid SlotIndex %d"), SlotIndex);
		return false;
	}

	// 빈 슬롯 체크
	if (CurrentLootSlots[SlotIndex].ItemId == -1 || CurrentLootSlots[SlotIndex].Count <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("TakeItem: Empty slot at index %d"), SlotIndex);
		return false;
	}

	// 인벤토리 컴포넌트 찾기
	UBaseInventoryComponent* Inventory = Taker->FindComponentByClass<UBaseInventoryComponent>();
	if (!Inventory)
	{
		UE_LOG(LogTemp, Error, TEXT("TakeItem: No InventoryComponent found on %s"), *Taker->GetName());
		return false;
	}

	// 아이템 데이터 가져오기
	UBaseItemData* ItemData = GetItemData(SlotIndex);
	if (!ItemData)
	{
		UE_LOG(LogTemp, Error, TEXT("TakeItem: Failed to get item data"));
		return false;
	}

	// 인벤토리에 추가 시도
	if (Inventory->AddItem(ItemData))
	{
		UE_LOG(LogTemp, Log, TEXT("TakeItem: %s took %s"),
			*Taker->GetName(), *ItemData->ItemName.ToString());

		// 슬롯 감소
		ReduceSlot(SlotIndex);

		return true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("TakeItem: Failed to add item to inventory (full?)"));
		return false;
	}
}

UBaseItemData* ULootableComponent::GetItemData(int32 SlotIndex) const
{
	// 슬롯 인덱스 범위 체크
	if (SlotIndex < 0 || SlotIndex >= CurrentLootSlots.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("GetItemData: Invalid SlotIndex %d"), SlotIndex);
		return nullptr;
	}

	int32 ItemPoolIndex = CurrentLootSlots[SlotIndex].ItemId;

	// 빈 슬롯 체크
	if (ItemPoolIndex == -1)
	{
		return nullptr;
	}

	// 아이템 풀 인덱스 범위 체크
	if (ItemPoolIndex < 0 || ItemPoolIndex >= ItemPool.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("GetItemData: Invalid ItemPoolIndex %d"), ItemPoolIndex);
		return nullptr;
	}

	return ItemPool[ItemPoolIndex].Get();
}

bool ULootableComponent::HasLootRemaining() const
{
	for (const FLootSlotData& Slot : CurrentLootSlots)
	{
		if (Slot.ItemId != -1 && Slot.Count > 0)
		{
			return true;
		}
	}
	return false;
}

// ========================================
// 내부 함수
// ========================================

void ULootableComponent::CompactLootSlots()
{
	// 유효한 아이템만 추출
	TArray<FLootSlotData> ValidSlots;
	for (const FLootSlotData& Slot : CurrentLootSlots)
	{
		if (Slot.ItemId != -1 && Slot.Count > 0)
		{
			ValidSlots.Add(Slot);
		}
	}

	// 리스트 재구성
	CurrentLootSlots.Empty();
	CurrentLootSlots.SetNum(MaxSlots);

	// 유효한 아이템을 앞쪽 슬롯에 배치
	for (int32 i = 0; i < ValidSlots.Num(); ++i)
	{
		CurrentLootSlots[i] = ValidSlots[i];
	}

	// 나머지는 빈 슬롯으로 채움
	for (int32 i = ValidSlots.Num(); i < MaxSlots; ++i)
	{
		CurrentLootSlots[i].ItemId = -1;
		CurrentLootSlots[i].Count = 0;
	}

	UE_LOG(LogTemp, Log, TEXT("CompactLootSlots: %d items remaining"), ValidSlots.Num());

	// 모든 아이템이 사라졌는지 확인
	if (ValidSlots.Num() == 0)
	{
		OnLootDepleted.Broadcast();

		if (bDeactivateWhenEmpty)
		{
			bCanLoot = false;
			UE_LOG(LogTemp, Log, TEXT("CompactLootSlots: Deactivated component (no items left)"));
		}

		if (bDestroyOwnerWhenEmpty)
		{
			UE_LOG(LogTemp, Log, TEXT("CompactLootSlots: Destroying owner actor (no items left)"));
			GetOwner()->Destroy();
		}
	}
}

void ULootableComponent::ReduceSlot(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= CurrentLootSlots.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("ReduceSlot: Invalid SlotIndex %d"), SlotIndex);
		return;
	}

	// 개수 감소
	if (CurrentLootSlots[SlotIndex].Count - 1 <= 0)
	{
		// 아이템 완전 소진
		CurrentLootSlots[SlotIndex].ItemId = -1;
		CurrentLootSlots[SlotIndex].Count = 0;
	}
	else
	{
		// 개수만 감소
		--CurrentLootSlots[SlotIndex].Count;
	}

	// 압축 정렬
	CompactLootSlots();

	// 네트워크 업데이트 (리플리케이션 강제)
	GetOwner()->ForceNetUpdate();
}

void ULootableComponent::OnRep_CurrentLootSlots()
{
	// 클라이언트에서 자동 호출됨
	OnLootChanged.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("OnRep_CurrentLootSlots: Loot updated for %s"),
		*GetOwner()->GetName());
}