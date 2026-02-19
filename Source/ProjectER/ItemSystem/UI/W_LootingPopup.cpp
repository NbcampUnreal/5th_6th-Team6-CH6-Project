// W_LootingPopup.cpp
#include "ItemSystem/UI/W_LootingPopup.h"
#include "ItemSystem/Data/BaseItemData.h"
#include "ItemSystem/Actor/BaseBoxActor.h"
#include "ItemSystem/Component/BaseInventoryComponent.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "CharacterSystem/Player/BasePlayerController.h"
#include "UI/UI_MainHUD.h"	//	툴팁용

// LootableComponent 지원
#include "ItemSystem/Component/LootableComponent.h"

void UW_LootingPopup::InitPopup(const AActor* Box)
{
	TargetBox = Box;


	// LootableComponent 먼저 확인
	ULootableComponent* LootComp = TargetBox->FindComponentByClass<ULootableComponent>();

	if (LootComp)
	{
		// LootableComponent 델리게이트 등록
		LootComp->OnLootChanged.AddUObject(this, &UW_LootingPopup::Refresh);
	}
	else
	{
		// 기존 BaseBoxActor 방식
		// 캐스트로 비-const 델리게이트 접근이 필요하면 설계를 조정(아래 참고)
		//ABaseBoxActor* Mutable = const_cast<ABaseBoxActor*>(ValidBox);
		//Mutable->OnLootChanged.AddUObject(this, &UW_LootingPopup::Refresh);
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
	const AActor* Box = TargetBox;
	if (!Box) return;

	UpdateLootingSlots(Box); // Box->GetLootSlots()로 그림
}

void UW_LootingPopup::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

/// <summary>
/// mpyi _ 툴팁 정상 해제를 위해 소멸자 추가
/// </summary>
void UW_LootingPopup::NativeDestruct()
{
	Super::NativeDestruct();

	TooltipInstance->RemoveFromParent();
	TooltipInstance = nullptr;
}


void UW_LootingPopup::UpdateLootingSlots(const AActor* Box)
{
	if (!IsValid(TargetBox))
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

	// LootableComponent에서 데이터 가져오기
	TArray<FLootSlot> Items;
	TArray<TObjectPtr<UBaseItemData>> ItemPool;

	ULootableComponent* LootComp = Box->FindComponentByClass<ULootableComponent>();

	if (LootComp)
	{
		// LootableComponent 사용
		Items = LootComp->GetCurrentItemList();
		ItemPool = LootComp->ItemPool;
	}
	else
	{
		//// 기존 BaseBoxActor 사용
		//Items = Box->GetCurrentItemList();  //// 임시 주석
		//ItemPool = Box->ItemPool;
	}

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
			// 안전한 배열 접근
			// i번째 칸에 아이템이 있는지 확인
			int32 ItemIndex = (i < Items.Num()) ? Items[i].ItemId : -1;
			bool bHasItem = (ItemIndex != -1) && (ItemIndex < ItemPool.Num());
			UBaseItemData* CurrentItem = bHasItem ? ItemPool[ItemIndex].Get() : nullptr;

			UImage* TargetImage = Cast<UImage>(NewSlot->GetWidgetFromName(TEXT("ItemIconImage")));
			UButton* SlotButton = Cast<UButton>(NewSlot->GetWidgetFromName(TEXT("SlotButton")));

			if (TargetImage)
			{
				if (bHasItem && CurrentItem)
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
				if (bHasItem && CurrentItem)
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
			HideTooltip();	// 아이템 먹으면 툴팁 안보이게
			return;
		}
	}
}

void UW_LootingPopup::TryLootItem(int32 SlotIndex)
{
	if (!IsValid(TargetBox) || SlotIndex == -1)
		return;

	APawn* OwningPawn = GetOwningPlayerPawn();

	ABasePlayerController* PC = GetOwningPlayer<ABasePlayerController>();
	if (!PC)
		return;

	const AActor* TargetActor = TargetBox;

	// LootableComponent가 있으면 새 RPC 사용
	ULootableComponent* LootComp = TargetActor->FindComponentByClass<ULootableComponent>();

	if (LootComp)
	{
		// LootableComponent를 사용하는 액터 - 새 RPC 사용
		PC->Server_TakeItemFromActor(TargetActor, SlotIndex);
	}
	else
	{
		//// 기존 BaseBoxActor - 기존 RPC 사용
		//ABaseBoxActor* BoxActor = Cast<ABaseBoxActor>(TargetActor);
		//if (BoxActor)
		//{
		//	PC->Server_TakeItem(BoxActor, SlotIndex);
		//}
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
