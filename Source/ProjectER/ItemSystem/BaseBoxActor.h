#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemSystem/I_ItemInteractable.h"
#include "BaseBoxActor.generated.h"

class UBaseItemData;

UCLASS()
class PROJECTER_API ABaseBoxActor : public AActor, public II_ItemInteractable
{
	GENERATED_BODY()

public:
	ABaseBoxActor();

	const TArray<TObjectPtr<UBaseItemData>>& GetCurrentLoot() const { return CurrentLoot; }

	// 클라이언트에서 호출할 아이템 제거 요청 RPC
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RemoveItemFromBox(UBaseItemData* ItemToRemove);

	void RemoveItemFromBox(UBaseItemData* ItemToRemove);

	// [추가] 클라이언트에서 호출할 상호작용(상자 열기) 요청 RPC
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_PickupItem(class APawn* InHandler);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_CurrentLoot();

	// 인터페이스 함수
	virtual void PickupItem(class APawn* InHandler) override;

	UPROPERTY(VisibleAnywhere, Category = "Box")
	TObjectPtr<UStaticMeshComponent> BoxMesh;

	UPROPERTY(EditAnywhere, Category = "GAS")
	TSubclassOf<class UGameplayAbility> OpenAbilityClass;

	UPROPERTY(EditAnywhere, Category = "ProjectER|Loot")
	TArray<TObjectPtr<class UBaseItemData>> ItemPool;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentLoot, VisibleAnywhere, Category = "ProjectER|Loot")
	TArray<TObjectPtr<UBaseItemData>> CurrentLoot;

	UPROPERTY(EditAnywhere, Category = "ProjectER|Loot")
	int32 MinLootCount = 1;

	UPROPERTY(EditAnywhere, Category = "ProjectER|Loot")
	int32 MaxLootCount = 3;
};