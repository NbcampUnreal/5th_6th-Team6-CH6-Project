#include "GameModeBase/PointActor/ER_PointActor.h"
#include "GameModeBase/Subsystem/NeutralSpawn/ER_NeutralSpawnSubsystem.h"
#include "GameModeBase/Subsystem/Object/ER_ObjectSubsystem.h"

AER_PointActor::AER_PointActor()
{

}

void AER_PointActor::BeginPlay()
{
	Super::BeginPlay();

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

