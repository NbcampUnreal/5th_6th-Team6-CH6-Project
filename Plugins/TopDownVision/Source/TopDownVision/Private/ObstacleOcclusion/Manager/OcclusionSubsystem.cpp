#include "ObstacleOcclusion/Manager/OcclusionSubsystem.h"
#include "Components/PrimitiveComponent.h"

DEFINE_LOG_CATEGORY(OcclusionSubsystem);

void UOcclusionSubsystem::RegisterTarget(UPrimitiveComponent* PrimComp, UMaterialInterface* BrushMat, float RadiusPadding)
{
	if (!IsValid(PrimComp)) return;

	// Prevent duplicate registration
	for (const FOcclusionBrushTarget& T : Targets)
	{
		if (T.PrimitiveComp == PrimComp) return;
	}

	FOcclusionBrushTarget Target;
	Target.PrimitiveComp = PrimComp;
	Target.RadiusPadding = RadiusPadding;
	Target.BrushMaterial = BrushMat; // nullptr is valid — painter default used as fallback

	Targets.Add(Target);

	UE_LOG(OcclusionSubsystem, Log,
		TEXT("UOcclusionSubsystem::RegisterTarget>> %s | Material: %s | Total: %d"),
		*PrimComp->GetOwner()->GetName(),
		BrushMat ? *BrushMat->GetName() : TEXT("None (painter default)"),
		Targets.Num());
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