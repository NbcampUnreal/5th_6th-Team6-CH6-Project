// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"// no need for array update, just update the entering and exiting target
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






public:	
	// ---------------- Registration ----------------
	UFUNCTION(BlueprintCallable, Category="LineOfSight")
	void RegisterVisionProvider(TScriptInterface<IVisionProviderInterface> Provider);
	UFUNCTION(BlueprintCallable, Category="LineOfSight")
	void UnregisterVisionProvider(TScriptInterface<IVisionProviderInterface> Provider);


	//Server

	void SetActorVisibleToTeam(uint8 Team, AActor* Target);
	void ClearActorVisibleToTeam(uint8 Team, AActor* Target);

};
