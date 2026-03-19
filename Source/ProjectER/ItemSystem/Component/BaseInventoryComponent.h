// File: 5th_6th-Team6-CH6-Project/Source/ProjectER/ItemSystem/Component/BaseInventoryComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ItemSystem/Data/UsableItemData.h"
#include "BaseInventoryComponent.generated.h"

class UAbilitySystemComponent;
class UBaseItemData;
class ABaseItemActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdatedSignature);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTER_API UBaseInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBaseInventoryComponent();

	// 아이템 추가 (서버에서만 실행되도록 내부 로직 수정)
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(UBaseItemData* InData);

	// 클라이언트가 호출하는 서버 요청용 RPC
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_AddItem(UBaseItemData* InData);

	// 아이템 사용
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UseItem(int32 SlotIndex);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_UseItem(int32 SlotIndex);

	// 인벤토리 정보 가져오기
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	int32 GetInventoryCount() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	UBaseItemData* GetItemAt(int32 SlotIndex) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 MaxSlots = 8;

protected:
	// 멀티플레이어 동기화를 위해 Replicated 추가
	UPROPERTY(ReplicatedUsing = OnRep_InventoryContents, VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<UBaseItemData*> InventoryContents;

	UFUNCTION()
	void OnRep_InventoryContents();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	struct FPendingFoodHealEffect
	{
		FString ItemName;
		float TotalHealAmount = 0.0f;
		float TotalDurationSeconds = 0.0f;
		float RemainingHealAmount = 0.0f;
		float HealPerTick = 0.0f;
		int32 RemainingTicks = 0;
		float TickInterval = 1.0f;
	};

	UAbilitySystemComponent* ResolveOwnerAbilitySystemComponent() const;
	bool CanUseItemsInCurrentLifeState() const;
	FGameplayTag GetSetByCallerTagFromStatType(EItemStatType StatType) const;

	bool ApplyItemEffect(UUsableItemData* ItemData);
	bool ApplyStatIncrease(UAbilitySystemComponent* ASC, UUsableItemData* ItemData);
	bool EnqueueFoodHeal(UUsableItemData* ItemData);
	bool ApplyHealAmount(float HealAmount);
	void StartNextFoodHealEffect();
	void StopFoodHealTimer();
	void HandleFoodHealTick();

	FTimerHandle FoodHealTickTimerHandle;
	TArray<FPendingFoodHealEffect> PendingFoodHealQueue;
	FPendingFoodHealEffect CurrentFoodHealEffect;
	bool bIsFoodHealEffectActive = false;

public:
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryUpdatedSignature OnInventoryUpdated;

	void ClearFoodHealEffects();

	// 슬롯 이동 / 교환
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SwapSlots(int32 FromIndex, int32 ToIndex);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SwapSlots(int32 FromIndex, int32 ToIndex);

	// 슬롯 아이템을 월드에 떨어뜨리기
	bool DropItemFromSlot(int32 SlotIndex, const FVector& SpawnLocation, TSubclassOf<ABaseItemActor> ItemActorClass, APawn* DropperPawn);
};