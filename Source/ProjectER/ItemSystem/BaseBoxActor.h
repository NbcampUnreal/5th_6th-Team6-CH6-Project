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
	void RemoveItemFromBox(UBaseItemData* ItemToRemove);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_CurrentLoot();

	virtual void PickupItem(class APawn* InHandler) override;

	UPROPERTY(VisibleAnywhere, Category = "Box")
	TObjectPtr<UStaticMeshComponent> BoxMesh;

	UPROPERTY(EditAnywhere, Category = "GAS")
	TSubclassOf<class UGameplayAbility> OpenAbilityClass;

	UPROPERTY(EditAnywhere, Category = "ProjectER|Loot")
	TArray<TObjectPtr<class UBaseItemData>> ItemPool;

	// 반드시 10개 요소가 리플리케이트 되도록 설정
	UPROPERTY(ReplicatedUsing = OnRep_CurrentLoot, VisibleAnywhere, Category = "ProjectER|Loot")
	TArray<TObjectPtr<UBaseItemData>> CurrentLoot;

	UPROPERTY(EditAnywhere, Category = "ProjectER|Loot")
	int32 MinLootCount = 1;

	UPROPERTY(EditAnywhere, Category = "ProjectER|Loot")
	int32 MaxLootCount = 3;
};