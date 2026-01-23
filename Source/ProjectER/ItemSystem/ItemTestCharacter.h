#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ItemTestCharacter.generated.h"

UCLASS()
class PROJECTER_API AItemTestCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AItemTestCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Inventory")
	class UBaseInventoryComponent* Inventory;
};