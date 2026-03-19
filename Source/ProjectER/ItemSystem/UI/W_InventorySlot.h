#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_InventorySlot.generated.h"

class UBorder;
class UImage;
class USizeBox;
class UBaseItemData;
class UBaseInventoryComponent;

UCLASS()
class PROJECTER_API UW_InventorySlot : public UUserWidget
{
	GENERATED_BODY()

public:
	UW_InventorySlot(const FObjectInitializer& ObjectInitializer);

	void SetSlotIndex(int32 InSlotIndex);
	int32 GetSlotIndex() const { return SlotIndex; }

	void SetItemData(UBaseItemData* InItemData);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	void BuildWidgetTree();
	void RefreshVisual();
	void ApplyNormalStyle();
	void ApplyDropHoverStyle();

	UBaseInventoryComponent* GetInventoryComponent() const;

	UPROPERTY()
	TObjectPtr<USizeBox> RootSizeBox = nullptr;

	UPROPERTY()
	TObjectPtr<UBorder> SlotBorder = nullptr;

	UPROPERTY()
	TObjectPtr<UImage> ItemIconImage = nullptr;

	UPROPERTY()
	TObjectPtr<UBaseItemData> CachedItemData = nullptr;

	int32 SlotIndex = INDEX_NONE;
};