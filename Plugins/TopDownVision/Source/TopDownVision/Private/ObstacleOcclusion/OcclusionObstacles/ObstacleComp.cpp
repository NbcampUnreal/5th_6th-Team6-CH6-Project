// Fill out your copyright notice in the Description page of Project Settings.


#include "TopDownVision/Public/ObstacleOcclusion/PhysicallOcclusion/ObstacleComp.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

// Sets default values for this component's properties
UObstacleComp::UObstacleComp()
{
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UObstacleComp::BeginPlay()
{
	Super::BeginPlay();

	//Set the occluded mesh to be hidden as default at first
	for (UStaticMeshComponent* Mesh : OccludedMeshes)
	{
		if (Mesh)
		{
			Mesh->SetVisibility(false, true);
		}
	}
	
}


// Called every frame
void UObstacleComp::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateTransition(DeltaTime);
}

void UObstacleComp::OnOcclusionEnter_Implementation(UActorComponent* SourceComponent)
{
	if (!SourceComponent)
	{
		return;
	}

	ActiveSources.Add(SourceComponent);
	TargetAlpha = 1.f;
}

void UObstacleComp::OnOcclusionExit_Implementation(UActorComponent* SourceComponent)
{
	if (!SourceComponent)
	{
		return;
	}

	ActiveSources.Remove(SourceComponent);

	if (!IsRevealed())
	{
		TargetAlpha = 0.f;
	}
}


void UObstacleComp::UpdateTransition(float DeltaTime)
{
	const float NewAlpha = FMath::FInterpTo(
		CurrentAlpha,
		TargetAlpha,
		DeltaTime,
		InterpSpeed);

	if (!FMath::IsNearlyEqual(NewAlpha, CurrentAlpha, 0.001f))//update till it is nearly zero
	{
		CurrentAlpha = NewAlpha;
		ApplyVisualState();
	}
}

void UObstacleComp::ApplyVisualState()
{
	// ----- Normal Meshes -----
	for (UStaticMeshComponent* Mesh : NormalMeshes)
	{
		if (!Mesh)
			continue;

		const float NormalOpacity = 1.f - CurrentAlpha;
		Mesh->SetScalarParameterValueOnMaterials(TEXT("Opacity"), NormalOpacity);
	}

	// ----- Occluded Meshes -----
	if (OccludedMeshes.Num() > 0)
	{
		const bool bShouldBeVisible = CurrentAlpha > 0.01f;

		for (UStaticMeshComponent* Mesh : OccludedMeshes)
		{
			if (!Mesh)
				continue;

			Mesh->SetVisibility(bShouldBeVisible, true);
			Mesh->SetScalarParameterValueOnMaterials(TEXT("Opacity"), CurrentAlpha);
		}
	}
}

