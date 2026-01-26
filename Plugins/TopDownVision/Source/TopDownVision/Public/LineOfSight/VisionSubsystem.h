// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "VisionSubsystem.generated.h"


//forward declare
class ULineOfSightComponent;// the source of the texture




/**
 * 
 */
//Log
TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(VisionSubsystem, Log, All);

UCLASS()
class TOPDOWNVISION_API UVisionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	void RegisterProvider(ULineOfSightComponent* Provider);
	void UnregisterProvider(ULineOfSightComponent* Provider);

private:
	UPROPERTY()
	TArray<TObjectPtr<ULineOfSightComponent>> RegisteredProviders;
};
