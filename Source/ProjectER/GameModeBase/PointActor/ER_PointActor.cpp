#include "GameModeBase/PointActor/ER_PointActor.h"
#include "GameModeBase/Subsystem/NeutralSpawn/ER_NeutralSpawnSubsystem.h"
#include "GameModeBase/Subsystem/Object/ER_ObjectSubsystem.h"

#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "Net/UnrealNetwork.h"

AER_PointActor::AER_PointActor()
{
	bReplicates = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	SelectedDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("SelectedDecal"));
	SelectedDecal->SetupAttachment(Root);

	// 기본값: 안 보이게
	SelectedDecal->SetHiddenInGame(true);
}

void AER_PointActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AER_PointActor, bSelected);
}

void AER_PointActor::BeginPlay()
{
	Super::BeginPlay();

	OnRep_Selected();

	if (!HasAuthority())
		return;

	if (PointType == EPointActorType::SpawnPoint)
	{
		if (UER_NeutralSpawnSubsystem* NSS = GetWorld()->GetSubsystem<UER_NeutralSpawnSubsystem>())
		{
			NSS->RegisterPoint(this);
		}
	}
	else if (PointType == EPointActorType::ObjectPoint)
	{
		if (UER_ObjectSubsystem* OSS = GetWorld()->GetSubsystem<UER_ObjectSubsystem>())
		{
			OSS->RegisterPoint(this);
		}
	}
	
}

void AER_PointActor::EndPlay(const EEndPlayReason::Type Reason)
{
	if (HasAuthority())
	{
		if (PointType == EPointActorType::SpawnPoint)
		{
			if (UER_NeutralSpawnSubsystem* NSS = GetWorld()->GetSubsystem<UER_NeutralSpawnSubsystem>())
			{
				NSS->UnregisterPoint(this);
			}
		}
		else if (PointType == EPointActorType::ObjectPoint)
		{
			if (UER_ObjectSubsystem* OSS = GetWorld()->GetSubsystem<UER_ObjectSubsystem>())
			{
				OSS->UnregisterPoint(this);
			}
		}
	}


	Super::EndPlay(Reason);
}

void AER_PointActor::OnRep_Selected()
{
	if (SelectedDecal)
	{
		SelectedDecal->SetHiddenInGame(!bSelected);
	}
}

void AER_PointActor::SetSelectedVisual(bool bOn)
{
	if (!HasAuthority())
		return;

	bSelected = bOn;
	OnRep_Selected();
}

