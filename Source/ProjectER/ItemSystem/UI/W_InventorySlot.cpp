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
#include "CharacterSystem/Player/BasePlayerController.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"

#include "UI/UI_ToolTip.h"	// 툴팁용
#include "UI/UI_ToolTipManager.h"	// 툴팁용

UW_InventorySlot::UW_InventorySlot(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Visible);
	bIsFocusable = false;

	static ConstructorHelpers::FClassFinder<UUI_ToolTip> ToolTipAsset(TEXT("/Game/Blueprints/BP_UI/BP_UI_ToolTip.BP_UI_ToolTip_C"));

	if (ToolTipAsset.Succeeded())
	{
		ToolTipWidgetClass = ToolTipAsset.Class;
	}
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

	RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay"));

	ItemIconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ItemIconImage"));
	ItemIconImage->SetVisibility(ESlateVisibility::HitTestInvisible);

	StackCountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StackCountText"));
	StackCountText->SetVisibility(ESlateVisibility::Hidden);
	StackCountText->SetText(FText::GetEmpty());
	StackCountText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	StackCountText->SetShadowOffset(FVector2D(1.f, 1.f));
	StackCountText->SetShadowColorAndOpacity(FLinearColor::Black);

	if (UOverlaySlot* IconSlot = RootOverlay->AddChildToOverlay(ItemIconImage))
	{
		IconSlot->SetHorizontalAlignment(HAlign_Fill);
		IconSlot->SetVerticalAlignment(VAlign_Fill);
	}

	if (UOverlaySlot* CountSlot = RootOverlay->AddChildToOverlay(StackCountText))
	{
		CountSlot->SetHorizontalAlignment(HAlign_Right);
		CountSlot->SetVerticalAlignment(VAlign_Bottom);
		CountSlot->SetPadding(FMargin(0.f, 0.f, 4.f, 2.f));
	}

	SlotBorder->SetContent(RootOverlay);
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
		}
		else
		{
			ItemIconImage->SetBrush(FSlateBrush());
			ItemIconImage->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	else
	{
		ItemIconImage->SetBrush(FSlateBrush());
		ItemIconImage->SetVisibility(ESlateVisibility::Hidden);
	}

	if (StackCountText)
	{
		if (CachedItemData && CachedStackCount > 1)
		{
			StackCountText->SetText(FText::FromString(FString::Printf(TEXT("*%d"), CachedStackCount)));
			StackCountText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			StackCountText->SetText(FText::GetEmpty());
			StackCountText->SetVisibility(ESlateVisibility::Hidden);
		}
	}
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

// 버튼 클릭으로 아이템 먹어지도록
void UW_InventorySlot::thisPressed()
{
	ABasePlayerController* PC = Cast<ABasePlayerController>(GetOwningPlayer());

	if (IsValid(PC))
	{
		PC->UseInventoryForUI(SlotIndex);
	}
}

FReply UW_InventorySlot::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		// UE_LOG(LogTemp, Log, TEXT("Inventory Slot [%d] - Mouse Down!"), SlotIndex);

		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UW_InventorySlot::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		
		// UE_LOG(LogTemp, Warning, TEXT("Inventory Slot [%d] - Mouse Up! Action Executed."), SlotIndex);
		thisPressed();
		
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
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
	DragOp->SourcePlayerController = Cast<ABasePlayerController>(GetOwningPlayer());
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

void UW_InventorySlot::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (!TooltipInstance && ToolTipWidgetClass)
	{
		TooltipInstance = CreateWidget<UUI_ToolTip>(GetOwningPlayer(), ToolTipWidgetClass);
		if (TooltipInstance)
		{
			TooltipInstance->SetVisibility(ESlateVisibility::Collapsed);
			TooltipInstance->AddToViewport(10);
		}
	}

	if (!TooltipManager && TooltipInstance)
	{
		TooltipManager = NewObject<UUI_ToolTipManager>(this);
		TooltipManager->setTooltipInstance(TooltipInstance);
	}

	// 툴팁 표시
	if (CachedItemData && TooltipManager)
	{
		TooltipInstance->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		TooltipManager->ShowTooltip(
			this,
			CachedItemData->ItemName,
			CachedItemData->ItemShortDesc,
			CachedItemData->ItemLongDesc,
			FText::GetEmpty(),
			true
		);
	}
}
void UW_InventorySlot::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	if (IsValid(TooltipInstance))
		TooltipInstance->SetVisibility(ESlateVisibility::Collapsed);

	UE_LOG(LogTemp, Log, TEXT("Mouse Left Slot %d"), SlotIndex);
}

void UW_InventorySlot::SetStackCount(int32 InStackCount)
{
	CachedStackCount = InStackCount;
	RefreshVisual();
}