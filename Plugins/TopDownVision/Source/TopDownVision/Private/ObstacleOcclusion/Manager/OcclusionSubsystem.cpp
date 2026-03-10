// Fill out your copyright notice in the Description page of Project Settings.


#include "ObstacleOcclusion/Manager/OcclusionSubsystem.h"

#include "Components/PrimitiveComponent.h"

DEFINE_LOG_CATEGORY(OcclusionSubsystem);

void UOcclusionSubsystem::RegisterTarget(UPrimitiveComponent* PrimComp, float RadiusPadding)
{
	if (!IsValid(PrimComp)) return;

	for (const FOcclusionBrushTarget& T : Targets)
	{
		if (T.PrimitiveComp == PrimComp) return;
	}

	FOcclusionBrushTarget Target;
	Target.PrimitiveComp = PrimComp;
	Target.RadiusPadding = RadiusPadding;
	Targets.Add(Target);

	UE_LOG(OcclusionSubsystem, Log,
		TEXT("UOcclusionSubsystem::RegisterTarget>> %s | Total: %d"),
		*PrimComp->GetOwner()->GetName(), Targets.Num());
}

void UOcclusionSubsystem::UnregisterTarget(UPrimitiveComponent* PrimComp)
{
	if (!IsValid(PrimComp)) return;

	Targets.RemoveAll([PrimComp](const FOcclusionBrushTarget& T)
	{
		return T.PrimitiveComp == PrimComp;
	});

	UE_LOG(OcclusionSubsystem, Log,
		TEXT("UOcclusionSubsystem::UnregisterTarget>> %s | Total: %d"),
		*PrimComp->GetOwner()->GetName(), Targets.Num());
}