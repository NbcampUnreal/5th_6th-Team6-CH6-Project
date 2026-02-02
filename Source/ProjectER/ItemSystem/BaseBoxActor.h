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

    // 아이템 목록 호출용
    const TArray<TObjectPtr<UBaseItemData>>& GetCurrentLoot() const { return CurrentLoot; }

    void RemoveItemFromBox(UBaseItemData* ItemToRemove);

protected:
    virtual void BeginPlay() override;

    // 네트워크 복제 설정 함수 추가
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void PickupItem(class APawn* InHandler) override;

    UPROPERTY(VisibleAnywhere, Category = "Box")
    TObjectPtr<UStaticMeshComponent> BoxMesh;

    UPROPERTY(EditAnywhere, Category = "GAS")
    TSubclassOf<class UGameplayAbility> OpenAbilityClass;

    UPROPERTY(EditAnywhere, Category = "ProjectER|Loot")
    TArray<TObjectPtr<class UBaseItemData>> ItemPool;

    UPROPERTY(Replicated, VisibleAnywhere, Category = "ProjectER|Loot")
    TArray<TObjectPtr<class UBaseItemData>> CurrentLoot;

    UPROPERTY(EditAnywhere, Category = "ProjectER|Loot")
    int32 MinLootCount = 3;

    UPROPERTY(EditAnywhere, Category = "ProjectER|Loot")
    int32 MaxLootCount = 6;
};