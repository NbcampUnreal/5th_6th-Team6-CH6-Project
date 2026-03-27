// Fill out your copyright notice in the Description page of Project Settings.


#include "GlobalUtil/StaticGlobalUtils.h"
#include "NavigationSystem.h"


float UStaticGlobalUtils::GetDistanceToActorBounds2D(const AActor* TargetActor, const FVector& FromLocation)
{
	if (!IsValid(TargetActor))
	{
		return TNumericLimits<float>::Max();
	}

	const FBox Bounds = TargetActor->GetComponentsBoundingBox(true);
	if (!Bounds.IsValid)
	{
		return FVector::Dist2D(FromLocation, TargetActor->GetActorLocation());
	}

	const FVector ClosestPoint(
		FMath::Clamp(FromLocation.X, Bounds.Min.X, Bounds.Max.X),
		FMath::Clamp(FromLocation.Y, Bounds.Min.Y, Bounds.Max.Y),
		FMath::Clamp(FromLocation.Z, Bounds.Min.Z, Bounds.Max.Z)
	);

	return FVector::Dist2D(FromLocation, ClosestPoint);
}

FVector UStaticGlobalUtils::GetApproachLocationForActor(UWorld* World, const AActor* TargetActor, const FVector& FromLocation)
{
	if (!IsValid(TargetActor))
	{
		return FromLocation;
	}

	const FBox Bounds = TargetActor->GetComponentsBoundingBox(true);
	if (!Bounds.IsValid)
	{
		return TargetActor->GetActorLocation();
	}

	const FVector Center = Bounds.GetCenter();
	FVector Direction = (FromLocation - Center);
	Direction.Z = 0.f;
	Direction = Direction.GetSafeNormal();

	if (Direction.IsNearlyZero())
	{
		Direction = FVector(1.f, 0.f, 0.f);
	}

	const float StandOff = FMath::Max(Bounds.GetExtent().Size2D(), 80.f) + 100.f;
	FVector DesiredLocation = Center + Direction * StandOff;
	DesiredLocation.Z = FromLocation.Z;

	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
	{
		FNavLocation Projected;
		if (NavSys->ProjectPointToNavigation(DesiredLocation, Projected, FVector(100.f, 100.f, 300.f)))
		{
			return Projected.Location;
		}
	}

	return DesiredLocation;
}
