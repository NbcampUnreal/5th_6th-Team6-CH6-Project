// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/Management/VisionGameStateComp.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "LineOfSight/Management/VisionProviderInterface.h"

UVisionGameStateComp::UVisionGameStateComp()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UVisionGameStateComp::BeginPlay()
{
	Super::BeginPlay();

	if (CanRunServerLogic())
	{
		// Initial vision update
		UpdateVision();
	}
}

void UVisionGameStateComp::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	//TeamVisionData.Empty();
	Super::EndPlay(EndPlayReason);
}

bool UVisionGameStateComp::CanRunServerLogic() const
{
	return GetOwnerRole() == ROLE_Authority;
}

void UVisionGameStateComp::RegisterVisionProvider(TScriptInterface<IVisionProviderInterface> Provider)
{
	/*if (!Provider) return;

	uint8 Team = Provider->GetVisionTeam();

	FTeamVisionData& Data = TeamVisionData.FindOrAdd(Team);
	Data.Providers.Add(Provider.GetObject());

	if (CanRunServerLogic())
	{
		UpdateVision();
	}*/
}

void UVisionGameStateComp::UnregisterVisionProvider(TScriptInterface<IVisionProviderInterface> Provider)
{/*
	if (!Provider) return;

	uint8 Team = Provider->GetVisionTeam();

	if (FTeamVisionData* Data = TeamVisionData.Find(Team))
	{
		Data->Providers.Remove(Provider.GetObject());
		if (CanRunServerLogic())
		{
			UpdateVision();
		}
	}*/
}

bool UVisionGameStateComp::IsActorVisibleToTeam(uint8 TeamChannel, AActor* Target) const
{
	/*if (!Target) return false;

	if (const FTeamVisionData* Data = TeamVisionData.Find(TeamChannel))
	{
		return Data->VisibleActors.Contains(Target);
	}*/

	return false;
}

void UVisionGameStateComp::UpdateVision()
{
	/*if (!CanRunServerLogic()) return;

	for (auto& Pair : TeamVisionData)
	{
		FTeamVisionData& Data = Pair.Value;
		Data.VisibleActors.Empty();

		for (TWeakObjectPtr<UObject> ProviderPtr : Data.Providers)
		{
			if (!ProviderPtr.IsValid()) continue;

			TScriptInterface<IVisionProviderInterface> Provider(ProviderPtr.Get());
			TArray<AActor*> Visible;
			Provider->GetVisibleActors(Visible);

			// Append actors to TSet (weak pointers)
			for (AActor* Actor : Visible)
			{
				if (Actor)
				{
					Data.VisibleActors.Add(Actor);
				}
			}
		}
	}

	// Rep notify manually
	for (auto& Pair : TeamVisionData)
	{
		OnRep_VisionUpdated();
	}*/
}

void UVisionGameStateComp::OnRep_VisionUpdated()
{
	// Runs on clients after replication
	UE_LOG(LogTemp, Log, TEXT("Vision data updated on client"));
}

void UVisionGameStateComp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(UVisionGameStateComp, TeamVisionData);
}