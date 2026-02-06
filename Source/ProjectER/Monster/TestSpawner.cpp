#include "Monster/TestSpawner.h"

ATestSpawner::ATestSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	SpawnCount = 1;
}

void ATestSpawner::BeginPlay()
{
	Super::BeginPlay();
	
	if(HasAuthority())
	{
		for (int32 i = 0; i < SpawnCount; i++)
		{
			GetWorld()->SpawnActor<AActor>(Monster, GetActorLocation(), FRotator::ZeroRotator);
		}
	}
}



