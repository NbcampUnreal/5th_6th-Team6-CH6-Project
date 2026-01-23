#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemSystem/I_ItemInteractable.h" // 방금 만든 인터페이스 포함
#include "BaseItemActor.generated.h"

class UStaticMeshComponent;
class UBaseItemData;

UCLASS()
class PROJECTER_API ABaseItemActor : public AActor, public II_ItemInteractable // 인터페이스 상속
{
	GENERATED_BODY()

public:
	ABaseItemActor();

	virtual void PickupItem(class APawn* InHandler) override; //

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Component")
	UStaticMeshComponent* ItemMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Data")
	UBaseItemData* ItemData;
};