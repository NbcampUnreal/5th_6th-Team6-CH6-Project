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

	// Called by AOcclusionBinder at BeginPlay for each primitive on its bound actors
	void RegisterBinderPrimitive(UPrimitiveComponent* Prim, AOcclusionBinder* Binder);

	// Called by AOcclusionBinder at EndPlay — removes all its registered primitives
	void UnregisterBinder(AOcclusionBinder* Binder);

	// Called by FindOwningOcclusionComp as final fallback
	// Returns the binder that owns the given primitive, or nullptr
	AOcclusionBinder* FindBinder(UPrimitiveComponent* Prim) const;

private:

	// Primitive → binder pair — key is hit primitive, value is owning binder
	TMap<TWeakObjectPtr<UPrimitiveComponent>, TWeakObjectPtr<AOcclusionBinder>> PrimitiveToBinderMap;
};