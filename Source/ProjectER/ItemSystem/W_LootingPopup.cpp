#include "ItemSystem/W_LootingPopup.h"
#include "ItemSystem/BaseItemData.h"
#include "ItemSystem/BaseBoxActor.h"
#include "BaseInventoryComponent.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "CharacterSystem/Player/BasePlayerController.h"

void UW_LootingPopup::InitPopup(const ABaseBoxActor* Box)
{
	TargetBox = Box;

	if (const ABaseBoxActor* Box = TargetBox.Get())
	{
		// 캐스트로 비-const 델리게이트 접근이 필요하면 설계를 조정(아래 참고)
		ABaseBoxActor* Mutable = const_cast<ABaseBoxActor*>(Box);
		Mutable->OnLootChanged.AddUObject(this, &UW_LootingPopup::Refresh);
	}

	Refresh();
}

void UW_LootingPopup::Refresh()
{
	const ABaseBoxActor* Box = TargetBox.Get();
	if (!Box) return;

	UpdateLootingSlots(Box); // Box->GetLootSlots()로 그림
}

void UW_LootingPopup::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}


void UW_LootingPopup::UpdateLootingSlots(const ABaseBoxActor* Box)
{
	if (!TargetBox.IsValid())
	{
		TargetBox = Box;
	}
	else
	{
		// 다른 박스라면 (원한다면) 교체
		if (TargetBox.Get() != Box)
		{
			TargetBox = Box;
		}
	}
	const TArray<FLootSlot>& Items = Box->GetCurrentItemList();

	if (!ItemGridPanel || !SlotWidgetClass) return;

	ItemGridPanel->ClearChildren();
	SlotItemMap.Empty();

	const int32 ColumnCount = 5;

	// 아이템 개수와 상관없이 무조건 10개의 슬롯 위젯을 생성
	for (int32 i = 0; i < 10; ++i)
	{
		UUserWidget* NewSlot = CreateWidget<UUserWidget>(GetOwningPlayer(), SlotWidgetClass);
		if (NewSlot)
		{
			// i번째 칸에 아이템이 있는지 확인
			int32 ItemIndex = Items[i].ItemId;
			bool bHasItem = ItemIndex != -1;
			UBaseItemData* CurrentItem = bHasItem ? Box->ItemPool[ItemIndex] : nullptr;

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
					SlotItemMap.Add(SlotButton, i);
					SlotButton->OnClicked.RemoveAll(this);
					SlotButton->OnClicked.AddDynamic(this, &UW_LootingPopup::OnSlotButtonClicked);
					SlotButton->SetIsEnabled(true);
				}
				else
				{
					// 아이템이 없으면 클릭 비활성화
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

void UW_LootingPopup::TryLootItem(int32 SlotIndex)
{
	if (!TargetBox.IsValid() || SlotIndex == -1)
		return; 

	APawn* OwningPawn = GetOwningPlayerPawn();

	if (ABasePlayerController* PC = GetOwningPlayer<ABasePlayerController>())
	{
		PC->Server_TakeItem(const_cast<ABaseBoxActor*>(TargetBox.Get()), SlotIndex);
	}
}