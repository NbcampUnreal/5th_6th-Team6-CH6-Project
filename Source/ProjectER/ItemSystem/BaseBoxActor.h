#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemSystem/I_ItemInteractable.h"
#include "BaseBoxActor.generated.h"

class UBaseItemData;

USTRUCT(BlueprintType)
struct FLootSlot
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ItemId = 0;         // ItemPool 인덱스

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Count = 0;          // 0이면 빈 슬롯
};

UCLASS()
class PROJECTER_API ABaseBoxActor : public AActor, public II_ItemInteractable
{
	GENERATED_BODY()

public:
	ABaseBoxActor();

	TArray<FLootSlot> GetCurrentItemList() const { return CurrentItemList; }

public:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


	UFUNCTION()
	void OnRep_GetCurrentItemList();

	void ReduceItem(int32 SlotIndex);

	UBaseItemData* GetItemData(int32 SlotIndex) const;

	// 인터페이스 함수
	virtual void PickupItem(class APawn* InHandler) override;

	UPROPERTY(VisibleAnywhere, Category = "Box")
	TObjectPtr<UStaticMeshComponent> BoxMesh;

	UPROPERTY(EditAnywhere, Category = "ProjectER|Loot")
	TArray<TObjectPtr<class UBaseItemData>> ItemPool;


	UPROPERTY(ReplicatedUsing = OnRep_GetCurrentItemList, VisibleAnywhere, Category = "ProjectER|Loot")
	TArray<FLootSlot> CurrentItemList;

	UPROPERTY(EditAnywhere, Category = "ProjectER|Loot")
	int32 MinLootCount = 1;

	UPROPERTY(EditAnywhere, Category = "ProjectER|Loot")
	int32 MaxLootCount = 3;


	DECLARE_MULTICAST_DELEGATE(FOnLootChanged);
	FOnLootChanged OnLootChanged;

};