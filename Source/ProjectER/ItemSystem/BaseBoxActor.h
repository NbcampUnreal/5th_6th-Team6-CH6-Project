#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemSystem/I_ItemInteractable.h"
#include "BaseBoxActor.generated.h"

class UBaseItemData;

UCLASS()
class PROJECTER_API ABaseBoxActor : public AActor, public II_ItemInteractable // 인터페이스 상속 추가
{
    GENERATED_BODY()

public:
    ABaseBoxActor();

    // 아이템 목록 호출용
    const TArray<TObjectPtr<UBaseItemData>>& GetCurrentLoot() const { return CurrentLoot; }

protected:
    virtual void BeginPlay() override;

    // 캐릭터가 상자에 도달했을 때 호출될 함수
    virtual void PickupItem(class APawn* InHandler) override;

    // 상자의 외형을 담당할 스태틱 메시 컴포넌트
    UPROPERTY(VisibleAnywhere, Category = "Box")
    TObjectPtr<UStaticMeshComponent> BoxMesh;

    // 상자를 열 때 실행할 GAS 어빌리티 (에디터에서 지정)
    UPROPERTY(EditAnywhere, Category = "GAS")
    TSubclassOf<class UGameplayAbility> OpenAbilityClass;

    UPROPERTY(EditAnywhere, Category = "ProjectER|Loot")
    TArray<TObjectPtr<class UBaseItemData>> ItemPool;

    // 현재 상자 안에 생성된 아이템들 (최대 10개 - 5x2)
    UPROPERTY(VisibleAnywhere, Category = "ProjectER|Loot")
    TArray<TObjectPtr<class UBaseItemData>> CurrentLoot;

    // 랜덤으로 몇 개를 뽑을지 설정
    UPROPERTY(EditAnywhere, Category = "ProjectER|Loot")
    int32 MinLootCount = 3;

    UPROPERTY(EditAnywhere, Category = "ProjectER|Loot")
    int32 MaxLootCount = 6;

};