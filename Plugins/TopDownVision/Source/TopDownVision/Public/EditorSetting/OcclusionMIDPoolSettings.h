// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "OcclusionMIDPoolSettings.generated.h"


UCLASS(config=Game, defaultconfig, meta=(DisplayName="Occlusion MID Pool"))
class TOPDOWNVISION_API UOcclusionMIDPoolSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UOcclusionMIDPoolSettings()
	{
		CategoryName = TEXT("TopDownVision");
		SectionName  = TEXT("Occlusion MID Pool");

		MaxPooledMIDsPerMaterial = 30;
		PreWarmCountPerMaterial  = 10;
	}

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category="Pool", meta=(ClampMin=1, ClampMax=64))
	int32 MaxPooledMIDsPerMaterial;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category="Pool", meta=(ClampMin=0, ClampMax=64))
	int32 PreWarmCountPerMaterial;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category="Pool|PreWarm")
	TArray<TSoftObjectPtr<UMaterialInterface>> PreWarmMaterials;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category="Pool", meta=(ClampMin=1.f))
	float OverflowTrimInterval = 60.f;
};

UCLASS(config=Game, defaultconfig, meta=(DisplayName="Occlusion Tags"))
class TOPDOWNVISION_API UOcclusionTagSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UOcclusionTagSettings()
	{
		CategoryName = TEXT("TopDownVision");
		SectionName  = TEXT("Occlusion Tags");
	}

	// Mesh Tags
	
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category="Tags")
	FName NormalMeshTag = TEXT("OcclusionMesh");

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category="Tags")
	FName OccludedMeshTag = TEXT("OccludedVisual");

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category="Tags")
	FName RTMaterialTag = TEXT("RTOcclusionMesh");

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category="Tags")
	FName NoShadowProxyTag = TEXT("NoShadowProxy");

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category="Tags")
	FName OcclusionTypeSwitchTag = TEXT("IsRTorPhysical");

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category="Tags")
	FName OcclusionLockTag = TEXT("ShouldOcclude");

	// Material Params

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category="Parameters")
	FName AlphaParameterName = TEXT("OcclusionAlpha");

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category="Parameters")
	FName ForceOccludeParameterName = TEXT("FullOcclusionAlpha");

	// TraceChannel

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category="TraceChannel")
	TEnumAsByte<ECollisionChannel> OcclusionTraceChannel = ECC_GameTraceChannel1;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category="TraceChannel")
	TEnumAsByte<ECollisionChannel> MouseTraceChannel = ECC_Visibility;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category="TraceChannel")
	TEnumAsByte<ECollisionChannel> InteriorTraceChannel = ECC_GameTraceChannel2;
};