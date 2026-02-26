// Fill out your copyright notice in the Description page of Project Settings.


#include "OcclusionProbeComp.h"

#include "FCurvedWorldUtil.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"


// Sets default values for this component's properties
UOcclusionProbeComp::UOcclusionProbeComp()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UOcclusionProbeComp::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		CameraManager = PC->PlayerCameraManager;
	}

	CurvedWorldSubsystem = GetWorld()->GetSubsystem<UCurvedWorldSubsystem>();

	InitializeProbes();
}


void UOcclusionProbeComp::InitializeProbes()
{
	ProbeSpheres.Reserve(NumProbes);

	for (int32 i = 0; i < NumProbes; ++i)
	{
		FString Name = FString::Printf(TEXT("OcclusionProbe_%d"), i);

		USphereComponent* Sphere = NewObject<USphereComponent>(GetOwner(), *Name);
		Sphere->SetupAttachment(this);
		Sphere->RegisterComponent();

		Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
		Sphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);

		ProbeSpheres.Add(Sphere);
	}
}

void UOcclusionProbeComp::TickComponent(float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateProbes();
}


void UOcclusionProbeComp::UpdateProbes()
{
	if (!CameraManager || !CurvedWorldSubsystem)
		return;

	FVector CameraLoc = CameraManager->GetCameraLocation();
	FVector TargetLoc = GetComponentLocation();

	TArray<FVector> PathPoints =
		FCurvedWorldUtil::GenerateCurvedPathPoints(
			CameraLoc,
			TargetLoc,
			NumProbes,
			CurvedWorldSubsystem,
			ECurveMathType::ZHeightOnly);

	float SphereRadius = CalculateScreenProjectedRadius();
	SphereRadius = FMath::Clamp(SphereRadius, MinSphereRadius, MaxSphereRadius);

	for (int32 i = 0; i < ProbeSpheres.Num(); ++i)
	{
		ProbeSpheres[i]->SetWorldLocation(PathPoints[i]);
		ProbeSpheres[i]->SetSphereRadius(SphereRadius);
	}
}

float UOcclusionProbeComp::CalculateScreenProjectedRadius() const
{
	if (!CameraManager)
		return MinSphereRadius;

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
		return MinSphereRadius;

	FVector ActorLocation = GetOwner()->GetActorLocation();

	FBox TargetBounds = GetOwner()->GetComponentsBoundingBox();
	float ApproxRadius = TargetBounds.GetExtent().Size();

	FVector WorldEdge = ActorLocation + FVector(ApproxRadius, 0, 0);

	FVector2D ScreenCenter;
	FVector2D ScreenEdge;

	if (!PC->ProjectWorldLocationToScreen(ActorLocation, ScreenCenter))
		return MinSphereRadius;

	if (!PC->ProjectWorldLocationToScreen(WorldEdge, ScreenEdge))
		return MinSphereRadius;

	float PixelRadius = FVector2D::Distance(ScreenCenter, ScreenEdge);

	float DistanceToCamera = FVector::Distance(
		ActorLocation,
		CameraManager->GetCameraLocation());

	float FOV = CameraManager->GetFOVAngle();

	float WorldRadius =
		2.f * DistanceToCamera *
		FMath::Tan(FMath::DegreesToRadians(PixelRadius * FOV / 1920.f));

	return WorldRadius;
}