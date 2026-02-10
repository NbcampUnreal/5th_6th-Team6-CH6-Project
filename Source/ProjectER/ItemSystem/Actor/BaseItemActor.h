#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemSystem/Interface/I_ItemInteractable.h"
#include "BaseItemActor.generated.h"

class USphereComponent;
class UBaseItemData;

UCLASS()
class PROJECTER_API ABaseItemActor : public AActor, public II_ItemInteractable
{
	GENERATED_BODY()

public:
	ABaseItemActor();

public:
	virtual void BeginPlay() override;

	// 상호작용 인터페이스 구현
	virtual void PickupItem(APawn* InHandler) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Interaction")
	TObjectPtr<USphereComponent> InteractionSphere;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Data")
	TObjectPtr<UBaseItemData> ItemData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Mesh")
	TObjectPtr<UStaticMeshComponent> ItemMesh;
};