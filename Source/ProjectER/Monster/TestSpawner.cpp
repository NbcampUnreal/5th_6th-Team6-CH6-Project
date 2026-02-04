#include "Monster/TestSpawner.h"

ATestSpawner::ATestSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

}

void ATestSpawner::BeginPlay()
{
	Super::BeginPlay();
	
	if(HasAuthority())
	{
		GetWorld()->SpawnActor<AActor>(Monster, GetActorLocation(), FRotator::ZeroRotator);
	}
}



