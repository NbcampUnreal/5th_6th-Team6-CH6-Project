#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "I_ItemInteractable.h"
#include "BaseItemActor.generated.h"

// 컴포넌트 전방 선언
class USphereComponent;
class UBaseItemData;

UCLASS()
class PROJECTER_API ABaseItemActor : public AActor, public II_ItemInteractable
{
	GENERATED_BODY()

public:
	ABaseItemActor();

protected:
	virtual void BeginPlay() override;

	virtual void PickupItem(APawn* InHandler) override;

	// 아이템 주변 감지용 콜리전
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Interaction")
	TObjectPtr<USphereComponent> InteractionSphere;

	// 부딪혔을 때 실행될 함수
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