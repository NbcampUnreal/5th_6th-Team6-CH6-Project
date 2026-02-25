#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TopDownVision/Public/ObstacleOcclusion/PhysicallOcclusion/OcclusionInterface.h"//interface
#include "ObstacleComp.generated.h"

class UStaticMeshComponent;

/*
 * Visual controller for stylized obstacles.
 * Handles mesh crossfade / fade-out when revealed.
 */

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UObstacleComp 
	: public UActorComponent
	, public IOcclusionInterface
{
	GENERATED_BODY()

public:
	UObstacleComp();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

public:
	/* ---- Interface Implementation ---- */

	virtual void OnOcclusionEnter_Implementation(UActorComponent* SourceComponent) override;
	virtual void OnOcclusionExit_Implementation(UActorComponent* SourceComponent) override;

protected:
	/* ---- Transition ---- */

	void UpdateTransition(float DeltaTime);
	void ApplyVisualState();

protected:
	/* ---- Reveal Tracking ---- */

	// Track active sources safely
	UPROPERTY()
	TSet<TWeakObjectPtr<UActorComponent>> ActiveSources;

	bool IsRevealed() const { return ActiveSources.Num() > 0; }

	UPROPERTY(VisibleAnywhere, Category="Obstacle")
	float CurrentAlpha = 0.f;     // 0 = normal, 1 = occluded

	float TargetAlpha = 0.f;

	UPROPERTY(EditAnywhere, Category="Obstacle")
	float InterpSpeed = 4.f;

protected:
	/* ---- Mesh References ---- */

	// All normal meshes controlled by this obstacle
	UPROPERTY(EditAnywhere, Category="Obstacle")
	TArray<TObjectPtr<UStaticMeshComponent>> NormalMeshes;

	// Optional occluded meshes (crossfade target)
	UPROPERTY(EditAnywhere, Category="Obstacle")
	TArray<TObjectPtr<UStaticMeshComponent>> OccludedMeshes;
};