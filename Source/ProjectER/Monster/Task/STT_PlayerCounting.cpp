#include "Monster/Task/STT_PlayerCounting.h"

#include "Components/SphereComponent.h"
#include "Monster/BaseMonster.h"
#include "StateTreeExecutionContext.h"
#include "Components/StateTreeComponent.h"
#include "GameplayTagContainer.h"

EStateTreeRunStatus USTT_PlayerCounting::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition)
{
	AActor* OwnerActor = Cast<AActor>(GetOuter());
	if (!OwnerActor) return EStateTreeRunStatus::Failed;

	//if (Isbool)
	//{
	//	return EStateTreeRunStatus::Succeeded;
	//} 

	UE_LOG(LogTemp, Warning, TEXT(" USTT_PlayerCounting EnterState"));
	//Isbool = true;

	Sphere = NewObject<USphereComponent>(OwnerActor);
	Sphere->SetSphereRadius(FindRadius);
	Sphere->SetCollisionProfileName(TEXT("PlayerCounter")); // Pawn만 체크
	Sphere->SetGenerateOverlapEvents(true);
	Sphere->SetWorldLocation(OwnerActor->GetActorLocation());
	Sphere->RegisterComponent();

	Sphere->OnComponentBeginOverlap.AddDynamic(this, &USTT_PlayerCounting::OnBeginOverlap);
	Sphere->OnComponentEndOverlap.AddDynamic(this, &USTT_PlayerCounting::OnEndOverlap);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus USTT_PlayerCounting::Tick(FStateTreeExecutionContext& Context, const float DeltaTime)
{
	return EStateTreeRunStatus();
}

void USTT_PlayerCounting::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition)
{

}

void USTT_PlayerCounting::OnBeginOverlap(
	UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor->IsA<APawn>())
	{
		ABaseMonster* OwnerActor = Cast<ABaseMonster>(GetOuter());
		OwnerActor->SetPlayerCount(1);
		UE_LOG(LogTemp, Warning, TEXT("PlayerCount: %d"), OwnerActor->GetPlayerCount());

		if (OwnerActor->GetPlayerCount() == 1)
		{
			UStateTreeComponent* STComp = OwnerActor->GetStateTreeComponent();
			if (IsValid(STComp) == false)
			{
				return;
			}

			FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(FName("AI.Event.Awake"));
			STComp->SendStateTreeEvent(FStateTreeEvent(EventTag));
		}
	}
}

void USTT_PlayerCounting::OnEndOverlap(
	UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor->IsA<APawn>())
	{
		ABaseMonster* OwnerActor = Cast<ABaseMonster>(GetOuter());
		OwnerActor->SetPlayerCount(-1);
		UE_LOG(LogTemp, Warning, TEXT("PlayerCount: %d"), OwnerActor->GetPlayerCount());

		if (OwnerActor->GetPlayerCount() == 0)
		{
			UStateTreeComponent* STComp = OwnerActor->GetStateTreeComponent();
			if (IsValid(STComp) == false)
			{
				return;
			}

			FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(FName("AI.Event.Sleep"));
			STComp->SendStateTreeEvent(FStateTreeEvent(EventTag));
		}

	}
}
