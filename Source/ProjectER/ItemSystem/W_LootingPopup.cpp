#include "ItemSystem/W_LootingPopup.h"
#include "ItemSystem/BaseItemData.h"
#include "ItemSystem/BaseBoxActor.h"
#include "BaseInventoryComponent.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/Image.h"
#include "Components/Button.h"

void UW_LootingPopup::InitPopup(AActor* InTargetBox, float InMaxDistance)
{
	TargetBox = InTargetBox;
	MaxDistance = InMaxDistance;
}

void UW_LootingPopup::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (TargetBox)
	{
		APawn* OwningPawn = GetOwningPlayerPawn();
		if (OwningPawn && OwningPawn->GetDistanceTo(TargetBox) > MaxDistance)
		{
			RemoveFromParent();
		}
	}
}

void UW_LootingPopup::UpdateLootingSlots(const TArray<UBaseItemData*>& Items)
{
	if (!ItemGridPanel) return;
	if (!SlotWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("W_LootingPopup: SlotWidgetClass가 할당되지 않았습니다!"));
		return;
	}

	ItemGridPanel->ClearChildren();
	SlotItemMap.Empty();

	const int32 TotalSlots = 10;
	const int32 ColumnCount = 5;

	for (int32 i = 0; i < TotalSlots; ++i)
	{
		UUserWidget* NewSlot = CreateWidget<UUserWidget>(GetOwningPlayer(), SlotWidgetClass);
		if (NewSlot)
		{
			bool bHasItem = Items.IsValidIndex(i) && Items[i] != nullptr;
			UBaseItemData* CurrentItem = bHasItem ? Items[i] : nullptr;

			UImage* TargetImage = Cast<UImage>(NewSlot->GetWidgetFromName(TEXT("ItemIconImage")));
			if (TargetImage)
			{
				if (bHasItem && CurrentItem && CurrentItem->ItemIcon.IsValid())
				{
					UTexture2D* LoadedTexture = Cast<UTexture2D>(CurrentItem->ItemIcon.LoadSynchronous());
					if (LoadedTexture)
					{
						TargetImage->SetBrushFromTexture(LoadedTexture);
						TargetImage->SetVisibility(ESlateVisibility::HitTestInvisible); // 클릭 통과
					}
				}
				else
				{
					TargetImage->SetVisibility(ESlateVisibility::Hidden);
				}
			}

			UButton* SlotButton = Cast<UButton>(NewSlot->GetWidgetFromName(TEXT("SlotButton")));
			if (SlotButton)
			{
				if (bHasItem)
				{
					SlotItemMap.Add(SlotButton, CurrentItem);
					SlotButton->OnClicked.RemoveAll(this);
					SlotButton->OnClicked.AddDynamic(this, &UW_LootingPopup::OnSlotButtonClicked);
					SlotButton->SetIsEnabled(true);
				}
				else
				{
					SlotButton->SetIsEnabled(false);
				}
			}

			UUniformGridSlot* GridSlot = ItemGridPanel->AddChildToUniformGrid(NewSlot);
			if (GridSlot)
			{
				GridSlot->SetRow(i / ColumnCount);
				GridSlot->SetColumn(i % ColumnCount);
			}
		}
	}
}

void UW_LootingPopup::OnSlotButtonClicked()
{
	for (auto& Elem : SlotItemMap)
	{
		UButton* Btn = Elem.Key;
		if (Btn && (Btn->IsPressed() || Btn->IsHovered()))
		{
			TryLootItem(Elem.Value);
			return;
		}
	}
}

void UW_LootingPopup::TryLootItem(UBaseItemData* TargetItem)
{
	if (!TargetItem || !TargetBox) return;

	APawn* PlayerPawn = GetOwningPlayerPawn();
	UBaseInventoryComponent* Inventory = PlayerPawn ? PlayerPawn->FindComponentByClass<UBaseInventoryComponent>() : nullptr;

	if (Inventory && Inventory->AddItem(TargetItem))
	{
		ABaseBoxActor* Box = Cast<ABaseBoxActor>(TargetBox);
		if (Box)
		{
			Box->RemoveItemFromBox(TargetItem);

			// 즉시 남은 아이템으로 UI 갱신
			UpdateLootingSlots(Box->GetCurrentLoot());

			if (Box->GetCurrentLoot().Num() == 0)
			{
				RemoveFromParent();
			}
		}
	}
}