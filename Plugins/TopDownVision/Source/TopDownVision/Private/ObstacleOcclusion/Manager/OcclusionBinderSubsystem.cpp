// Fill out your copyright notice in the Description page of Project Settings.


#include "ObstacleOcclusion/Manager/OcclusionBinderSubsystem.h"
#include "ObstacleOcclusion/Binder/OcclusionBinder.h"

DEFINE_LOG_CATEGORY(OcclusionBinderSubsystem);

void UOcclusionBinderSubsystem::RegisterBinderPrimitive(UPrimitiveComponent* Prim, AOcclusionBinder* Binder)
{
	if (!IsValid(Prim) || !IsValid(Binder)) return;

	PrimitiveToBinderMap.Add(Prim, Binder);

	UE_LOG(OcclusionBinderSubsystem, Log,
		TEXT("UOcclusionBinderSubsystem::RegisterBinderPrimitive>> %s → %s"),
		*Prim->GetName(),
		*Binder->GetName());
}

void UOcclusionBinderSubsystem::UnregisterBinder(AOcclusionBinder* Binder)
{
	if (!IsValid(Binder)) return;

	// Remove all primitives that point to this binder
	for (auto It = PrimitiveToBinderMap.CreateIterator(); It; ++It)
	{
		if (It.Value() == Binder)
			It.RemoveCurrent();
	}

	UE_LOG(OcclusionBinderSubsystem, Log,
		TEXT("UOcclusionBinderSubsystem::UnregisterBinder>> %s unregistered"),
		*Binder->GetName());
}

AOcclusionBinder* UOcclusionBinderSubsystem::FindBinder(UPrimitiveComponent* Prim) const
{
	if (!IsValid(Prim)) return nullptr;

	const TWeakObjectPtr<AOcclusionBinder>* Found = PrimitiveToBinderMap.Find(Prim);
	if (!Found || !Found->IsValid()) return nullptr;

	return Found->Get();
}