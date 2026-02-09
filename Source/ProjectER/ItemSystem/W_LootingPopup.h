#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_LootingPopup.generated.h"

class UUniformGridPanel;
class UBaseItemData;
class ABaseBoxActor;
class UButton;

UCLASS()
class PROJECTER_API UW_LootingPopup : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Looting")
	void UpdateLootingSlots(const ABaseBoxActor* Box);

	UFUNCTION(BlueprintCallable, Category = "Looting")
	void TryLootItem(int32 ItemPoolIdx);

	void InitPopup(const ABaseBoxActor* Box);

	void Refresh();
	//AActor* GetTargetBox() const { return TargetBox; }

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(EditDefaultsOnly, Category = "Looting")
	TSubclassOf<UUserWidget> SlotWidgetClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> ItemGridPanel;

	UPROPERTY()
	TWeakObjectPtr<const ABaseBoxActor> TargetBox = nullptr;

	UPROPERTY()
	float MaxDistance = 300.f;

private:
	// 버튼을 눌렀을 때 어떤 아이템인지 알기 위한 매핑
	UPROPERTY()
	TMap<UButton*, int32> SlotItemMap;

	UFUNCTION()
	void OnSlotButtonClicked();
};