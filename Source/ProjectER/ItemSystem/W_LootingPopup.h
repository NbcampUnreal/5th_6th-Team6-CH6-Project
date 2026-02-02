#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_LootingPopup.generated.h"

class UUniformGridPanel;
class UBaseItemData;

UCLASS()
class PROJECTER_API UW_LootingPopup : public UUserWidget
{
    GENERATED_BODY()

public:
    // 슬롯 생성 및 아이콘 로드를 직접 처리하는 함수
    UFUNCTION(BlueprintCallable, Category = "Looting")
    void UpdateLootingSlots(const TArray<UBaseItemData*>& Items);

    UFUNCTION(BlueprintCallable, Category = "Looting")
    void TryLootItem(UBaseItemData* TargetItem);

    void InitPopup(AActor* InTargetBox, float InMaxDistance = 300.f);

protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UPROPERTY(EditDefaultsOnly, Category = "Looting")
    TSubclassOf<UUserWidget> SlotWidgetClass;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UUniformGridPanel> ItemGridPanel;

    UPROPERTY()
    TObjectPtr<AActor> TargetBox;

    UPROPERTY()
    float MaxDistance = 300.f;
};