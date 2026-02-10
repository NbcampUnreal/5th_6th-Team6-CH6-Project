#include "ItemSystem/Actor/BaseBoxActor.h"
#include "ItemSystem/Data/BaseItemData.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "ItemSystem/Component/BaseInventoryComponent.h"
#include "ItemSystem/UI/W_LootingPopup.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameplayAbilitySpec.h"
#include "CharacterSystem/Player/BasePlayerController.h"


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

void ABaseBoxActor::PickupItem(APawn* InHandler)
{
	if (!InHandler)
	{
		UE_LOG(LogTemp, Warning, TEXT("PickupItem: Invalid InHandler"));
		return;
	}

	// 박스는 직접 습득이 아니라 UI를 통한 상호작용이므로
	// PlayerController를 통해 Server_BeginLoot() 호출
	ABasePlayerController* PC = Cast<ABasePlayerController>(InHandler->GetController());
	if (PC)
	{
		PC->Server_BeginLoot(this);
		UE_LOG(LogTemp, Log, TEXT("PickupItem: Opening box via %s"), *InHandler->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PickupItem: No BasePlayerController found"));
	}
}


void ABaseBoxActor::OnRep_GetCurrentItemList()
{
	OnLootChanged.Broadcast();
}

void ABaseBoxActor::ReduceItem(int32 SlotIndex)
{
	// 범위 체크
	if (SlotIndex < 0 || SlotIndex >= CurrentItemList.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("ReduceItem: Invalid SlotIndex %d"), SlotIndex);
		return;
	}

	// 개수 감소
	if (CurrentItemList[SlotIndex].Count - 1 <= 0)
	{
		// 아이템이 완전히 소진됨 - 빈 슬롯으로 만듦
		CurrentItemList[SlotIndex].ItemId = -1;
		CurrentItemList[SlotIndex].Count = 0;
	}
	else
	{
		// 개수만 감소
		--CurrentItemList[SlotIndex].Count;
	}

	// 아이템 정렬
	CompactItemList();

	// 네트워크 업데이트
	ForceNetUpdate();
}

void ABaseBoxActor::CompactItemList()
{
	// 유효한 아이템만 추출
	TArray<FLootSlot> ValidItems;
	for (const FLootSlot& Slot : CurrentItemList)
	{
		if (Slot.ItemId != -1 && Slot.Count > 0)
		{
			ValidItems.Add(Slot);
		}
	}

	// 리스트 재구성
	CurrentItemList.Empty();
	CurrentItemList.SetNum(10);

	// 유효한 아이템 배치
	for (int32 i = 0; i < ValidItems.Num(); ++i)
	{
		CurrentItemList[i] = ValidItems[i];
	}

	// 나머지는 빈 슬롯으로 채움
	for (int32 i = ValidItems.Num(); i < 10; ++i)
	{
		CurrentItemList[i].ItemId = -1;
		CurrentItemList[i].Count = 0;
	}

	UE_LOG(LogTemp, Log, TEXT("CompactItemList: %d items remaining"), ValidItems.Num());
}

UBaseItemData* ABaseBoxActor::GetItemData(int32 SlotIndex) const
{
	// 슬롯 인덱스 범위 체크
	if (SlotIndex < 0 || SlotIndex >= CurrentItemList.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("GetItemData: Invalid SlotIndex %d (Max: %d)"), SlotIndex, CurrentItemList.Num() - 1);
		return nullptr;
	}

	int32 ItemPoolIndex = CurrentItemList[SlotIndex].ItemId;

	// 빈 슬롯 체크
	if (ItemPoolIndex == -1)
	{
		UE_LOG(LogTemp, Warning, TEXT("GetItemData: Empty slot at index %d"), SlotIndex);
		return nullptr;
	}

	// 아이템 풀 인덱스 범위 체크
	if (ItemPoolIndex < 0 || ItemPoolIndex >= ItemPool.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("GetItemData: Invalid ItemPoolIndex %d (Max: %d)"), ItemPoolIndex, ItemPool.Num() - 1);
		return nullptr;
	}

	return ItemPool[ItemPoolIndex].Get();
}
