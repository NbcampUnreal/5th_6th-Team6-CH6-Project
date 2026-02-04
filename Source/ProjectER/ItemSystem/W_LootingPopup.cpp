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
	if (TargetBox && GetOwningPlayerPawn())
	{
		if (GetOwningPlayerPawn()->GetDistanceTo(TargetBox) > MaxDistance)
			RemoveFromParent();
	}
}

void UW_LootingPopup::UpdateLootingSlots(const TArray<UBaseItemData*>& Items)
{
	if (!ItemGridPanel || !SlotWidgetClass) return;

	ItemGridPanel->ClearChildren();
	SlotItemMap.Empty();

	const int32 ColumnCount = 5;

	// [중요] 아이템 개수와 상관없이 무조건 10개의 슬롯 위젯을 생성
	for (int32 i = 0; i < 10; ++i)
	{
		UUserWidget* NewSlot = CreateWidget<UUserWidget>(GetOwningPlayer(), SlotWidgetClass);
		if (NewSlot)
		{
			// i번째 칸에 아이템이 있는지 확인
			bool bHasItem = Items.IsValidIndex(i) && Items[i] != nullptr;
			UBaseItemData* CurrentItem = bHasItem ? Items[i] : nullptr;

			UImage* TargetImage = Cast<UImage>(NewSlot->GetWidgetFromName(TEXT("ItemIconImage")));
			UButton* SlotButton = Cast<UButton>(NewSlot->GetWidgetFromName(TEXT("SlotButton")));

			if (TargetImage)
			{
				if (bHasItem)
				{
					TargetImage->SetBrushFromTexture(CurrentItem->ItemIcon.LoadSynchronous());
					TargetImage->SetVisibility(ESlateVisibility::HitTestInvisible);
				}
				else
				{
					// 아이템이 없으면 이미지만 숨김 (슬롯 배경은 남음)
					TargetImage->SetVisibility(ESlateVisibility::Collapsed);
				}
			}

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
					// 아이템이 없으면 클릭 비활성화 (이 상태에서 슬롯 위젯의 디자인에 따라 반투명하게 보임)
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

	ABaseBoxActor* Box = Cast<ABaseBoxActor>(TargetBox);
	APawn* OwningPawn = GetOwningPlayerPawn();

	if (Box && OwningPawn)
	{
		UBaseInventoryComponent* Inv = OwningPawn->FindComponentByClass<UBaseInventoryComponent>();
		if (Inv && Inv->AddItem(TargetItem))
		{
			// 서버에 박스 아이템 제거 요청
			Box->RemoveItemFromBox(TargetItem);
		}
	}
}