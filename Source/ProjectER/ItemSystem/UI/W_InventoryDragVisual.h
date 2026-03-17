#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_InventoryDragVisual.generated.h"

class USizeBox;
class UBorder;
class UImage;
class UTexture2D;

UCLASS()
class PROJECTER_API UW_InventoryDragVisual : public UUserWidget
{
	GENERATED_BODY()

public:
	UW_InventoryDragVisual(const FObjectInitializer& ObjectInitializer);

	void SetItemTexture(UTexture2D* InTexture);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildWidgetTree();
	void RefreshVisual();

	UPROPERTY()
	TObjectPtr<USizeBox> RootSizeBox = nullptr;

	UPROPERTY()
	TObjectPtr<UBorder> RootBorder = nullptr;

	UPROPERTY()
	TObjectPtr<UImage> ItemIconImage = nullptr;

	UPROPERTY()
	TObjectPtr<UTexture2D> PendingTexture = nullptr;
};