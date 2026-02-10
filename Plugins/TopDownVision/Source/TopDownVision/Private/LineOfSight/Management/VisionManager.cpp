// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/Management/VisionManager.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "LineOfSight/Management/VisionProviderInterface.h"

UVisionManager::UVisionManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UVisionManager::BeginPlay()
{
	Super::BeginPlay();

	if (!CanRunServerLogic())
	{
		SetComponentTickEnabled(false);
	}

}

void UVisionManager::RegisterVisionProvider( TScriptInterface<IVisionProviderInterface> Provider)
{
	if (!CanRunServerLogic() || !Provider)
		return;

	uint8 Team = Provider->GetVisionTeam();
	TeamVision.FindOrAdd(Team).Providers.Add(Provider.GetObject());
}

void UVisionManager::UnregisterVisionProvider(
	TScriptInterface<IVisionProviderInterface> Provider)
{
	if (!CanRunServerLogic() || !Provider)
		return;

	uint8 Team = Provider->GetVisionTeam();
	if (FTeamVisionData* Data = TeamVision.Find(Team))
	{
		Data->Providers.Remove(Provider.GetObject());
	}
}

void UVisionManager::UpdateVision()
{
	if (!CanRunServerLogic())
	{
		return;
	}

	UObstacleSubsystem* Obstacles =
		GetWorld()->GetSubsystem<UObstacleSubsystem>();

	if (!Obstacles)
		return;

	for (auto& [TeamId, Data] : TeamVision)
	{
		Data.VisibleActors.Reset();

		for (TWeakObjectPtr<AActor> ProviderActor : Data.Providers)
		{
			if (!ProviderActor.IsValid())
				continue;

			IVisionProviderInterface* Provider =
				Cast<IVisionProviderInterface>(ProviderActor.Get());

			if (!Provider)
				continue;

			// TODO:
			// - Query obstacles
			// - Perform LOS
			// - Collect visible actors
			// Example placeholder:
			// Data.VisibleActors.Add(TargetActor);
		}

		// Flatten for replication
		TArray<AActor*>& RepArray =
			ReplicatedVisibleActors.FindOrAdd(TeamId);

		RepArray.Reset();
		for (auto& Actor : Data.VisibleActors)
		{
			if (Actor.IsValid())
				RepArray.Add(Actor.Get());
		}
	}
}

bool UVisionManager::CanRunServerLogic()
{
	return GetOwner() && GetOwner()->HasAuthority();
}

bool UVisionManager::IsActorVisibleToTeam(uint8 TeamId, AActor* Target) const
{
	if (const TArray<AActor*>* List =
		ReplicatedVisibleActors.Find(TeamId))
	{
		return List->Contains(Target);
	}
	return false;
}

void UVisionManager::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UVisionManager, ReplicatedVisibleActors);
}

void UVisionManager::OnRep_VisionUpdated()
{
	// Client-side hook:
	// - update fog
	// - update PP material
	// - update minimap
}