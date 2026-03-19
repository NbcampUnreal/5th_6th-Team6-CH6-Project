#include "ItemSystem/UI/W_InventoryDragVisual.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"

UW_InventoryDragVisual::UW_InventoryDragVisual(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

TSharedRef<SWidget> UW_InventoryDragVisual::RebuildWidget()
{
	BuildWidgetTree();

	TSharedRef<SWidget> Result = Super::RebuildWidget();
	RefreshVisual();
	return Result;
}

void UW_InventoryDragVisual::BuildWidgetTree()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("InventoryDragVisualTree"));
	}

	if (WidgetTree->RootWidget)
	{
		return;
	}

	RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
	RootSizeBox->SetWidthOverride(56.f);
	RootSizeBox->SetHeightOverride(56.f);

	RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RootBorder"));
	RootBorder->SetPadding(FMargin(2.f));
	RootBorder->SetBrushColor(FLinearColor(1.f, 1.f, 1.f, 0.9f));

	ItemIconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ItemIconImage"));
	ItemIconImage->SetVisibility(ESlateVisibility::HitTestInvisible);

	RootBorder->SetContent(ItemIconImage);
	RootSizeBox->SetContent(RootBorder);
	WidgetTree->RootWidget = RootSizeBox;
}

void UW_InventoryDragVisual::SetItemTexture(UTexture2D* InTexture)
{
	PendingTexture = InTexture;
	RefreshVisual();
}

void UW_InventoryDragVisual::RefreshVisual()
{
	if (!ItemIconImage)
	{
		return;
	}

	if (PendingTexture)
	{
		ItemIconImage->SetBrushFromTexture(PendingTexture);
		ItemIconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		ItemIconImage->SetBrush(FSlateBrush());
		ItemIconImage->SetVisibility(ESlateVisibility::Hidden);
	}
}