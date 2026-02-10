// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VisionManager.generated.h"


class UObstacleSubsystem;
class IVisionProviderInterface;

USTRUCT()
struct FTeamVisionData
{
	GENERATED_BODY()

	// Not replicated directly (derived data)
	UPROPERTY()
	TSet<TWeakObjectPtr<UObject>> Providers;

	// Replicated result
	UPROPERTY()
	TSet<TWeakObjectPtr<AActor>> VisibleActors;
};

UCLASS(ClassGroup=(Vision), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UVisionManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UVisionManager();

	// === Registration ===
	void RegisterVisionProvider(TScriptInterface<IVisionProviderInterface> Provider);
	void UnregisterVisionProvider(TScriptInterface<IVisionProviderInterface> Provider);

	// === Queries ===
	bool IsActorVisibleToTeam(uint8 TeamId, AActor* Target) const;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	// Server-only working data
	UPROPERTY()
	TMap<uint8, FTeamVisionData> TeamVision;

	// Replicated vision results (flattened)
	UPROPERTY(ReplicatedUsing=OnRep_VisionUpdated)
	TMap<uint8, TArray<AActor*>> ReplicatedVisibleActors;

	UFUNCTION()
	void OnRep_VisionUpdated();

	// Internal
	void UpdateVision();

	bool CanRunServerLogic();
};