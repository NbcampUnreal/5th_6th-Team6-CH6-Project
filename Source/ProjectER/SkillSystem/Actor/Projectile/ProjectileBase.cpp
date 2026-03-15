#include "SkillSystem/Actor/Projectile/ProjectileBase.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/PrimitiveComponent.h"

// Sets default values
AProjectileBase::AProjectileBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 0.0f;
	ProjectileMovement->MaxSpeed = 0.0f;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
}

void AProjectileBase::SetProjectileMovement(float InitialSpeed, float GravityScale, bool bInDestroyOnHit)
{
	if (IsValid(ProjectileMovement))
	{
		ProjectileMovement->InitialSpeed = InitialSpeed;
		ProjectileMovement->MaxSpeed = FMath::Max(InitialSpeed, 10000.0f);
		ProjectileMovement->ProjectileGravityScale = GravityScale;
		ProjectileMovement->SetVelocityInLocalSpace(FVector::ForwardVector * InitialSpeed);
	}

	bDestroyOnHit = bInDestroyOnHit;
}

// Called when the game starts or when spawned
void AProjectileBase::BeginPlay()
{
	Super::BeginPlay();
	
}

void AProjectileBase::OnShapeBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnShapeBeginOverlap(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if (bDestroyOnHit && HasAuthority() && IsValid(OtherActor) && OtherActor != this && OtherActor != InstigatorActor)
	{
		if (HitActors.Contains(OtherActor)) // If it was a valid hit based on parent rules
		{
			Destroy();
		}
	}
}

// Called every frame
void AProjectileBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
