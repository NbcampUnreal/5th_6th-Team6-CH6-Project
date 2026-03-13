#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelAreaBridgeVolume.generated.h"

class UBoxComponent;

UCLASS()
class PROJECTER_API ALevelAreaBridgeVolume : public AActor
{
	GENERATED_BODY()

public:

	ALevelAreaBridgeVolume();

protected:

	virtual void BeginPlay() override;

private:

	UPROPERTY(VisibleAnywhere, Category="Bridge")
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