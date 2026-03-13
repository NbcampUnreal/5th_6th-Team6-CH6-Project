// LootableComponent.cpp
// 위치: Source/ProjectER/ItemSystem/Component/LootableComponent.cpp
#include "ItemSystem/Component/LootableComponent.h"
#include "ItemSystem/Data/BaseItemData.h"
#include "ItemSystem/Component/BaseInventoryComponent.h"
#include "CharacterSystem/Player/BasePlayerController.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

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
	DOREPLIFETIME(ULootableComponent, bDestroyOwnerWhenEmpty); // replicate config flag so clients see it
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

	// 모든 아이템이 소진되었는지 확인
	if (!HasLootRemaining())
	{
		// 옵션이 켜져있을 때만 루팅이 모두 끝난 이벤트를 브로드캐스트
		if (bDestroyOwnerWhenEmpty)
		{
			OnLootDepleted.Broadcast();
		}

		if (bDestroyOwnerWhenEmpty && GetOwner()->HasAuthority())
		{
			UE_LOG(LogTemp, Log, TEXT("[LootableComponent] Owner %s destroyed because loot emptied."), *GetOwner()->GetName());

			// RootComponent의 Collision 비활성화
			AActor* Owner = GetOwner();

			// 1. RootComponent가 PrimitiveComponent인 경우 (박스의 경우 StaticMeshComponent)
			if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Owner->GetRootComponent()))
			{
				RootPrim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				UE_LOG(LogTemp, Log, TEXT("[LootableComponent] Disabled collision on RootComponent"));
			}

			// 2. Character인 경우 CapsuleComponent도 확인
			if (ACharacter* Character = Cast<ACharacter>(Owner))
			{
				if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
				{
					Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					UE_LOG(LogTemp, Log, TEXT("[LootableComponent] Disabled collision on CapsuleComponent"));
				}
			}

			Owner->Destroy();
			// Owner is gone, no need to call ForceNetUpdate
			return;
		}
	}

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

	// 비워진 상태가 루팅 완료와 동일하므로 처리
	if (bDestroyOwnerWhenEmpty)
	{
		OnLootDepleted.Broadcast();
		UE_LOG(LogTemp, Log, TEXT("[LootableComponent] Owner %s destroyed because loot cleared."), *GetOwner()->GetName());

		//  RootComponent의 Collision 비활성화
		AActor* Owner = GetOwner();

		// 1. RootComponent가 PrimitiveComponent인 경우
		if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Owner->GetRootComponent()))
		{
			RootPrim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			UE_LOG(LogTemp, Log, TEXT("[LootableComponent] Disabled collision on RootComponent (ClearLoot)"));
		}

		// 2. Character인 경우 CapsuleComponent도 확인
		if (ACharacter* Character = Cast<ACharacter>(Owner))
		{
			if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
			{
				Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				UE_LOG(LogTemp, Log, TEXT("[LootableComponent] Disabled collision on CapsuleComponent (ClearLoot)"));
			}
		}

		Owner->Destroy();
	}
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
		
		//TODO: The loot interaction need to be synced -> need server client communication
		
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
	// 클라이언트에서도 루팅이 모두 끝난 경우 알림
	// 팝업 닫힘은 옵션이 켜져있을 때만 수행
	if (!HasLootRemaining() && bDestroyOwnerWhenEmpty && OnLootDepleted.IsBound())
	{
		OnLootDepleted.Broadcast();
	}

	UE_LOG(LogTemp, Log, TEXT("[LootableComponent] OnRep_CurrentItemList: Loot updated for %s"),
		*GetOwner()->GetName());
}