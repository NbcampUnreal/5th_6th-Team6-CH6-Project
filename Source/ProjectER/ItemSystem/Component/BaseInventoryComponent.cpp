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
#include "ItemSystem/Actor/BaseItemActor.h"

UBaseInventoryComponent::UBaseInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	MaxSlots = 8;
	MaxStackPerSlot = 5;
	SetIsReplicatedByDefault(true);
}

void UBaseInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	EnsureInventoryArraysValid();
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
	DOREPLIFETIME(UBaseInventoryComponent, InventoryStackCounts);
}

int32 UBaseInventoryComponent::GetInventoryCount() const
{
	int32 Count = 0;

	const int32 SlotCount = FMath::Min(InventoryContents.Num(), InventoryStackCounts.Num());
	for (int32 i = 0; i < SlotCount; ++i)
	{
		if (InventoryContents[i] != nullptr && InventoryStackCounts[i] > 0)
		{
			++Count;
		}
	}

	return Count;
}

bool UBaseInventoryComponent::AddItem(UBaseItemData* Item)
{
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

	EnsureInventoryArraysValid();

	const int32 SafeMaxStack = FMath::Max(1, MaxStackPerSlot);

	// 1) 먼저 기존 스택에 합치기
	for (int32 i = 0; i < InventoryContents.Num(); ++i)
	{
		if (InventoryContents[i] == Item &&
			InventoryStackCounts.IsValidIndex(i) &&
			InventoryStackCounts[i] > 0 &&
			InventoryStackCounts[i] < SafeMaxStack)
		{
			++InventoryStackCounts[i];
			OnInventoryUpdated.Broadcast();

			UE_LOG(LogTemp, Log, TEXT("[BaseInventoryComponent] Stacked item '%s' in slot %d. Count=%d"),
				*Item->ItemName.ToString(), i, InventoryStackCounts[i]);

			return true;
		}
	}

	// 2) 빈 슬롯 찾기
	int32 EmptySlotIndex = INDEX_NONE;
	for (int32 i = 0; i < InventoryContents.Num(); ++i)
	{
		if (InventoryContents[i] == nullptr || InventoryStackCounts[i] <= 0)
		{
			EmptySlotIndex = i;
			break;
		}
	}

	if (EmptySlotIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] Inventory is full. Cannot add item: %s"), *Item->ItemName.ToString());
		return false;
	}

	InventoryContents[EmptySlotIndex] = Item;
	InventoryStackCounts[EmptySlotIndex] = 1;

	OnInventoryUpdated.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("[BaseInventoryComponent] Added new item '%s' to slot %d"),
		*Item->ItemName.ToString(), EmptySlotIndex);

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
	EnsureInventoryArraysValid();
	OnInventoryUpdated.Broadcast();
}

UBaseItemData* UBaseInventoryComponent::GetItemAt(const int32 SlotIndex) const
{
	if (!InventoryContents.IsValidIndex(SlotIndex) || !InventoryStackCounts.IsValidIndex(SlotIndex))
	{
		return nullptr;
	}

	if (InventoryStackCounts[SlotIndex] <= 0)
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

	if (InventoryStackCounts.IsValidIndex(SlotIndex) && InventoryStackCounts[SlotIndex] > 1)
	{
		--InventoryStackCounts[SlotIndex];
	}
	else
	{
		InventoryContents[SlotIndex] = nullptr;
		if (InventoryStackCounts.IsValidIndex(SlotIndex))
		{
			InventoryStackCounts[SlotIndex] = 0;
		}
	}

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

bool UBaseInventoryComponent::SwapSlots(int32 FromIndex, int32 ToIndex)
{
	AActor* const OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] SwapSlots: Owner is null"));
		return false;
	}

	if (FromIndex == ToIndex)
	{
		return true;
	}

	if (!InventoryContents.IsValidIndex(FromIndex) || !InventoryContents.IsValidIndex(ToIndex) ||
		!InventoryStackCounts.IsValidIndex(FromIndex) || !InventoryStackCounts.IsValidIndex(ToIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] SwapSlots: Invalid index From=%d To=%d"), FromIndex, ToIndex);
		return false;
	}

	// 클라이언트면 서버에 요청
	if (!OwnerActor->HasAuthority())
	{
		Server_SwapSlots(FromIndex, ToIndex);
		return true;
	}

	// 빈 슬롯에서 드래그한 경우는 무시
	if (InventoryContents[FromIndex] == nullptr || InventoryStackCounts[FromIndex] <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] SwapSlots: Source slot is empty. From=%d"), FromIndex);
		return false;
	}

	UBaseItemData* SourceItem = InventoryContents[FromIndex];
	UBaseItemData* TargetItem = InventoryContents[ToIndex];

	int32& SourceCount = InventoryStackCounts[FromIndex];
	int32& TargetCount = InventoryStackCounts[ToIndex];

	// 1) 같은 아이템이면 먼저 스택 합치기 시도
	if (SourceItem != nullptr &&
		TargetItem != nullptr &&
		SourceItem == TargetItem)
	{
		const int32 SafeMaxStack = FMath::Max(1, MaxStackPerSlot);
		const int32 SpaceLeft = SafeMaxStack - TargetCount;

		if (SpaceLeft <= 0)
		{
			return true;
		}

		const int32 MoveAmount = FMath::Min(SourceCount, SpaceLeft);

		TargetCount += MoveAmount;
		SourceCount -= MoveAmount;

		if (SourceCount <= 0)
		{
			InventoryContents[FromIndex] = nullptr;
			SourceCount = 0;
		}

		OnInventoryUpdated.Broadcast();
		return true;
	}

	// 2) 같은 아이템이 아니면 기존처럼 위치 교환
	UBaseItemData* TempItem = InventoryContents[FromIndex];
	InventoryContents[FromIndex] = InventoryContents[ToIndex];
	InventoryContents[ToIndex] = TempItem;

	int32 TempCount = InventoryStackCounts[FromIndex];
	InventoryStackCounts[FromIndex] = InventoryStackCounts[ToIndex];
	InventoryStackCounts[ToIndex] = TempCount;

	OnInventoryUpdated.Broadcast();
	return true;
}

bool UBaseInventoryComponent::Server_SwapSlots_Validate(int32 FromIndex, int32 ToIndex)
{
	return FromIndex >= 0
		&& FromIndex < MaxSlots
		&& ToIndex >= 0
		&& ToIndex < MaxSlots
		&& FromIndex != ToIndex;
}

void UBaseInventoryComponent::Server_SwapSlots_Implementation(int32 FromIndex, int32 ToIndex)
{
	SwapSlots(FromIndex, ToIndex);
}

bool UBaseInventoryComponent::DropItemFromSlot(int32 SlotIndex, const FVector& SpawnLocation, TSubclassOf<ABaseItemActor> ItemActorClass, APawn* DropperPawn)
{
	AActor* const OwnerActor = GetOwner();
	if (OwnerActor == nullptr || !OwnerActor->HasAuthority())
	{
		return false;
	}

	if (!InventoryContents.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] DropItemFromSlot: Invalid SlotIndex %d"), SlotIndex);
		return false;
	}

	UBaseItemData* ItemData = InventoryContents[SlotIndex];
	if (ItemData == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] DropItemFromSlot: Slot %d is empty"), SlotIndex);
		return false;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	TSubclassOf<ABaseItemActor> SpawnClass = ItemActorClass;
	if (!SpawnClass)
	{
		SpawnClass = ABaseItemActor::StaticClass();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerActor;
	SpawnParams.Instigator = DropperPawn;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ABaseItemActor* SpawnedItem = World->SpawnActor<ABaseItemActor>(
		SpawnClass,
		SpawnLocation,
		FRotator::ZeroRotator,
		SpawnParams);

	if (!SpawnedItem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BaseInventoryComponent] DropItemFromSlot: Failed to spawn dropped item actor"));
		return false;
	}

	SpawnedItem->InitializeFromItemData(ItemData, DropperPawn);

	if (InventoryStackCounts[SlotIndex] > 1)
	{
		--InventoryStackCounts[SlotIndex];
	}
	else
	{
		InventoryContents[SlotIndex] = nullptr;
		InventoryStackCounts[SlotIndex] = 0;
	}

	OnInventoryUpdated.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("[BaseInventoryComponent] Dropped item '%s' from slot %d"),
		*ItemData->ItemName.ToString(), SlotIndex);

	return true;
}

void UBaseInventoryComponent::EnsureInventoryArraysValid()
{
	if (InventoryContents.Num() < MaxSlots)
	{
		InventoryContents.AddDefaulted(MaxSlots - InventoryContents.Num());
	}
	else if (InventoryContents.Num() > MaxSlots)
	{
		InventoryContents.SetNum(MaxSlots);
	}

	if (InventoryStackCounts.Num() < MaxSlots)
	{
		InventoryStackCounts.AddZeroed(MaxSlots - InventoryStackCounts.Num());
	}
	else if (InventoryStackCounts.Num() > MaxSlots)
	{
		InventoryStackCounts.SetNum(MaxSlots);
	}

	for (int32 i = 0; i < MaxSlots; ++i)
	{
		if (InventoryContents[i] == nullptr || InventoryStackCounts[i] <= 0)
		{
			InventoryContents[i] = nullptr;
			InventoryStackCounts[i] = 0;
		}
	}
}

int32 UBaseInventoryComponent::GetStackCountAt(int32 SlotIndex) const
{
	if (!InventoryContents.IsValidIndex(SlotIndex) || !InventoryStackCounts.IsValidIndex(SlotIndex))
	{
		return 0;
	}

	if (InventoryContents[SlotIndex] == nullptr)
	{
		return 0;
	}

	return FMath::Max(InventoryStackCounts[SlotIndex], 0);
}