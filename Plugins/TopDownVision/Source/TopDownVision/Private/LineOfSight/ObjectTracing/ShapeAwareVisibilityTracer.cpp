// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/ObjectTracing/ShapeAwareVisibilityTracer.h"

//Shapes
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"

// Debug
#include "DrawDebugHelpers.h"

//Log
DEFINE_LOG_CATEGORY(VisibilityTrace);

bool UShapeAwareVisibilityTracer::IsTargetVisible(UWorld* ContextWorld, const FVector& ObserverLocation,
	UPrimitiveComponent* TargetShape, float MaxDistance, ECollisionChannel ObstacleChannel,
	const TArray<AActor*>& IgnoredActors,bool bDrawDebugLine, float RayGapDegrees)
{
	if (!ContextWorld || !TargetShape)
	{
		UE_LOG(VisibilityTrace, Verbose,
			TEXT("UShapeAwareVisibilityTracer::IsTargetVisible >> Invalid world or target"));
		return false;
	}

	float LeftRad = 0.f;
	float RightRad = 0.f;

	if (!ComputeAngularSpan(ObserverLocation, TargetShape, LeftRad, RightRad))
	{
		UE_LOG(VisibilityTrace, Verbose,
			TEXT("UShapeAwareVisibilityTracer::IsTargetVisible >> Failed to compute angular span"));
		return false;
	}

	const float SpanRad = RightRad - LeftRad;
	if (SpanRad <= KINDA_SMALL_NUMBER)
	{
		UE_LOG(VisibilityTrace, Verbose,
			TEXT("UShapeAwareVisibilityTracer::IsTargetVisible >> Angular span too small"));
		return false;
	}

	const float DesiredGapRad =
		FMath::DegreesToRadians(FMath::Max(RayGapDegrees, 0.5f));

	const int32 RayCount =
		FMath::Max(1, FMath::CeilToInt(SpanRad / DesiredGapRad));

	const float StepRad = SpanRad / RayCount;

	UE_LOG(VisibilityTrace, VeryVerbose,
		TEXT("UShapeAwareVisibilityTracer::IsTargetVisible >> Span=%.2f deg, Rays=%d, Step=%.2f deg"),
		FMath::RadiansToDegrees(SpanRad),
		RayCount,
		FMath::RadiansToDegrees(StepRad));

	// Trace rays
	for (int32 i = 0; i <= RayCount; ++i)
	{
		const float Angle = LeftRad + StepRad * i;
		const FVector Dir(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		const FVector End = ObserverLocation + Dir * MaxDistance;

		if (TraceVisibilityRay(
			ContextWorld,
			ObserverLocation,
			End,
			IgnoredActors,
			ObstacleChannel,
			bDrawDebugLine))
		{
			return true; // any clear ray = visible
		}
	}

	return false;
}
bool UShapeAwareVisibilityTracer::TraceVisibilityRay(UWorld* ContextWorld, const FVector& Start,
	const FVector& End, const TArray<AActor*>& IgnoredActors, ECollisionChannel ObstacleChannel,bool bDrawDebugLine) const
{
	if (!ContextWorld)
	{
		return false;
	}

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(VisibilityTrace), false);

	for (AActor* Actor : IgnoredActors)// add ignoring actors to the params
	{
		if (Actor)
		{
			Params.AddIgnoredActor(Actor);
		}
	}

	const bool bBlocked = ContextWorld->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ObstacleChannel,
		Params);

	if (bDrawDebugLine)//draw debug line
	{
		DrawDebugLine(
			ContextWorld,
			Start,
			End,
			bBlocked ? FColor::Red : FColor::Green,
			false,
			0.1f,
			0,
			1.f);
	}

	return !bBlocked;
}



bool UShapeAwareVisibilityTracer::ComputeAngularSpan(const FVector& ObserverLocation, UPrimitiveComponent* ShapeComp,
                                                     float& OutLeftRad, float& OutRightRad) const
{
	if (!ShapeComp)
		return false;

	if (const USphereComponent* Sphere = Cast<USphereComponent>(ShapeComp))
	{
		return ComputeSphereSpan(ObserverLocation, Sphere, OutLeftRad, OutRightRad);
	}

	if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(ShapeComp))
	{
		return ComputeCapsuleSpan(ObserverLocation, Capsule, OutLeftRad, OutRightRad);
	}

	if (const UBoxComponent* Box = Cast<UBoxComponent>(ShapeComp))
	{
		return ComputeBoxSpan(ObserverLocation, Box, OutLeftRad, OutRightRad);
	}

	// safe fallback
	return ComputeBoundsSpan(ObserverLocation, ShapeComp, OutLeftRad, OutRightRad);
}

#pragma region Shapes

//Shapes
bool UShapeAwareVisibilityTracer::ComputeSphereSpan(
	const FVector& ObserverLocation,
	const USphereComponent* Sphere,
	float& OutLeftRad,
	float& OutRightRad) const
{
	const FVector Center = Sphere->GetComponentLocation();
	const FVector ToCenter = Center - ObserverLocation;

	const float Distance = ToCenter.Size2D();
	if (Distance <= KINDA_SMALL_NUMBER)
		return false;

	const float Radius = Sphere->GetScaledSphereRadius();
	const float CenterAngle = FMath::Atan2(ToCenter.Y, ToCenter.X);

	const float HalfAngle =
		FMath::Asin(FMath::Clamp(Radius / Distance, 0.f, 1.f));

	OutLeftRad  = CenterAngle - HalfAngle;
	OutRightRad = CenterAngle + HalfAngle;
	return true;
}

bool UShapeAwareVisibilityTracer::ComputeCapsuleSpan(
	const FVector& ObserverLocation,
	const UCapsuleComponent* Capsule,
	float& OutLeftRad,
	float& OutRightRad) const
{
	const FVector Center = Capsule->GetComponentLocation();
	const FVector Up = Capsule->GetUpVector();

	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const float Radius = Capsule->GetScaledCapsuleRadius();

	const FVector P0 = Center + Up * HalfHeight;
	const FVector P1 = Center - Up * HalfHeight;

	float MinAngle = BIG_NUMBER;
	float MaxAngle = -BIG_NUMBER;

	auto Accumulate = [&](const FVector& P)
	{
		const FVector Dir = P - ObserverLocation;
		const float A = FMath::Atan2(Dir.Y, Dir.X);
		MinAngle = FMath::Min(MinAngle, A);
		MaxAngle = FMath::Max(MaxAngle, A);
	};

	Accumulate(P0);
	Accumulate(P1);

	const float Distance = FVector::Dist2D(ObserverLocation, Center);
	const float Expand =
		FMath::Asin(FMath::Clamp(Radius / Distance, 0.f, 1.f));

	OutLeftRad  = MinAngle - Expand;
	OutRightRad = MaxAngle + Expand;
	return true;
}

bool UShapeAwareVisibilityTracer::ComputeBoxSpan(
	const FVector& ObserverLocation,
	const UBoxComponent* Box,
	float& OutLeftRad,
	float& OutRightRad) const
{
	const FTransform& TM = Box->GetComponentTransform();
	const FVector Extent = Box->GetScaledBoxExtent();

	float MinAngle = BIG_NUMBER;
	float MaxAngle = -BIG_NUMBER;

	for (int32 X = -1; X <= 1; X += 2)
		for (int32 Y = -1; Y <= 1; Y += 2)
		{
			const FVector Local(X * Extent.X, Y * Extent.Y, 0.f);
			const FVector World = TM.TransformPosition(Local);
			const FVector Dir = World - ObserverLocation;

			const float A = FMath::Atan2(Dir.Y, Dir.X);
			MinAngle = FMath::Min(MinAngle, A);
			MaxAngle = FMath::Max(MaxAngle, A);
		}

	OutLeftRad  = MinAngle;
	OutRightRad = MaxAngle;
	return true;
}

bool UShapeAwareVisibilityTracer::ComputeBoundsSpan(
	const FVector& ObserverLocation,
	const UPrimitiveComponent* ShapeComp,
	float& OutLeftRad,
	float& OutRightRad) const
{
	const FBox Box = ShapeComp->Bounds.GetBox();

	float MinAngle = BIG_NUMBER;
	float MaxAngle = -BIG_NUMBER;

	const FVector Corners[4] = {
		{Box.Min.X, Box.Min.Y, 0.f},
		{Box.Min.X, Box.Max.Y, 0.f},
		{Box.Max.X, Box.Min.Y, 0.f},
		{Box.Max.X, Box.Max.Y, 0.f}
	};

	for (const FVector& C : Corners)
	{
		const FVector Dir = C - ObserverLocation;
		const float A = FMath::Atan2(Dir.Y, Dir.X);
		MinAngle = FMath::Min(MinAngle, A);
		MaxAngle = FMath::Max(MaxAngle, A);
	}

	OutLeftRad  = MinAngle;
	OutRightRad = MaxAngle;
	return true;
}

#pragma endregion
