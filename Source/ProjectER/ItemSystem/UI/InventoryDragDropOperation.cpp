#include "ItemSystem/UI/InventoryDragDropOperation.h"
#include "CharacterSystem/Player/BasePlayerController.h"

void UInventoryDragDropOperation::DragCancelled_Implementation(const FPointerEvent& PointerEvent)
{
	Super::DragCancelled_Implementation(PointerEvent);

	if (SourceSlotIndex == INDEX_NONE || !SourcePlayerController)
	{
		return;
	}

	SourcePlayerController->RequestDropInventoryItemFromUI(
		SourceSlotIndex,
		PointerEvent.GetScreenSpacePosition());
}