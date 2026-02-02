#include "ItemSystem/W_LootingPopup.h"
#include "ItemSystem/BaseItemData.h"
#include "ItemSystem/BaseBoxActor.h"
#include "BaseInventoryComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/Image.h"
#include "Blueprint/UserWidget.h"

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
		if (OwningPawn)
		{
			float CurrentDistance = OwningPawn->GetDistanceTo(TargetBox);
			if (CurrentDistance > MaxDistance)
			{
				RemoveFromParent();
			}
		}
	}
}

void UW_LootingPopup::UpdateLootingSlots(const TArray<UBaseItemData*>& Items)
{
	if (!ItemGridPanel || !SlotWidgetClass) return;

	ItemGridPanel->ClearChildren();

	const int32 TotalSlots = 10;
	const int32 ColumnCount = 5;

	for (int32 i = 0; i < TotalSlots; ++i)
	{
		UUserWidget* NewSlot = CreateWidget<UUserWidget>(GetOwningPlayer(), SlotWidgetClass);
		if (NewSlot)
		{
			// 해당 인덱스에 아이템이 있는지 확인
			bool bHasItem = Items.IsValidIndex(i) && Items[i] != nullptr;

			// 데이터 주입 (아이템이 있을 때만)
			if (bHasItem)
			{
				FProperty* Property = NewSlot->GetClass()->FindPropertyByName(TEXT("ItemData"));
				if (Property)
				{
					if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
					{
						ObjProp->SetObjectPropertyValue(Property->ContainerPtrToValuePtr<void>(NewSlot), Items[i]);
					}
				}
			}

			// 아이콘 세팅 (아이템이 있으면 아이콘, 없으면 투명/숨김)
			UImage* TargetImage = Cast<UImage>(NewSlot->GetWidgetFromName(TEXT("ItemIconImage")));
			if (TargetImage)
			{
				if (bHasItem && Items[i]->ItemIcon.IsValid())
				{
					UTexture2D* LoadedTexture = Items[i]->ItemIcon.LoadSynchronous();
					if (LoadedTexture)
					{
						TargetImage->SetBrushFromTexture(LoadedTexture);
						TargetImage->SetVisibility(ESlateVisibility::Visible);
					}
				}
				else
				{
					// 빈 슬롯은 아이콘을 숨김
					TargetImage->SetVisibility(ESlateVisibility::Hidden);
				}
			}

			// 그리드 배치 (5열 기준)
			UUniformGridSlot* GridSlot = ItemGridPanel->AddChildToUniformGrid(NewSlot);
			if (GridSlot)
			{
				GridSlot->SetRow(i / ColumnCount);
				GridSlot->SetColumn(i % ColumnCount);
			}
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

			UpdateLootingSlots(Box->GetCurrentLoot());

			if (Box->GetCurrentLoot().Num() == 0)
			{
				RemoveFromParent();
			}
		}
	}
}