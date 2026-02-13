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