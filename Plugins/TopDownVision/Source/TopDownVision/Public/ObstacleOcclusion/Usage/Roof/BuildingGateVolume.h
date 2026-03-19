#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuildingGateVolume.generated.h"

class UBoxComponent;
class UBuildingTrackerComponent;

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(BuildingGateVolume, Log, All);

UCLASS()
class TOPDOWNVISION_API ABuildingGateVolume : public AActor
{
	GENERATED_BODY()

public:

	ABuildingGateVolume();

	virtual void BeginPlay() override;

private:

	UPROPERTY(VisibleAnywhere, Category="Building Gate")
	TObjectPtr<UBoxComponent> OverlapVolume;

	UFUNCTION()
	void OnOverlapBegin(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);
};