#include "ItemSystem/UI/W_InventorySlot.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "ItemSystem/Component/BaseInventoryComponent.h"
#include "ItemSystem/Data/BaseItemData.h"
#include "ItemSystem/UI/InventoryDragDropOperation.h"
#include "ItemSystem/UI/W_InventoryDragVisual.h"

UW_InventorySlot::UW_InventorySlot(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Visible);
	bIsFocusable = false;
}

TSharedRef<SWidget> UW_InventorySlot::RebuildWidget()
{
	BuildWidgetTree();

	TSharedRef<SWidget> Result = Super::RebuildWidget();
	RefreshVisual();
	ApplyNormalStyle();
	return Result;
}

void UW_InventorySlot::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshVisual();
	ApplyNormalStyle();
}

void UW_InventorySlot::BuildWidgetTree()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("InventorySlotWidgetTree"));
	}

	if (WidgetTree->RootWidget)
	{
		return;
	}

	RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
	RootSizeBox->SetWidthOverride(64.f);
	RootSizeBox->SetHeightOverride(64.f);

	SlotBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SlotBorder"));
	SlotBorder->SetPadding(FMargin(4.f));

	ItemIconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ItemIconImage"));
	ItemIconImage->SetVisibility(ESlateVisibility::HitTestInvisible);

	SlotBorder->SetContent(ItemIconImage);
	RootSizeBox->SetContent(SlotBorder);
	WidgetTree->RootWidget = RootSizeBox;
}

void UW_InventorySlot::SetSlotIndex(int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;
}

void UW_InventorySlot::SetItemData(UBaseItemData* InItemData)
{
	CachedItemData = InItemData;
	RefreshVisual();
}

void UW_InventorySlot::RefreshVisual()
{
	if (!ItemIconImage)
	{
		return;
	}

	if (CachedItemData && !CachedItemData->ItemIcon.IsNull())
	{
		if (UTexture2D* IconTexture = CachedItemData->ItemIcon.LoadSynchronous())
		{
			ItemIconImage->SetBrushFromTexture(IconTexture);
			ItemIconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
			return;
		}
	}

	ItemIconImage->SetBrush(FSlateBrush());
	ItemIconImage->SetVisibility(ESlateVisibility::Hidden);
}

void UW_InventorySlot::ApplyNormalStyle()
{
	if (!SlotBorder)
	{
		return;
	}

	SlotBorder->SetBrushColor(FLinearColor(0.08f, 0.08f, 0.08f, 0.85f));
}

void UW_InventorySlot::ApplyDropHoverStyle()
{
	if (!SlotBorder)
	{
		return;
	}

	SlotBorder->SetBrushColor(FLinearColor(0.85f, 0.65f, 0.20f, 0.95f));
}

UBaseInventoryComponent* UW_InventorySlot::GetInventoryComponent() const
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return nullptr;
	}

	APawn* Pawn = PC->GetPawn();
	if (!Pawn)
	{
		return nullptr;
	}

	return Pawn->FindComponentByClass<UBaseInventoryComponent>();
}

FReply UW_InventorySlot::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (CachedItemData && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UW_InventorySlot::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	if (!CachedItemData)
	{
		OutOperation = nullptr;
		return;
	}

	UInventoryDragDropOperation* DragOp =
		Cast<UInventoryDragDropOperation>(UWidgetBlueprintLibrary::CreateDragDropOperation(UInventoryDragDropOperation::StaticClass()));

	if (!DragOp)
	{
		OutOperation = nullptr;
		return;
	}

	DragOp->SourceSlotIndex = SlotIndex;
	DragOp->Pivot = EDragPivot::MouseDown;

	UW_InventoryDragVisual* DragVisual = CreateWidget<UW_InventoryDragVisual>(GetOwningPlayer(), UW_InventoryDragVisual::StaticClass());
	if (DragVisual)
	{
		UTexture2D* IconTexture = nullptr;
		if (!CachedItemData->ItemIcon.IsNull())
		{
			IconTexture = CachedItemData->ItemIcon.LoadSynchronous();
		}

		DragVisual->SetItemTexture(IconTexture);
		DragOp->DefaultDragVisual = DragVisual;
	}

	OutOperation = DragOp;
}

void UW_InventorySlot::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);

	if (const UInventoryDragDropOperation* DragOp = Cast<UInventoryDragDropOperation>(InOperation))
	{
		if (DragOp->SourceSlotIndex != SlotIndex)
		{
			ApplyDropHoverStyle();
		}
	}
}

void UW_InventorySlot::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);
	ApplyNormalStyle();
}

bool UW_InventorySlot::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	ApplyNormalStyle();

	const UInventoryDragDropOperation* DragOp = Cast<UInventoryDragDropOperation>(InOperation);
	if (!DragOp)
	{
		return false;
	}

	if (DragOp->SourceSlotIndex == SlotIndex)
	{
		return true;
	}

	UBaseInventoryComponent* InventoryComp = GetInventoryComponent();
	if (!InventoryComp)
	{
		return false;
	}

	return InventoryComp->SwapSlots(DragOp->SourceSlotIndex, SlotIndex);
}