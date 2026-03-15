#include "ObstacleOcclusion/Manager/OcclusionSubsystem.h"
#include "Components/PrimitiveComponent.h"

DEFINE_LOG_CATEGORY(OcclusionSubsystem);

int32 UOcclusionSubsystem::RegisterTarget(UPrimitiveComponent* PrimComp, UMaterialInterface* BrushMat, float VisionHalfRadius)
{
	if (!IsValid(PrimComp)) return INDEX_NONE;

	for (int32 i = 0; i < Targets.Num(); ++i)
	{
		if (Targets[i].PrimitiveComp == PrimComp) return i; // already registered
	}

	FOcclusionBrushTarget Target;
	Target.PrimitiveComp = PrimComp;
	Target.VisibleRadius = VisionHalfRadius*2.f;
	Target.BrushMaterial = BrushMat;

	const int32 Index = Targets.Add(Target);

	UE_LOG(OcclusionSubsystem, Log,
		TEXT("UOcclusionSubsystem::RegisterTarget>> %s | Index: %d | Total: %d"),
		*PrimComp->GetOwner()->GetName(), Index, Targets.Num());

	return Index;
}

void UOcclusionSubsystem::UpdateTargetByIndex(int32 Index, float NewRevealAlpha, float NewVisionHalfRadius)
{
	if (!Targets.IsValidIndex(Index)) return;

	Targets[Index].RevealAlpha = FMath::Clamp(NewRevealAlpha, 0.f, 1.f);

	if (NewVisionHalfRadius >= 0.f)
		Targets[Index].VisibleRadius = NewVisionHalfRadius*2.f;
}

void UOcclusionSubsystem::UnregisterTarget(UPrimitiveComponent* PrimComp)
{
	if (!IsValid(PrimComp)) return;

	for (int32 i = Targets.Num() - 1; i >= 0; --i)
	{
		if (Targets[i].PrimitiveComp != PrimComp) continue;

		Targets.RemoveAtSwap(i);

		UE_LOG(OcclusionSubsystem, Log,
			TEXT("UOcclusionSubsystem::UnregisterTarget>> %s | Total: %d"),
			*PrimComp->GetOwner()->GetName(), Targets.Num());

		return;
	}
}