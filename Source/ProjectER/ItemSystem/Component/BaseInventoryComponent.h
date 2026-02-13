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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 MaxSlots = 20;

protected:
	// 멀티플레이어 동기화를 위해 Replicated 추가
	UPROPERTY(ReplicatedUsing = OnRep_InventoryContents, VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<UBaseItemData*> InventoryContents;

	UFUNCTION()
	void OnRep_InventoryContents();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryUpdatedSignature OnInventoryUpdated;
};