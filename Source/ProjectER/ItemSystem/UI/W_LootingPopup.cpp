#include "ItemSystem/UI/W_LootingPopup.h"
#include "ItemSystem/Data/BaseItemData.h"
#include "ItemSystem/Actor/BaseBoxActor.h"
#include "ItemSystem/Component/BaseInventoryComponent.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "CharacterSystem/Player/BasePlayerController.h"

#include "UI/UI_MainHUD.h"

void UW_LootingPopup::InitPopup(const ABaseBoxActor* Box)
{
	TargetBox = Box;

	if (const ABaseBoxActor* Box = TargetBox.Get())
	{
		// 캐스트로 비-const 델리게이트 접근이 필요하면 설계를 조정(아래 참고)
		ABaseBoxActor* Mutable = const_cast<ABaseBoxActor*>(Box);
		Mutable->OnLootChanged.AddUObject(this, &UW_LootingPopup::Refresh);
	}

	// 
	
	// 툴팁용 공간
	// ※☆★☆★☆★툴팁은 Refresh() 이전에 설정되어야 함!!!!☆★☆★☆★※
	// Refresh에서 버튼을 생성하는 과정에 툴팁 바인드가 포함되어 있기 때문
#pragma region Tooltip
	// 툴팁 init
	if (IsValid(TooltipClass) && !TooltipInstance)
	{
		TooltipInstance = Cast<UUI_ToolTip>(CreateWidget<UUserWidget>(GetWorld(), TooltipClass));
		TooltipInstance->SetVisibility(ESlateVisibility::Collapsed);
		TooltipInstance->AddToViewport(100); // UI 가시성 우선순위 위로
	}
	if (!TooltipManager)
	{
		TooltipManager = NewObject<UUI_ToolTipManager>(this);
		TooltipManager->setTooltipInstance(TooltipInstance);
	}
	// ※☆★☆★☆★툴팁은 Refresh() 이전에 설정되어야 함!!!!☆★☆★☆★※
#pragma endregion

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

					/// 툴팁 바인드 ///
					SlotButton->OnHovered.AddDynamic(this, &UW_LootingPopup::OnItemHovered);
					SlotButton->OnUnhovered.AddDynamic(this, &UW_LootingPopup::HideTooltip);
					/// 툴팁 바인드 ///
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

#pragma region Tooltip

void UW_LootingPopup::OnItemHovered()
{
	for (auto& Elem : SlotItemMap)
	{
		UButton* Btn = Elem.Key;
		UE_LOG(LogTemp, Error, TEXT("ssssssssss"));
		UE_LOG(LogTemp, Error, TEXT("Btn : %s"), *Btn->GetName());
		if (Btn->IsHovered())
		{
			// Todo:
			/// 버튼 정보의 아이템 데이터를 읽어와서 툴팁에 전달 하도록 추후 업데이트 예정///
			if (TooltipManager)
			{
				TooltipManager->ShowTooltip(
					Btn,
					nullptr,
					FText::FromString("Item Name"),
					FText::FromString("Short Description"),
					FText::FromString("Detailed Description goes here."),
					true
				);
			}
		}
	}


	// 차후 데이터 애셋에서 정보를 읽어올 수 있도록 개선해야 함

}

void UW_LootingPopup::HideTooltip()
{
	if (IsValid(TooltipInstance))
		TooltipInstance->SetVisibility(ESlateVisibility::Collapsed);
}

#pragma endregion

