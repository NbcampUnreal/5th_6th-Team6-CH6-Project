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
	DOREPLIFETIME(UMonsterRangeComponent, PlayerCountInOutSphere);
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

	RangeSphere->OnComponentBeginOverlap.AddDynamic(
		this, &UMonsterRangeComponent::OnPlayerCountingBeginOverlap);
	RangeSphere->OnComponentEndOverlap.AddDynamic(
		this, &UMonsterRangeComponent::OnPlayerCountingEndOverlap);

	RangeSphere->RegisterComponent();
	RangeSphere->SetWorldLocation(GetOwner()->GetActorLocation());


	// 서버에서만 생성하여 체크
	OutSphere = NewObject<USphereComponent>(this);
	OutSphere->InitSphereRadius(PlayerOutSphereRadius);
	OutSphere->SetCollisionProfileName(TEXT("PlayerCounter"));
	OutSphere->SetGenerateOverlapEvents(true);

	OutSphere->OnComponentBeginOverlap.AddDynamic(
		this, &UMonsterRangeComponent::OnPlayerOutBeginOverlap);
	OutSphere->OnComponentEndOverlap.AddDynamic(
		this, &UMonsterRangeComponent::OnPlayerOutEndOverlap);

	OutSphere->RegisterComponent();
	OutSphere->SetWorldLocation(GetOwner()->GetActorLocation());
}

void UMonsterRangeComponent::SetPlayerCount(int32 Amount)
{
	PlayerCount = Amount;
}

int32 UMonsterRangeComponent::GetPlayerCount()
{
	return PlayerCount;
}

void UMonsterRangeComponent::SetOutSphereRadius(float Radius)
{
	if (IsValid(OutSphere))
	{
		OutSphere->SetSphereRadius(Radius);
	}
}

//이거 스폰하고  실행
void UMonsterRangeComponent::InitMonsterGroup()
{
	ABaseMonster* OwnerMonster = Cast<ABaseMonster>(GetOwner());
	if (!OwnerMonster || !OwnerMonster->HasAuthority()) return;

	FPrimaryAssetId MyId = OwnerMonster->GetMonsterId();

	if (!MyId.IsValid()) return;

	if (!IsValid(this) || !IsValid(OwnerMonster) || !IsValid(RangeSphere)) return;

	TArray<AActor*> GroupActors;
	RangeSphere->GetOverlappingActors(GroupActors, AActor::StaticClass());


	for (AActor* Actor : GroupActors)
	{
		ABaseMonster* Monster = Cast<ABaseMonster>(Actor);
		// 죽지않고 같은 종류의 몬스터인 경우
		if (Monster && !Monster->GetbIsDead() && Monster->GetMonsterId() == MyId)
		{
			MonsterGroup.AddUnique(Monster);

			// 따로 스폰된 경우 자기를 다른 그룹에 넣어주려고
			if (UMonsterRangeComponent* OtherRangeComp = Monster->GetMonsterRangeComp())
			{
				OtherRangeComp->GetMonsterGroup().AddUnique(OwnerMonster);
			}
		}
	}
}


TArray<TObjectPtr<ABaseMonster>>& UMonsterRangeComponent::GetMonsterGroup()
{
	return MonsterGroup;
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

		//if (Debug)
		//{
		//	DrawDebugSphere(
		//		GetWorld(),
		//		RangeSphere->GetComponentLocation(),
		//		RangeSphere->GetScaledSphereRadius(),
		//		8,
		//		FColor::Red,
		//		false,
		//		1.f,
		//		0,
		//		1.f
		//	);
		//}
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

		//if (Debug)
		//{
		//	DrawDebugSphere(
		//		GetWorld(),
		//		RangeSphere->GetComponentLocation(),
		//		RangeSphere->GetScaledSphereRadius(),
		//		8,
		//		FColor::Green,
		//		false,
		//		1.f,
		//		0,
		//		1.f
		//	);
		//}
	}
}

void UMonsterRangeComponent::OnPlayerOutBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor->IsA<ABaseCharacter>())
	{
		PlayerCountInOutSphere = FMath::Max(0, PlayerCountInOutSphere + 1);

		if (PlayerCountInOutSphere == 1)
		{
			OnPlayerInOutSphereOne.Broadcast();	
		}
	}
}

void UMonsterRangeComponent::OnPlayerOutEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor->IsA<ABaseCharacter>())
	{
		PlayerCountInOutSphere = FMath::Max(0, PlayerCountInOutSphere - 1);

		if (PlayerCountInOutSphere == 0)
		{
			OnPlayerInOutSphereZero.Broadcast();
		}

		AActor* Target = Cast<ABaseMonster>(GetOwner())->GetTargetPlayer();
		if (Target == OtherActor)
		{
			OnPlayerOut.Broadcast();
		}
	}
	if (OtherActor && OtherActor->IsA<ABaseMonster>())
	{
		if (OtherActor == GetOwner())
		{
			OnPlayerOut.Broadcast();
		}
	}
}

void UMonsterRangeComponent::OnRep_PlayerCountInOutSphere()
{
	if (PlayerCountInOutSphere > 0)
	{
		OnPlayerInOutSphereOne.Broadcast();
	}
	else
	{
		OnPlayerInOutSphereZero.Broadcast();
	}
}