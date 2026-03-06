#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BaseInventoryComponent.generated.h"

class UBaseItemData;

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
	int32 MaxSlots = 20;

protected:
	// 멀티플레이어 동기화를 위해 Replicated 추가
	UPROPERTY(ReplicatedUsing = OnRep_InventoryContents, VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<UBaseItemData*> InventoryContents;

	UFUNCTION()
	void OnRep_InventoryContents();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

private:
	class UAbilitySystemComponent* ResolveOwnerAbilitySystemComponent() const;

	// 아이템 효과 적용 헬퍼 함수
	bool ApplyItemEffect(class UUsableItemData* ItemData);
	bool ApplyStatIncrease(class UAbilitySystemComponent* ASC, class UUsableItemData* ItemData);

public:
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryUpdatedSignature OnInventoryUpdated;
};