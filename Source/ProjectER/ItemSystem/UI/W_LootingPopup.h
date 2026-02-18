#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_LootingPopup.generated.h"

class UUniformGridPanel;
class UBaseItemData;
class ABaseBoxActor;
class UButton;
class UUI_ToolTip;		// 툴팁용
class UUI_ToolTipManager;	// 툴팁용

UCLASS()
class PROJECTER_API UW_LootingPopup : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Looting")
	void UpdateLootingSlots(const AActor* Box);

	UFUNCTION(BlueprintCallable, Category = "Looting")
	void TryLootItem(int32 ItemPoolIdx);

	void InitPopup(const AActor* Box);

	void Refresh();
	//AActor* GetTargetBox() const { return TargetBox; }

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override; // 팝업 클래스 소멸 처리를 위해 소멸자 추가

	UPROPERTY(EditDefaultsOnly, Category = "Looting")
	TSubclassOf<UUserWidget> SlotWidgetClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> ItemGridPanel;

	UPROPERTY()
	TObjectPtr<const AActor> TargetBox = nullptr;

	UPROPERTY()
	float MaxDistance = 300.f;

private:
	// 버튼을 눌렀을 때 어떤 아이템인지 알기 위한 매핑
	UPROPERTY()
	TMap<UButton*, int32> SlotItemMap;

	UFUNCTION()
	void OnSlotButtonClicked();

	// 툴팁용 공간
#pragma region Tooltip
protected:
	// 툴팁 클래스 (에디터에서 할당)
	UPROPERTY(EditAnywhere, Category = "Tooltip")
	TSubclassOf<UUserWidget> TooltipClass;

	// 툴팁 인스턴스
	UPROPERTY()
	UUI_ToolTip* TooltipInstance;

	UPROPERTY()
	UUI_ToolTipManager* TooltipManager;

	UFUNCTION()
	void OnItemHovered();
public:
	UFUNCTION()
	void HideTooltip();
#pragma endregion
};