#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "I_ItemInteractable.generated.h"

UINTERFACE(MinimalAPI)
class UI_ItemInteractable : public UInterface
{
	GENERATED_BODY()
};

class PROJECTER_API II_ItemInteractable
{
	GENERATED_BODY()

public:
	virtual void PickupItem(class APawn* InHandler) = 0;
};