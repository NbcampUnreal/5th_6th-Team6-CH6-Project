#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_LootingPopup.generated.h"

UCLASS()
class PROJECTER_API UW_LootingPopup : public UUserWidget
{
    GENERATED_BODY()

public:
    // 상자 안의 아이템 리스트를 받아 UI를 갱신하는 함수
    UFUNCTION(BlueprintImplementableEvent, Category = "Looting")
    void UpdateLootingSlots(const TArray<class UBaseItemData*>& Items);

public:
    // 초기화 시 상자와 캐릭터 정보를 저장
    void InitPopup(AActor* InTargetBox, float InMaxDistance = 300.f);

protected:
    // 매 프레임 거리를 체크하기 위해 NativeTick 사용
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UPROPERTY()
    TObjectPtr<AActor> TargetBox;

    UPROPERTY()
    float MaxDistance = 300.f;
};