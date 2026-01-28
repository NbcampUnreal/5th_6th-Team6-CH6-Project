#include "Monster/MonsterRangeComponent.h"

#include "Components/SphereComponent.h"
#include "Components/StateTreeComponent.h"
#include "Monster/BaseMonster.h"

UMonsterRangeComponent::UMonsterRangeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UMonsterRangeComponent::BeginPlay()
{
	Super::BeginPlay();

	RangeSphere = NewObject<USphereComponent>(this);
    RangeSphere->InitSphereRadius(SphereRadius);
    RangeSphere->SetCollisionProfileName(TEXT("PlayerCounter"));
    RangeSphere->SetGenerateOverlapEvents(true);
    RangeSphere->SetWorldLocation(GetOwner()->GetActorLocation());
    RangeSphere->RegisterComponent();

    RangeSphere->OnComponentBeginOverlap.AddDynamic(
        this, &UMonsterRangeComponent::OnPlayerCountingBeginOverlap);
    RangeSphere->OnComponentEndOverlap.AddDynamic(
        this, &UMonsterRangeComponent::OnPlayerCountingEndOverlap);


	if(Debug)
	{ 
		DrawDebugSphere(
			GetWorld(),
			RangeSphere->GetComponentLocation(),
			RangeSphere->GetScaledSphereRadius(),
			8,
			FColor::Green,
			true,
			-1.f,
			0,
			2.f
		);
	}
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
	if (OtherActor && OtherActor->IsA<APawn>())
	{
		PlayerCount++;
		UE_LOG(LogTemp, Warning, TEXT("PlayerCount: %d"), PlayerCount);

		if (PlayerCount == 1)
		{
			UStateTreeComponent* STComp = Cast<ABaseMonster>(GetOwner())->GetStateTreeComponent();
			if (IsValid(STComp) == false)
			{
				return;
			}

			FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(FName("AI.Event.Awake"));
			STComp->SendStateTreeEvent(FStateTreeEvent(EventTag));
		}
	}
}

void UMonsterRangeComponent::OnPlayerCountingEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor->IsA<APawn>())
	{
		PlayerCount--;
		UE_LOG(LogTemp, Warning, TEXT("PlayerCount: %d"), PlayerCount);

		if (PlayerCount == 0)
		{
			UStateTreeComponent* STComp = Cast<ABaseMonster>(GetOwner())->GetStateTreeComponent();
			if (IsValid(STComp) == false)
			{
				return;
			}

			FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(FName("AI.Event.Sleep"));
			STComp->SendStateTreeEvent(FStateTreeEvent(EventTag));
		}
	}
}