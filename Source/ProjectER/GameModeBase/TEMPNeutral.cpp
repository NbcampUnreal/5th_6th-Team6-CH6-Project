
#include "GameModeBase/TEMPNeutral.h"
#include "GameModeBase/GameMode/ER_InGameMode.h"

// Sets default values
ATEMPNeutral::ATEMPNeutral()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ATEMPNeutral::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATEMPNeutral::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATEMPNeutral::Death()
{
	if (!HasAuthority())
		return;

	auto InGameMode = Cast<AER_InGameMode>(GetWorld()->GetAuthGameMode());
	InGameMode->NotifyNeutralDied(this);

	SetLifeSpan(0.1f);
}

void ATEMPNeutral::Respawn()
{

}


