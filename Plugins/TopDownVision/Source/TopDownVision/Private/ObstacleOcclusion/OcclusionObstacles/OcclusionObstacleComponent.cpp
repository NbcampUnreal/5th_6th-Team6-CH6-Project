// Fill out your copyright notice in the Description page of Project Settings.


#include "TopDownVision/Public/ObstacleOcclusion/PhysicallOcclusion/OcclusionObstacleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TopDownVisionDebug.h"//log



UOcclusionObstacleComponent::UOcclusionObstacleComponent()
{
	PrimaryComponentTick.bCanEverTick = true;//tick to be true
}

void UOcclusionObstacleComponent::BeginPlay()//set up for the visual
{
	Super::BeginPlay();
}

void UOcclusionObstacleComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void UOcclusionObstacleComponent::InitializeCollision()
{
	UE_LOG(Occlusion, Log,
		TEXT("UOcclusionObstacleComponent::InitializeCollision>> Initializing collision for %s"),
		*GetOwner()->GetName());

	for (UStaticMeshComponent* Mesh : NormalMeshes)
	{
		if (!Mesh) continue;

		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(
			OcclusionProbeChannel,
			ECR_Overlap);

		Mesh->SetGenerateOverlapEvents(true);

		Mesh->OnComponentBeginOverlap.AddDynamic(
			this,
			&UOcclusionObstacleComponent::OnMeshBeginOverlap);

		Mesh->OnComponentEndOverlap.AddDynamic(
			this,
			&UOcclusionObstacleComponent::OnMeshEndOverlap);

		UE_LOG(Occlusion, Log,
			TEXT("UOcclusionObstacleComponent::InitializeCollision>> Bound overlap events on %s"),
			*Mesh->GetName());
	}

	for (UStaticMeshComponent* Mesh : OccludedMeshes)
	{
		if (!Mesh) continue;

		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		UE_LOG(Occlusion, Log,
			TEXT("UOcclusionObstacleComponent::InitializeCollision>> Disabled collision on OccludedMesh %s"),
			*Mesh->GetName());
	}
}

void UOcclusionObstacleComponent::CleanupInvalidOverlaps()
{
	int32 RemovedCount = 0;

	for (auto It = ActiveOverlaps.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
			RemovedCount++;
		}
	}

	if (RemovedCount > 0)
	{
		UE_LOG(Occlusion, Log,
			TEXT("UOcclusionObstacleComponent::CleanupInvalidOverlaps>> Removed %d invalid overlaps"),
			RemovedCount);
	}
}

void UOcclusionObstacleComponent::DiscoverChildMeshes()
{
	// Clear previous data (safety for PIE / re-init)
	NormalMeshes.Empty();
	OccludedMeshes.Empty();
	NormalDynamicMaterials.Empty();
	OccludedDynamicMaterials.Empty();
	ActiveOverlaps.Empty();

	UE_LOG(Occlusion, Log,
		TEXT("UOcclusionObstacleComponent::DiscoverChildMeshes>> Discovering child meshes for %s"),
		*GetOwner()->GetName());

	TArray<USceneComponent*> Children;
	GetChildrenComponents(true, Children);

	for (USceneComponent* Child : Children)
	{
		if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Child))
		{
			if (Mesh->ComponentHasTag(NormalMeshTag))
			{
				NormalMeshes.Add(Mesh);

				UE_LOG(Occlusion, Log,
					TEXT("UOcclusionObstacleComponent::DiscoverChildMeshes>> Registered NormalMesh: %s"),
					*Mesh->GetName());
			}
			else if (Mesh->ComponentHasTag(OccludedMeshTag))
			{
				OccludedMeshes.Add(Mesh);

				UE_LOG(Occlusion, Log,
					TEXT("UOcclusionObstacleComponent::DiscoverChildMeshes>> Registered OccludedMesh: %s"),
					*Mesh->GetName());
			}
		}
	}

	UE_LOG(Occlusion, Log,
		TEXT("UOcclusionObstacleComponent::DiscoverChildMeshes>> Total NormalMeshes: %d | OccludedMeshes: %d"),
		NormalMeshes.Num(),
		OccludedMeshes.Num());

	if (NormalMeshes.Num() == 0 && OccludedMeshes.Num() == 0)
	{
		UE_LOG(Occlusion, Warning,
			TEXT("UOcclusionObstacleComponent::DiscoverChildMeshes>> No tagged meshes found on %s"),
			*GetOwner()->GetName());
	}

	Modify();

	bool DidMarked=MarkPackageDirty();
	
}

void UOcclusionObstacleComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float TargetAlpha = bShouldBeOccluded ? 1.f : 0.f;

	if (bShouldBeOccluded != bLastOcclusionState)
	{
		UE_LOG(Occlusion, Verbose,
			TEXT("UOcclusionObstacleComponent::TickComponent>> Occlusion State Changed -> %s"),
			bShouldBeOccluded ? TEXT("OCCLUDED") : TEXT("VISIBLE"));

		bLastOcclusionState = bShouldBeOccluded;
	}

	CurrentAlpha = FMath::FInterpTo(
		CurrentAlpha,
		TargetAlpha,
		DeltaTime,
		FadeSpeed);

	UpdateMaterialAlpha();
}

void UOcclusionObstacleComponent::SetupOcclusionMeshes()
{
	DiscoverChildMeshes();
	InitializeCollision();
	InitializeMaterials();

	UE_LOG(Occlusion, Log,
		TEXT("UOcclusionObstacleComponent::SetupOcclusionMeshes>> Completed setup for %s"),
		*GetOwner()->GetName());
}

void UOcclusionObstacleComponent::OnMeshBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                                     UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherComp) return;

	if (OtherComp->GetCollisionObjectType() != OcclusionProbeChannel)
		return;

	ActiveOverlaps.Add(OtherComp);

	bShouldBeOccluded = ActiveOverlaps.Num() > 0;

	UE_LOG(Occlusion, Log,
		TEXT("UOcclusionObstacleComponent::OnMeshBeginOverlap>> Overlap BEGIN with %s | ActiveOverlaps: %d"),
		*OtherComp->GetName(),
		ActiveOverlaps.Num());
}

void UOcclusionObstacleComponent::OnMeshEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherComp) return;

	if (OtherComp->GetCollisionObjectType() != OcclusionProbeChannel)
		return;

	ActiveOverlaps.Remove(OtherComp);

	CleanupInvalidOverlaps();

	bShouldBeOccluded = ActiveOverlaps.Num() > 0;

	UE_LOG(Occlusion, Log,
		TEXT("UOcclusionObstacleComponent::OnMeshEndOverlap>> Overlap END with %s | ActiveOverlaps: %d"),
		*OtherComp->GetName(),
		ActiveOverlaps.Num());
}

void UOcclusionObstacleComponent::InitializeMaterials()
{
	//internal helper function for setting nomal and occluded meshes
	UE_LOG(Occlusion, Log,
		TEXT("UOcclusionObstacleComponent::InitializeMaterials>> Creating dynamic materials for %s"),
		*GetOwner()->GetName());

	auto SetupArray =
		[this](const TArray<UStaticMeshComponent*>& MeshArray,
			   TArray<UMaterialInstanceDynamic*>& OutArray)
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
						OutArray.Add(Dyn);

						UE_LOG(Occlusion, Log,
							TEXT("UOcclusionObstacleComponent::InitializeMaterials>> Created Dynamic Material on %s (Index %d)"),
							*Mesh->GetName(),
							i);
					}
				}
			}
		};

	SetupArray(NormalMeshes, NormalDynamicMaterials);
	SetupArray(OccludedMeshes, OccludedDynamicMaterials);

	UE_LOG(Occlusion, Log,
		TEXT("UOcclusionObstacleComponent::InitializeMaterials>> NormalDyn: %d | OccludedDyn: %d"),
		NormalDynamicMaterials.Num(),
		OccludedDynamicMaterials.Num());
}

void UOcclusionObstacleComponent::UpdateMaterialAlpha()
{
	//swapped
	const float NormalAlpha   =  1.f - CurrentAlpha;;
	const float OccludedAlpha = CurrentAlpha;

	for (UMaterialInstanceDynamic* Dyn : NormalDynamicMaterials)
	{
		if (!Dyn) continue;

		Dyn->SetScalarParameterValue(
			AlphaParameterName,
			NormalAlpha);
	}

	for (UMaterialInstanceDynamic* Dyn : OccludedDynamicMaterials)
	{
		if (!Dyn) continue;

		Dyn->SetScalarParameterValue(
			AlphaParameterName,
			OccludedAlpha);
	}
}
