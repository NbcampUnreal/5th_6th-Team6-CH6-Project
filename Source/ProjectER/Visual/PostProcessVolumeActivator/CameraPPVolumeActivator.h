#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraPPVolumeActivator.generated.h"

class APostProcessVolume;
class UTopDownCameraComp;

USTRUCT()
struct FPPVolumeRuntime
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<APostProcessVolume> Volume = nullptr;

	float TargetWeight = 0.f;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTER_API UCameraPPVolumeActivator : public UActorComponent
{
	GENERATED_BODY()

public:
	UCameraPPVolumeActivator();

protected:

	virtual void BeginPlay() override;

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

public:

	UFUNCTION(BlueprintCallable, Category="Post Process Volume")
	void Initialize(UTopDownCameraComp* InCameraComp);

protected:

	// How often overlap detection runs
	UPROPERTY(EditDefaultsOnly, Category="Post Process Volume")
	float UpdateInterval = 0.1f;

	// Blend interpolation speed
	UPROPERTY(EditAnywhere, Category="Post Process Volume")
	float BlendSpeed = 8.f;

private:

	UPROPERTY()
	TObjectPtr<UTopDownCameraComp> CameraComp;

	UPROPERTY()
	TArray<FPPVolumeRuntime> ManagedVolumes;

	float DetectionTimer = 0.f;

	void CacheVolumes();

	void UpdateVolumes();
};