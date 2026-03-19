#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "InventoryDragDropOperation.generated.h"

class ABasePlayerController;

UCLASS()
class PROJECTER_API UInventoryDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 SourceSlotIndex = INDEX_NONE;

	UPROPERTY()
	TObjectPtr<ABasePlayerController> SourcePlayerController = nullptr;

	virtual void DragCancelled_Implementation(const FPointerEvent& PointerEvent) override;
};