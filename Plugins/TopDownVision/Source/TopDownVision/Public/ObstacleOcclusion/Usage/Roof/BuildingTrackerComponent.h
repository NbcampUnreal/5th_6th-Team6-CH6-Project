#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuildingTrackerComponent.generated.h"

class ABuildingOcclusionManager;

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(BuildingTracker, Log, All);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UBuildingTrackerComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UBuildingTrackerComponent();

	virtual void BeginPlay() override;

	// ── Config ────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, Category="Building Tracker")
	TEnumAsByte<ECollisionChannel> InteriorTraceChannel = ECC_GameTraceChannel2;

	UPROPERTY(EditAnywhere, Category="Building Tracker")
	float TraceLength = 300.f;

	UPROPERTY(EditAnywhere, Category="Building Tracker")
	float TraceInterval = 0.1f;

	// ── API ───────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category="Building Tracker")
	void SetTrackerActivation(bool bActivate);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Building Tracker")
	bool IsInitialized() const { return bInitialized; }

	UFUNCTION(BlueprintCallable, Category="Building Tracker")
	void StartTrackerLoop();

	UFUNCTION(BlueprintCallable, Category="Building Tracker")
	void StopTrackerLoop();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Building Tracker")
	bool IsTrackerLoopActive() const;

private:

	void UpdateBuildingState();
	void OnEnteredRoom(ABuildingOcclusionManager* Manager) const;
	void OnExitedRoom(ABuildingOcclusionManager* Manager) const;

	bool bInitialized = false;

	// Cached proxy actor — compared each update to detect area change
	TWeakObjectPtr<AActor> LastHitProxyActor=nullptr;

	TWeakObjectPtr<ABuildingOcclusionManager> LastBuildingManager=nullptr;

	FTimerHandle TrackerTimerHandle;
};