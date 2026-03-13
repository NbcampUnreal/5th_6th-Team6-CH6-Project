#pragma once

#include "CoreMinimal.h"
#include "OcclusionTraceTypes.generated.h"

USTRUCT(BlueprintType)
struct FOcclusionSweepConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sweep")
	FVector OriginOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sweep")
	float SphereRadius = 20.f;
};

USTRUCT(BlueprintType)
struct FOcclusionProbe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Probe")
	TArray<FOcclusionSweepConfig> Sweeps;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Probe")
	FVector BaseOrigin = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Probe")
	FVector Target = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Probe")
	TEnumAsByte<ECollisionChannel> Channel = ECC_GameTraceChannel1;

	// Runtime state — not a UPROPERTY, not serialized
	TSet<TWeakObjectPtr<AActor>> PreviousHits;

	TSet<int32> HitSweepIndices;
};