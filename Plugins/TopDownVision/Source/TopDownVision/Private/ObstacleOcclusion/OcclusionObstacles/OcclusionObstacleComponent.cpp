// Fill out your copyright notice in the Description page of Project Settings.


#include "TopDownVision/Public/ObstacleOcclusion/PhysicallOcclusion/OcclusionObstacleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"



UOcclusionObstacleComponent::UOcclusionObstacleComponent()
{
	PrimaryComponentTick.bCanEverTick = true;//tick to be true
}

void UOcclusionObstacleComponent::BeginPlay()//set up for the visual
{
	Super::BeginPlay();

	InitializeCollision();
	InitializeMaterials();
}

void UOcclusionObstacleComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void UOcclusionObstacleComponent::InitializeCollision()
{
	for (UStaticMeshComponent* Mesh : NormalMeshes)
	{
		if (!Mesh) continue;

		// Only overlap OccluderProbe channel
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(
			ECC_GameTraceChannel1,   // OccluderProbe
			ECR_Overlap);

		Mesh->SetGenerateOverlapEvents(true);

		Mesh->OnComponentBeginOverlap.AddDynamic(
			this,
			&UOcclusionObstacleComponent::OnMeshBeginOverlap);

		Mesh->OnComponentEndOverlap.AddDynamic(
			this,
			&UOcclusionObstacleComponent::OnMeshEndOverlap);
	}

	// Occluded meshes = visual only
	for (UStaticMeshComponent* Mesh : OccludedMeshes)
	{
		if (!Mesh) continue;
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void UOcclusionObstacleComponent::CleanupInvalidOverlaps()
{
	for (auto It = ActiveOverlaps.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

void UOcclusionObstacleComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float TargetAlpha = bShouldBeOccluded ? 1.f : 0.f;

	CurrentAlpha = FMath::FInterpTo(
		CurrentAlpha,
		TargetAlpha,
		DeltaTime,
		FadeSpeed);

	UpdateMaterialAlpha();
}

void UOcclusionObstacleComponent::OnMeshBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherComp) return;

	// Safety check: only accept OccluderProbe channel
	if (OtherComp->GetCollisionObjectType() != ECC_GameTraceChannel1)
		return;

	ActiveOverlaps.Add(OtherComp);

	bShouldBeOccluded = ActiveOverlaps.Num() > 0;
}

void UOcclusionObstacleComponent::OnMeshEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherComp) return;

	if (OtherComp->GetCollisionObjectType() != ECC_GameTraceChannel1)
		return;

	ActiveOverlaps.Remove(OtherComp);

	CleanupInvalidOverlaps();

	bShouldBeOccluded = ActiveOverlaps.Num()>0;
}

void UOcclusionObstacleComponent::InitializeMaterials()
{
	//internal helper function for setting nomal and occluded meshes
	auto SetupArray = [this](const TArray<UStaticMeshComponent*>& MeshArray)
	{
		for (UStaticMeshComponent* Mesh : MeshArray)
		{
			if (!Mesh) continue;

			const int32 MatCount = Mesh->GetNumMaterials();

			for (int32 i = 0; i < MatCount; ++i)
			{
				UMaterialInterface* BaseMat = Mesh->GetMaterial(i);
				if (!BaseMat) continue;

				UMaterialInstanceDynamic* Dyn =
					Mesh->CreateDynamicMaterialInstance(i, BaseMat);

				if (Dyn)
				{
					DynamicMaterials.Add(Dyn);
				}
			}
		}
	};

	SetupArray(NormalMeshes);
	SetupArray(OccludedMeshes);
}

void UOcclusionObstacleComponent::UpdateMaterialAlpha()
{
	for (UMaterialInstanceDynamic* Dyn : DynamicMaterials)
	{
		if (!Dyn) continue;

		Dyn->SetScalarParameterValue(
			AlphaParameterName,
			CurrentAlpha);
	}
}
