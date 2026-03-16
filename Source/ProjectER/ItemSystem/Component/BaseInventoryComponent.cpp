// File: 5th_6th-Team6-CH6-Project/Source/ProjectER/ItemSystem/Component/BaseInventoryComponent.cpp

#include "ItemSystem/Component/BaseInventoryComponent.h"
#include "ItemSystem/Data/BaseItemData.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"
#include "CharacterSystem/GameplayTags/GameplayTags.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

UBaseInventoryComponent::UBaseInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	MaxSlots = 20;
	SetIsReplicatedByDefault(true);
}

void UBaseInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize inventory slots with nullptrs on the server only if not already initialized
	if (GetOwner()->HasAuthority() && InventoryContents.Num() == 0)
	{
		InventoryContents.Init(nullptr, MaxSlots);
	}
}

void UBaseInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearFoodHealEffects();
	Super::EndPlay(EndPlayReason);
}

void UBaseInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UBaseInventoryComponent, InventoryContents);
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

bool UBaseInventoryComponent::AddItem(UBaseItemData* Item)
{
	/*if (Item == nullptr || InventoryContents.Num() >= MaxSlots)*/
	if (Item == nullptr)
	{
		return false;
	}

	AActor* const OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return false;
	}

	if (!OwnerActor->HasAuthority())
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

	OnInventoryUpdated.Broadcast();
	return true;
}

bool UBaseInventoryComponent::Server_AddItem_Validate(UBaseItemData* InData)
{
	return InData != nullptr;
}

void UBaseInventoryComponent::Server_AddItem_Implementation(UBaseItemData* InData)
{
	AddItem(InData);
}

void UBaseInventoryComponent::OnRep_InventoryContents()
{
	OnInventoryUpdated.Broadcast();
}

UBaseItemData* UBaseInventoryComponent::GetItemAt(const int32 SlotIndex) const
{
	if (!InventoryContents.IsValidIndex(SlotIndex))
	{
		return nullptr;
	}

	return InventoryContents[SlotIndex];
}

void UBaseInventoryComponent::UseItem(const int32 SlotIndex)
{
	AActor* const OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] UseItem: Owner is null"));
		return;
	}

	if (!OwnerActor->HasAuthority())
	{
		Server_UseItem(SlotIndex);
		return;
	}

	if (!InventoryContents.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] UseItem: Invalid SlotIndex %d"), SlotIndex);
		return;
	}

	UBaseItemData* const ItemData = InventoryContents[SlotIndex];
	if (ItemData == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] UseItem: Slot %d is empty"), SlotIndex);
		return;
	}

	UUsableItemData* const UsableItem = Cast<UUsableItemData>(ItemData);
	if (UsableItem == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] UseItem: Item '%s' is not usable"), *ItemData->ItemName.ToString());
		return;
	}

	if (!CanUseItemsInCurrentLifeState())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] UseItem: blocked while Down/Death. Item='%s'"), *UsableItem->ItemName.ToString());
		return;
	}

	const bool bEffectApplied = ApplyItemEffect(UsableItem);

	if (!bEffectApplied)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] UseItem: Failed to apply effect for item '%s'"), *UsableItem->ItemName.ToString());
		return;
	}

	if (!UsableItem->bConsumable)
	{
		return;
	}

	InventoryContents[SlotIndex] = nullptr;
	OnInventoryUpdated.Broadcast();
}

bool UBaseInventoryComponent::Server_UseItem_Validate(const int32 SlotIndex)
{
	return SlotIndex >= 0 && SlotIndex < MaxSlots;
}

void UBaseInventoryComponent::Server_UseItem_Implementation(const int32 SlotIndex)
{
	UseItem(SlotIndex);
}

UAbilitySystemComponent* UBaseInventoryComponent::ResolveOwnerAbilitySystemComponent() const
{
	AActor* const OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] ResolveASC: Owner is null"));
		return nullptr;
	}

	if (UAbilitySystemComponent* const OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor))
	{
		return OwnerASC;
	}

	const APawn* const OwnerPawn = Cast<APawn>(OwnerActor);
	if (OwnerPawn == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] ResolveASC: Owner '%s' is not Pawn"), *GetNameSafe(OwnerActor));
		return nullptr;
	}

	const APlayerState* const PlayerState = OwnerPawn->GetPlayerState();
	if (PlayerState == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] ResolveASC: Pawn '%s' has no PlayerState"), *GetNameSafe(OwnerPawn));
		return nullptr;
	}

	const IAbilitySystemInterface* const ASCInterface = Cast<IAbilitySystemInterface>(PlayerState);
	if (ASCInterface == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] ResolveASC: PlayerState '%s' has no AbilitySystemInterface"), *GetNameSafe(PlayerState));
		return nullptr;
	}

	UAbilitySystemComponent* const ASC = ASCInterface->GetAbilitySystemComponent();
	if (ASC == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] ResolveASC: PlayerState '%s' ASC is null"), *GetNameSafe(PlayerState));
		return nullptr;
	}

	return ASC;
}

bool UBaseInventoryComponent::CanUseItemsInCurrentLifeState() const
{
	UAbilitySystemComponent* const ASC = ResolveOwnerAbilitySystemComponent();
	if (ASC == nullptr)
	{
		return false;
	}

	const bool bIsDown = ASC->HasMatchingGameplayTag(ProjectER::State::Life::Down);
	const bool bIsDead = ASC->HasMatchingGameplayTag(ProjectER::State::Life::Death);

	return !bIsDown && !bIsDead;
}

FGameplayTag UBaseInventoryComponent::GetSetByCallerTagFromStatType(const EItemStatType StatType) const
{
	switch (StatType)
	{
	case EItemStatType::AttackPower:
		return ProjectER::Status::AttackPower;
	case EItemStatType::Defense:
		return ProjectER::Status::Defense;
	case EItemStatType::AttackSpeed:
		return ProjectER::Status::AttackSpeed;
	case EItemStatType::MoveSpeed:
		return ProjectER::Status::MoveSpeed;
	default:
		return FGameplayTag();
	}
}

bool UBaseInventoryComponent::ApplyItemEffect(UUsableItemData* ItemData)
{
	if (ItemData == nullptr)
	{
		return false;
	}

	UAbilitySystemComponent* const ASC = ResolveOwnerAbilitySystemComponent();
	if (ASC == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] ApplyItemEffect: Failed to resolve ASC"));
		return false;
	}

	switch (ItemData->EffectType)
	{
	case EItemEffectType::IncreaseStat:
		return ApplyStatIncrease(ASC, ItemData);
	case EItemEffectType::HealOverTime:
		return EnqueueFoodHeal(ItemData);
	default:
		UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] ApplyItemEffect: Unknown effect type"));
		return false;
	}
}

bool UBaseInventoryComponent::ApplyStatIncrease(UAbilitySystemComponent* ASC, UUsableItemData* ItemData)
{
	if (ASC == nullptr || ItemData == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] ApplyStatIncrease: ASC or ItemData is null"));
		return false;
	}

	if (ItemData->EffectValue <= 0.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] ApplyStatIncrease: Invalid EffectValue %.2f"), ItemData->EffectValue);
		return false;
	}

	const FGameplayTag StatTag = GetSetByCallerTagFromStatType(ItemData->StatType);
	if (!StatTag.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] ApplyStatIncrease: Invalid StatTag"));
		return false;
	}

	if (ItemData->ItemStatEffectClass.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] ApplyStatIncrease: ItemStatEffectClass is null (%s)"), *GetNameSafe(ItemData));
		return false;
	}

	const TSubclassOf<UGameplayEffect> ItemEffectClass = ItemData->ItemStatEffectClass.LoadSynchronous();
	if (ItemEffectClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] ApplyStatIncrease: Failed to load GE class '%s'"), *ItemData->ItemStatEffectClass.ToString());
		return false;
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(ItemEffectClass, 1.0f, EffectContext);
	if (!SpecHandle.IsValid() || SpecHandle.Data == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] ApplyStatIncrease: Failed to create GE spec"));
		return false;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(StatTag, ItemData->EffectValue);

	const FActiveGameplayEffectHandle ActiveHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	if (!ActiveHandle.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] ApplyStatIncrease: Failed to apply GE"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[BaseInventoryComponent] Applied item GE. StatTag: %s, Value: %.2f"), *StatTag.ToString(), ItemData->EffectValue);
	return true;
}

bool UBaseInventoryComponent::EnqueueFoodHeal(UUsableItemData* ItemData)
{
	if (ItemData == nullptr)
	{
		return false;
	}

	if (!CanUseItemsInCurrentLifeState())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] EnqueueFoodHeal: blocked while Down/Death"));
		return false;
	}

	if (ItemData->TotalHealAmount <= 0.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] EnqueueFoodHeal: Invalid TotalHealAmount %.2f"), ItemData->TotalHealAmount);
		return false;
	}

	if (ItemData->HealDurationSeconds <= 0.0f || ItemData->HealTickInterval <= 0.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseInventoryComponent] EnqueueFoodHeal: Invalid duration %.2f or tick %.2f"), ItemData->HealDurationSeconds, ItemData->HealTickInterval);
		return false;
	}

	const int32 TotalTicks = FMath::Max(1, FMath::RoundToInt(ItemData->HealDurationSeconds / ItemData->HealTickInterval));
	const float HealPerTick = ItemData->TotalHealAmount / static_cast<float>(TotalTicks);

	FPendingFoodHealEffect NewEffect;
	NewEffect.ItemName = ItemData->ItemName.ToString();
	NewEffect.TotalHealAmount = ItemData->TotalHealAmount;
	NewEffect.TotalDurationSeconds = ItemData->HealDurationSeconds;
	NewEffect.RemainingHealAmount = ItemData->TotalHealAmount;
	NewEffect.HealPerTick = HealPerTick;
	NewEffect.RemainingTicks = TotalTicks;
	NewEffect.TickInterval = ItemData->HealTickInterval;

	PendingFoodHealQueue.Add(NewEffect);
	StartNextFoodHealEffect();

	UE_LOG(LogTemp, Log, TEXT("[BaseInventoryComponent] EnqueueFoodHeal: Total=%.2f, Duration=%.2f, Tick=%.2f, Queue=%d"),
		ItemData->TotalHealAmount,
		ItemData->HealDurationSeconds,
		ItemData->HealTickInterval,
		PendingFoodHealQueue.Num());

	return true;
}

bool UBaseInventoryComponent::ApplyHealAmount(const float HealAmount)
{
	if (HealAmount <= 0.0f)
	{
		return false;
	}

	if (!CanUseItemsInCurrentLifeState())
	{
		return false;
	}

	UAbilitySystemComponent* const ASC = ResolveOwnerAbilitySystemComponent();
	if (ASC == nullptr)
	{
		return false;
	}

	ASC->ApplyModToAttribute(UBaseAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, HealAmount);
	return true;
}

void UBaseInventoryComponent::StartNextFoodHealEffect()
{
	if (bIsFoodHealEffectActive)
	{
		return;
	}

	if (PendingFoodHealQueue.IsEmpty())
	{
		return;
	}

	AActor* const OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return;
	}

	UWorld* const World = OwnerActor->GetWorld();
	if (World == nullptr)
	{
		return;
	}

	CurrentFoodHealEffect = PendingFoodHealQueue[0];
	PendingFoodHealQueue.RemoveAt(0);
	bIsFoodHealEffectActive = true;

	World->GetTimerManager().SetTimer(
		FoodHealTickTimerHandle,
		this,
		&UBaseInventoryComponent::HandleFoodHealTick,
		CurrentFoodHealEffect.TickInterval,
		true);
}

void UBaseInventoryComponent::StopFoodHealTimer()
{
	AActor* const OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return;
	}

	UWorld* const World = OwnerActor->GetWorld();
	if (World == nullptr)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(FoodHealTickTimerHandle);
}

void UBaseInventoryComponent::HandleFoodHealTick()
{
	if (!bIsFoodHealEffectActive)
	{
		StopFoodHealTimer();
		return;
	}

	if (!CanUseItemsInCurrentLifeState())
	{
		UE_LOG(LogTemp, Log, TEXT("[BaseInventoryComponent] HandleFoodHealTick: owner became Down/Death, clear food heal"));
		ClearFoodHealEffects();
		return;
	}

	if (CurrentFoodHealEffect.RemainingTicks <= 0 || CurrentFoodHealEffect.RemainingHealAmount <= 0.0f)
	{
		StopFoodHealTimer();
		bIsFoodHealEffectActive = false;
		StartNextFoodHealEffect();
		return;
	}

	const float TickHealAmount = FMath::Min(CurrentFoodHealEffect.HealPerTick, CurrentFoodHealEffect.RemainingHealAmount);
	const int32 RemainingTicksAfterThisTick = FMath::Max(CurrentFoodHealEffect.RemainingTicks - 1, 0);
	const float RemainingDurationSeconds = static_cast<float>(RemainingTicksAfterThisTick) * CurrentFoodHealEffect.TickInterval;

	UE_LOG(LogTemp, Log, TEXT("[FoodHealTick] Item=%s, TotalHeal=%.2f, TickHeal=%.2f, RemainingDuration=%.2f sec"),
		*CurrentFoodHealEffect.ItemName,
		CurrentFoodHealEffect.TotalHealAmount,
		TickHealAmount,
		RemainingDurationSeconds);

	const bool bHealed = ApplyHealAmount(TickHealAmount);
	if (!bHealed)
	{
		StopFoodHealTimer();
		bIsFoodHealEffectActive = false;
		return;
	}

	CurrentFoodHealEffect.RemainingHealAmount -= TickHealAmount;
	CurrentFoodHealEffect.RemainingTicks -= 1;

	if (CurrentFoodHealEffect.RemainingTicks > 0 && CurrentFoodHealEffect.RemainingHealAmount > 0.0f)
	{
		return;
	}

	StopFoodHealTimer();
	bIsFoodHealEffectActive = false;
	StartNextFoodHealEffect();
}

void UBaseInventoryComponent::ClearFoodHealEffects()
{
	StopFoodHealTimer();
	PendingFoodHealQueue.Empty();
	CurrentFoodHealEffect = FPendingFoodHealEffect();
	bIsFoodHealEffectActive = false;

	UE_LOG(LogTemp, Log, TEXT("[BaseInventoryComponent] ClearFoodHealEffects: cleared all food heal effects"));
}