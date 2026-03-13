#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelManagement/LevelAreaTrackerComponent.h"
#include "LevelManagement/Requirements/LevelAreaData.h"
#include "LevelManagement/Area/LevelAreaNode.h"
#include "LevelAreaActor.generated.h"

UCLASS()
class PROJECTER_API ALevelAreaActor : public AActor
{
	GENERATED_BODY()

public:

	ALevelAreaActor();

	/* ---------- Config ---------- */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area")
	int32 NodeID = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area")
	TArray<int32> NeighborNodeIDs;

	// Physical material to register — stamped onto child meshes via ApplyMaterialToMeshes()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area|Surface")
	TObjectPtr<UPhysicalMaterial> FloorMaterial = nullptr;


	/* ---------- Editor Utility ---------- */

	// Stamps FloorMaterial onto every StaticMeshComponent parented to this actor.
	// Call this in the editor after setting FloorMaterial.
	UFUNCTION(CallInEditor, Category="Area|Surface")
	void ApplyMaterialToMeshes();


	/* ---------- Replicated State ---------- */

	UPROPERTY(ReplicatedUsing=OnRep_HazardState, BlueprintReadOnly, Category="Area")
	EAreaHazardState HazardState = EAreaHazardState::Safe;

	UPROPERTY(BlueprintAssignable, Category="Area")
	FOnAreaHazardStateChanged OnHazardStateChanged; 


	/* ---------- Server API ---------- */

	UFUNCTION(BlueprintCallable, Category="Area", BlueprintAuthorityOnly)
	void SetHazardState(EAreaHazardState NewState);


	/* ---------- Lifecycle ---------- */

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;


private:

	TUniquePtr<LevelAreaNode> OwnedNode;

	void RegisterWithSubsystem();
	void UnregisterFromSubsystem();

	UFUNCTION()
	void OnRep_HazardState();
};