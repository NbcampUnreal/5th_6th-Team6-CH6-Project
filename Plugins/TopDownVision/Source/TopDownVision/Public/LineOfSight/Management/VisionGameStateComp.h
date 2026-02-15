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


//Requirements

/*USTRUCT()
struct FVisionVisibleEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

public:

	// The actor being tracked
	UPROPERTY()
	AActor* TargetActor = nullptr;

	// Bitmask of teams that can currently see this actor
	UPROPERTY()
	uint8 VisibilityMask = 0;
};

USTRUCT()
struct FVisionVisibleContainer : public FFastArraySerializer
{
	GENERATED_BODY()

public:

	UPROPERTY()
	TArray<FVisionVisibleEntry> Entries;

	// Required for delta replication
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FVisionVisibleEntry, FVisionVisibleContainer>(
			Entries, DeltaParams, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FVisionVisibleContainer> : public TStructOpsTypeTraitsBase2<FVisionVisibleContainer>
{
	enum
	{
		WithNetDeltaSerializer = true
	};
};*/

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
	
/*private:

	//Entry helper function
	FVisionVisibleEntry* FindEntry(AActor* Target);
	FVisionVisibleEntry& CreateEntry(AActor* Target);
	
	// Server-only logic guard
	bool CanRunServerLogic() const;

	// ---------------- Server logic ----------------
	void UpdateVision();

	// Called on clients when replication happens
	UFUNCTION()
	void OnRep_VisionUpdated();

public:

	// Mark actor visible to a team
	void SetActorVisibleToTeam(uint8 TeamID, AActor* Target);
	// Remove visibility for a team
	void ClearActorVisibleToTeam(uint8 TeamID, AActor* Target);
	
	UFUNCTION(BlueprintCallable, Category="LineOfSight")
	bool IsActorVisibleToTeam(uint8 TeamID, AActor* Target) const;

	


private:

	//Replicated Vision Containers
	UPROPERTY(Replicated)
	FVisionVisibleContainer VisionContainer;

	// Server-only list of providers (not replicated)
	UPROPERTY()
	TArray<TScriptInterface<IVisionProviderInterface>> RegisteredProviders;*/
};
