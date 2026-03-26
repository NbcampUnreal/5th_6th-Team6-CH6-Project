// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OcclusionBinderSubsystem.generated.h"

class UPrimitiveComponent;
class AOcclusionBinder;

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(OcclusionBinderSubsystem, Log, All);

UCLASS()
class TOPDOWNVISION_API UOcclusionBinderSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Called by AOcclusionBinder at BeginPlay for each primitive on its bound actors
	void RegisterBinderPrimitive(UPrimitiveComponent* Prim, AOcclusionBinder* Binder);

	// Called by AOcclusionBinder at EndPlay — removes all its registered primitives
	void UnregisterBinder(AOcclusionBinder* Binder);

	// Called by FindOwningOcclusionComp as final fallback
	// Returns the binder that owns the given primitive, or nullptr
	AOcclusionBinder* FindBinder(UPrimitiveComponent* Prim) const;


	// Returns a recycled MID for Parent, or creates a new one.
	// Outer is only used when creating — any stable UObject works (e.g. the mesh).
	UMaterialInstanceDynamic* CheckoutMID(UMaterialInterface* Parent, UObject* Outer);

	void ReturnMID(UMaterialInstanceDynamic* MID);

	// Returns true if this material is registered for pooling
	static bool IsMaterialPooled(const UMaterialInterface* Material);
	
	void PrePopulatePool();// populate the mid in pool
	void CommitGenocide();// kill them all

	void TrimOverflowPool();// clean the overflowed mids

private:

	// Primitive → binder pair — key is hit primitive, value is owning binder
	TMap<TWeakObjectPtr<UPrimitiveComponent>, TWeakObjectPtr<AOcclusionBinder>> PrimitiveToBinderMap;
	
	TMap<TObjectPtr<UMaterialInterface>, TArray<TObjectPtr<UMaterialInstanceDynamic>>> MIDPool;

	// GC anchor — prevents the engine from collecting idle pooled MIDs
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> AllPooledMIDs;

	// Over-cap MIDs waiting for GC — trimmed periodically
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> OverflowMIDs;

	FTimerHandle TrimTimerHandle;
};