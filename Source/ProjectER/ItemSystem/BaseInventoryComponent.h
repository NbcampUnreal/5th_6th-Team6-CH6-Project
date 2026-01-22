#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BaseInventoryComponent.generated.h"

class UBaseItemData;

// 인벤토리 변경 시 UI에 알릴 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdatedSignature);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTER_API UBaseInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBaseInventoryComponent();

	// 아이템 추가 함수
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(UBaseItemData* InData);

public:
	// 인벤토리 최대 슬롯 수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 MaxSlots = 20;

protected:
	// 아이템 데이터를 담는 배열
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<UBaseItemData*> InventoryContents;

public:
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryUpdatedSignature OnInventoryUpdated;
};