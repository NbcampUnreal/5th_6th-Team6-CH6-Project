#include "ItemSystem/Component/BaseInventoryComponent.h"
#include "ItemSystem/Data/BaseItemData.h"
#include "ItemSystem/Data/UsableItemData.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AttributeSet.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

UBaseInventoryComponent::UBaseInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	MaxSlots = 20;
	SetIsReplicatedByDefault(true);
}

int32 UBaseInventoryComponent::GetInventoryCount() const
{
	int32 Count = 0;
	for (UBaseItemData* Item : InventoryContents)
	{
		if (Item != nullptr)
		{
			++Count;
		}
	}
	return Count;
}

void UBaseInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize inventory slots with nullptrs on the server
	if (GetOwner()->HasAuthority())
	{
		InventoryContents.Init(nullptr, MaxSlots);
	}
}

void UBaseInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UBaseInventoryComponent, InventoryContents);
}

bool UBaseInventoryComponent::AddItem(UBaseItemData* Item)
{
	if (!Item) return false;

	if (!GetOwner()->HasAuthority())
	{
		Server_AddItem(Item);
		return true;
	}

	// 빈 슬롯 찾기
	int32 EmptySlotIndex = INDEX_NONE;
	for (int32 i = 0; i < InventoryContents.Num(); ++i)
	{
		if (InventoryContents[i] == nullptr)
		{
			EmptySlotIndex = i;
			break;
		}
	}

	// 가방이 가득 찼으면 실패
	if (EmptySlotIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] Inventory is full. Cannot add item: %s"), *Item->ItemName.ToString());
		return false;
	}

	InventoryContents[EmptySlotIndex] = Item;

	if (GEngine)
	{
		FString DebugMsg = FString::Printf(TEXT("가방에 추가됨: %s (슬롯 %d)"),
			*Item->ItemName.ToString(), EmptySlotIndex);
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, DebugMsg);
	}

	UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] Broadcasting OnInventoryUpdated (AddItem)"));
	OnInventoryUpdated.Broadcast();
	return true;
}

bool UBaseInventoryComponent::Server_AddItem_Validate(UBaseItemData* InData) { return true; }

void UBaseInventoryComponent::Server_AddItem_Implementation(UBaseItemData* InData)
{
	AddItem(InData);
}

void UBaseInventoryComponent::OnRep_InventoryContents()
{
	UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] Broadcasting OnInventoryUpdated (OnRep)"));
	OnInventoryUpdated.Broadcast();
}
// 특정 슬롯의 아이템 가져오기
UBaseItemData* UBaseInventoryComponent::GetItemAt(int32 SlotIndex) const
{
	if (SlotIndex >= 0 && SlotIndex < InventoryContents.Num())
	{
		return InventoryContents[SlotIndex];
	}
	return nullptr;
}

// 아이템 사용
void UBaseInventoryComponent::UseItem(int32 SlotIndex)
{
	// 서버에서만 실행
	if (!GetOwner()->HasAuthority())
	{
		Server_UseItem(SlotIndex);
		return;
	}

	// 유효성 검사
	if (SlotIndex < 0 || SlotIndex >= InventoryContents.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] UseItem: Invalid SlotIndex %d (Size: %d)"), SlotIndex, InventoryContents.Num());
		return;
	}

	UBaseItemData* ItemData = InventoryContents[SlotIndex];
	if (!ItemData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] UseItem: Slot %d is empty"), SlotIndex);
		return;
	}

	// UsableItemData로 캐스트
	UUsableItemData* UsableItem = Cast<UUsableItemData>(ItemData);
	if (!UsableItem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] UseItem: Item '%s' is not usable"),
			*ItemData->ItemName.ToString());
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] Using item: %s (Slot %d)"),
		*UsableItem->ItemName.ToString(), SlotIndex);

	// 아이템 효과 적용 (실패 시 소비하지 않음)
	const bool bEffectApplied = ApplyItemEffect(UsableItem);
	if (!bEffectApplied)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] UseItem: Failed to apply effect for item %s"), *UsableItem->ItemName.ToString());
		return;
	}

	// 소비 아이템이면 제거
	if (UsableItem->bConsumable)
	{
		InventoryContents[SlotIndex] = nullptr;

		UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] Item consumed and removed from slot %d"), SlotIndex);

		// ✅ UI 갱신을 위해 OnRep 호출
		OnRep_InventoryContents();
	}
}

bool UBaseInventoryComponent::Server_UseItem_Validate(int32 SlotIndex)
{
	return SlotIndex >= 0 && SlotIndex < MaxSlots;
}

void UBaseInventoryComponent::Server_UseItem_Implementation(int32 SlotIndex)
{
	UseItem(SlotIndex);
}

UAbilitySystemComponent* UBaseInventoryComponent::ResolveOwnerAbilitySystemComponent() const
{
	AActor* const OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] ResolveOwnerAbilitySystemComponent: Owner is null"));
		return nullptr;
	}

	if (UAbilitySystemComponent* const OwnerAsc = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor))
	{
		return OwnerAsc;
	}

	const APawn* const OwnerPawn = Cast<APawn>(OwnerActor);
	if (OwnerPawn == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] ResolveOwnerAbilitySystemComponent: No ASC on owner '%s'"), *OwnerActor->GetName());
		return nullptr;
	}

	const APlayerState* const PlayerState = OwnerPawn->GetPlayerState();
	if (PlayerState == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] ResolveOwnerAbilitySystemComponent: Pawn '%s' has no PlayerState"), *OwnerActor->GetName());
		return nullptr;
	}

	if (const IAbilitySystemInterface* const AscInterface = Cast<IAbilitySystemInterface>(PlayerState))
	{
		if (UAbilitySystemComponent* const PlayerStateAsc = AscInterface->GetAbilitySystemComponent())
		{
			return PlayerStateAsc;
		}
	}

	UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] ResolveOwnerAbilitySystemComponent: PlayerState '%s' has no ASC"), *PlayerState->GetName());
	return nullptr;
}

// 아이템 효과 적용
bool UBaseInventoryComponent::ApplyItemEffect(UUsableItemData* ItemData)
{
	if (!ItemData)
	{
		return false;
	}

	UAbilitySystemComponent* const ASC = ResolveOwnerAbilitySystemComponent();
	if (ASC == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] ApplyItemEffect: Failed to resolve ASC for owner '%s'"), *GetNameSafe(GetOwner()));
		return false;
	}

	switch (ItemData->EffectType)
	{
	case EItemEffectType::IncreaseStat:
		return ApplyStatIncrease(ASC, ItemData);

	default:
		UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] Unknown effect type"));
		return false;
	}
}

// 스탯 증가 적용
bool UBaseInventoryComponent::ApplyStatIncrease(UAbilitySystemComponent* ASC, UUsableItemData* ItemData)
{
	if (!ASC || !ItemData)
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] ASC or ItemData is null!"));
		return false;
	}

	// BaseAttributeSet 가져오기
	const UBaseAttributeSet* BaseAS = Cast<UBaseAttributeSet>(ASC->GetAttributeSet(UBaseAttributeSet::StaticClass()));
	if (!BaseAS)
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] No BaseAttributeSet found!"));
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] BaseAttributeSet found!"));

	// Attribute 가져오기
	FGameplayAttribute Attribute;
	FString AttributeName;

	switch (ItemData->StatType)
	{
	case EItemStatType::AttackPower:
		Attribute = BaseAS->GetAttackPowerAttribute();
		AttributeName = TEXT("AttackPower");
		break;
	case EItemStatType::Defense:
		Attribute = BaseAS->GetDefenseAttribute();
		AttributeName = TEXT("Defense");
		break;
	case EItemStatType::AttackSpeed:
		Attribute = BaseAS->GetAttackSpeedAttribute();
		AttributeName = TEXT("AttackSpeed");
		break;
	case EItemStatType::MoveSpeed:
		Attribute = BaseAS->GetMoveSpeedAttribute();
		AttributeName = TEXT("MoveSpeed");
		break;
	default:
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] Unknown stat type"));
		return false;
	}

	if (!Attribute.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] Invalid attribute '%s'!"), *AttributeName);
		return false;
	}

	// 현재 값 가져오기
	float CurrentValue = ASC->GetNumericAttribute(Attribute);
	float NewValue = CurrentValue + ItemData->EffectValue;

	UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] Increasing %s: %.1f + %.1f = %.1f"),
		*AttributeName, CurrentValue, ItemData->EffectValue, NewValue);

	// Attribute 값 증가
	ASC->SetNumericAttributeBase(Attribute, NewValue);

	// 확인
	const float VerifyValue = ASC->GetNumericAttribute(Attribute);
	UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] Stat increased successfully! Verified: %.1f"), VerifyValue);
	return true;
}