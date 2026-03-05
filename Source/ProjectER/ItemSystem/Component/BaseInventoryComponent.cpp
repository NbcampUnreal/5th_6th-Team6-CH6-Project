#include "ItemSystem/Component/BaseInventoryComponent.h"
#include "ItemSystem/Data/BaseItemData.h"
#include "ItemSystem/Data/UsableItemData.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"
#include "Net/UnrealNetwork.h"

UBaseInventoryComponent::UBaseInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	MaxSlots = 20;
	SetIsReplicatedByDefault(true);
}

void UBaseInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UBaseInventoryComponent, InventoryContents);
}

bool UBaseInventoryComponent::AddItem(UBaseItemData* Item)
{
	if (!Item || InventoryContents.Num() >= MaxSlots) return false;

	if (!GetOwner()->HasAuthority())
	{
		Server_AddItem(Item);
		return true;
	}

	InventoryContents.Add(Item);

	if (GEngine)
	{
		FString DebugMsg = FString::Printf(TEXT("가방에 추가됨: %s (현재 %d개)"),
			*Item->ItemName.ToString(), InventoryContents.Num());
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

	// 아이템 효과 적용
	ApplyItemEffect(UsableItem);

	// 소비 아이템이면 제거
	if (UsableItem->bConsumable)
	{
		InventoryContents.RemoveAt(SlotIndex);

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

// 아이템 효과 적용
void UBaseInventoryComponent::ApplyItemEffect(UUsableItemData* ItemData)
{
	if (!ItemData)
		return;

	// Owner의 AbilitySystemComponent 가져오기
	AActor* Owner = GetOwner();
	if (!Owner)
		return;

	UAbilitySystemComponent* ASC = Owner->FindComponentByClass<UAbilitySystemComponent>();
	if (!ASC)
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] ApplyItemEffect: No ASC found!"));
		return;
	}

	switch (ItemData->EffectType)
	{
	case EItemEffectType::IncreaseStat:
		ApplyStatIncrease(ASC, ItemData);
		break;

	default:
		UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] Unknown effect type"));
		break;
	}
}

// 스탯 증가 적용
void UBaseInventoryComponent::ApplyStatIncrease(UAbilitySystemComponent* ASC, UUsableItemData* ItemData)
{
	if (!ASC || !ItemData)
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] ASC or ItemData is null!"));
		return;
	}

	// BaseAttributeSet 가져오기
	const UBaseAttributeSet* BaseAS = Cast<UBaseAttributeSet>(ASC->GetAttributeSet(UBaseAttributeSet::StaticClass()));
	if (!BaseAS)
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] No BaseAttributeSet found!"));
		return;
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
		return;
	}

	if (!Attribute.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] Invalid attribute '%s'!"), *AttributeName);
		return;
	}

	// 현재 값 가져오기
	float CurrentValue = ASC->GetNumericAttribute(Attribute);
	float NewValue = CurrentValue + ItemData->EffectValue;

	UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] Increasing %s: %.1f + %.1f = %.1f"),
		*AttributeName, CurrentValue, ItemData->EffectValue, NewValue);

	// Attribute 값 증가
	ASC->SetNumericAttributeBase(Attribute, NewValue);

	// 확인
	float VerifyValue = ASC->GetNumericAttribute(Attribute);
	UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] Stat increased successfully! Verified: %.1f"), VerifyValue);
}