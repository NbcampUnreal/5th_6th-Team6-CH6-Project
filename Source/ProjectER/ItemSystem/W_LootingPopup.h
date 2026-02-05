#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_LootingPopup.generated.h"

class UUniformGridPanel;
class UBaseItemData;
class UButton;

UCLASS()
class PROJECTER_API UW_LootingPopup : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Looting")
	void UpdateLootingSlots(const TArray<UBaseItemData*>& Items);

	UFUNCTION(BlueprintCallable, Category = "Looting")
	void TryLootItem(UBaseItemData* TargetItem);

	void InitPopup(AActor* InTargetBox, float InMaxDistance = 300.f);
	AActor* GetTargetBox() const { return TargetBox; }

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

private:
	// 버튼을 눌렀을 때 어떤 아이템인지 알기 위한 매핑
	UPROPERTY()
	TMap<UButton*, UBaseItemData*> SlotItemMap;

	UFUNCTION()
	void OnSlotButtonClicked();
};