#include "Monster/MonsterRangeComponent.h"

#include "Components/SphereComponent.h"
#include "Components/StateTreeComponent.h"
#include "Monster/BaseMonster.h"

#include "CharacterSystem/Character/BaseCharacter.h"
#include "AbilitySystemComponent.h"

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
			// State 전환 이벤트
			UStateTreeComponent* STComp = Cast<ABaseMonster>(GetOwner())->GetStateTreeComponent();
			if (IsValid(STComp) == false)
			{
				return;
			}

			FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(FName("AI.Event.Awake"));
			STComp->SendStateTreeEvent(FStateTreeEvent(EventTag));
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
	if (OtherActor && OtherActor->IsA<APawn>())
	{
		PlayerCount = FMath::Max(0, PlayerCount - 1);
		UE_LOG(LogTemp, Warning, TEXT("PlayerCount: %d"), PlayerCount);

		// State 전환 이벤트
		ABaseMonster* Monster = Cast<ABaseMonster>(GetOwner());
		if (IsValid(Monster) == false)
		{
			return;
		}

		UStateTreeComponent* STComp = Monster->GetStateTreeComponent();
		if (IsValid(STComp) == false)
		{
			return;
		}

		//TargetPlayer나가면 Return상태로 전환
		if (OtherActor == Monster->GetTargetPlayer())
		{
			UE_LOG(LogTemp, Warning, TEXT("MonsterState : Return"));
			FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(FName("AI.Event.Return"));
			STComp->SendStateTreeEvent(FStateTreeEvent(EventTag));
			Monster->SetTargetPlayer(nullptr);
			Monster->SetbIsCombat(false);
		}

		//TargetPlayer도 없고 근처에 아무도 없으면 Sleep 상태로 전환
		else if (PlayerCount == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("MonsterState : Sleep"));
			FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(FName("AI.Event.Sleep"));
			STComp->SendStateTreeEvent(FStateTreeEvent(EventTag));
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