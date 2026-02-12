// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LineOfSight/Management/VisionProviderInterface.h"
#include "VisionGameStateComp.generated.h"

/**
 * GameState component to manage shared vision state per team
 */
UCLASS(ClassGroup=(Vision), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UVisionGameStateComp : public UActorComponent
{
	GENERATED_BODY()

public:
	UVisionGameStateComp();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	// Server-only logic guard
	bool CanRunServerLogic() const;

	// ---------------- Server logic ----------------
	void UpdateVision();

	// Called on clients when replication happens
	UFUNCTION()
	void OnRep_VisionUpdated();

public:
	// ---------------- Registration ----------------
	UFUNCTION(BlueprintCallable, Category="LineOfSight")
	void RegisterVisionProvider(TScriptInterface<IVisionProviderInterface> Provider);

	UFUNCTION(BlueprintCallable, Category="LineOfSight")
	void UnregisterVisionProvider(TScriptInterface<IVisionProviderInterface> Provider);

	// ---------------- Queries ----------------
	UFUNCTION(BlueprintCallable, Category="LineOfSight")
	bool IsActorVisibleToTeam(uint8 TeamChannel, AActor* Target) const;

private:
	
};
