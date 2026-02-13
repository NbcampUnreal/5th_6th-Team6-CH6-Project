#include "Monster/MonsterRangeComponent.h"

#include "Net/UnrealNetwork.h"
#include "Components/SphereComponent.h"
#include "Components/StateTreeComponent.h"
#include "CharacterSystem/Character/BaseCharacter.h"
#include "Monster/BaseMonster.h"

UMonsterRangeComponent::UMonsterRangeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UMonsterRangeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMonsterRangeComponent, PlayerCount);
}

void UMonsterRangeComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	// 서버에서만 생성하여 체크
	RangeSphere = NewObject<USphereComponent>(this);
    RangeSphere->InitSphereRadius(PlayerCountSphereRadius);
    RangeSphere->SetCollisionProfileName(TEXT("PlayerCounter"));
    RangeSphere->SetGenerateOverlapEvents(true);
    RangeSphere->SetWorldLocation(GetOwner()->GetActorLocation());
    RangeSphere->RegisterComponent();

    RangeSphere->OnComponentBeginOverlap.AddDynamic(
        this, &UMonsterRangeComponent::OnPlayerCountingBeginOverlap);
    RangeSphere->OnComponentEndOverlap.AddDynamic(
        this, &UMonsterRangeComponent::OnPlayerCountingEndOverlap);


	// 서버에서만 생성하여 체크
	OutSphere = NewObject<USphereComponent>(this);
	OutSphere->InitSphereRadius(PlayerOutSphereRadius);
	OutSphere->SetCollisionProfileName(TEXT("PlayerCounter"));
	OutSphere->SetGenerateOverlapEvents(true);
	OutSphere->SetWorldLocation(GetOwner()->GetActorLocation());
	OutSphere->RegisterComponent();

	OutSphere->OnComponentEndOverlap.AddDynamic(
		this, &UMonsterRangeComponent::OnPlayerOutEndOverlap);
}

void UMonsterRangeComponent::SetPlayerCount(int32 Amount)
{
	PlayerCount = Amount;
}

int32 UMonsterRangeComponent::GetPlayerCount()
{
	return PlayerCount;
}
void UMonsterRangeComponent::OnPlayerCountingBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor->IsA<ABaseCharacter>())
	{
		PlayerCount = FMath::Max(0, PlayerCount + 1);

		if (PlayerCount == 1)
		{
			OnPlayerCountOne.Broadcast();	
		}

		if (Debug)
		{
			DrawDebugSphere(
				GetWorld(),
				RangeSphere->GetComponentLocation(),
				RangeSphere->GetScaledSphereRadius(),
				8,
				FColor::Red,
				false,
				1.f,
				0,
				1.f
			);
		}
	}
}

void UMonsterRangeComponent::OnPlayerCountingEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor->IsA<ABaseCharacter>())
	{
		PlayerCount = FMath::Max(0, PlayerCount - 1);

		if (PlayerCount == 0)
		{
			OnPlayerCountZero.Broadcast();
		}

		if (Debug)
		{
			DrawDebugSphere(
				GetWorld(),
				RangeSphere->GetComponentLocation(),
				RangeSphere->GetScaledSphereRadius(),
				8,
				FColor::Green,
				false,
				1.f,
				0,
				1.f
			);
		}
	}
}

void UMonsterRangeComponent::OnPlayerOutEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor->IsA<ABaseCharacter>())
	{
		AActor* Target = Cast<ABaseMonster>(GetOwner())->GetTargetPlayer();
		if (Target == OtherActor)
		{
			OnPlayerOut.Broadcast();
		}

		if (Debug)
		{
			DrawDebugSphere(
				GetWorld(),
				OutSphere->GetComponentLocation(),
				OutSphere->GetScaledSphereRadius(),
				8,
				FColor::Green,
				false,
				1.f,
				0,
				1.f
			);
		}
	}
}