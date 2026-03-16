// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayEffectComponent/LaunchProjectile.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "GameFramework/Actor.h"
#include "SkillSystem/GameplayEffectComponent/BaseGECConfig.h"
#include "SkillSystem/Actor/BaseRangeOverlapEffectActor.h"
#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"
#include "SkillSystem/GameAbility/SkillBase.h"

#include "SkillSystem/GameplayEffectComponent/LaunchProjectile.h"
#include "SkillSystem/GameplayEffectComponent/SummonRangeAtBone.h"
#include "GameFramework/ProjectileMovementComponent.h"

#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

ULaunchProjectile::ULaunchProjectile()
{
	ConfigClass = ULaunchProjectileConfig::StaticClass();
}

TSubclassOf<UBaseGECConfig> ULaunchProjectile::GetRequiredConfigClass() const
{
	return ULaunchProjectileConfig::StaticClass();
}

void ULaunchProjectile::InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, const USummonRangeBaseConfig* Config, AActor* Instigator, const FGameplayEffectContextHandle& Context, const FGameplayCueParameters& HitTargetCueParameters) const
{
	Super::InitializeRangeActor(RangeActor, Config, Instigator, Context, HitTargetCueParameters);

	const ULaunchProjectileConfig* const ProjectileConfig = Cast<ULaunchProjectileConfig>(Config);

	if (IsValid(RangeActor) && IsValid(ProjectileConfig))
	{
		RangeActor->bDestroyOnOverlap = ProjectileConfig->bDestroyOnHit;
		
		// 필수: 동적 이동 컴포넌트의 위치 변화가 클라이언트에 복제되도록 설정
		RangeActor->SetReplicateMovement(true);

		UProjectileMovementComponent* MovementComp = NewObject<UProjectileMovementComponent>(RangeActor, UProjectileMovementComponent::StaticClass(), TEXT("DynamicProjectileMovement"));
		if (IsValid(MovementComp))
		{
			// 동적으로 추가된 컴포넌트가 에디터/클라이언트에 정상 표시되고 복제되도록 설정
			MovementComp->CreationMethod = EComponentCreationMethod::Instance;
			MovementComp->SetIsReplicated(true);

			// 이동에 필요한 기본 속성 할당
			float Speed = ProjectileConfig->InitialSpeed;
			MovementComp->InitialSpeed = Speed;
			MovementComp->MaxSpeed = Speed;
			MovementComp->ProjectileGravityScale = ProjectileConfig->GravityScale;
			
			// 컴포넌트를 액터 인스턴스에 등록
			MovementComp->RegisterComponent();
			RangeActor->AddInstanceComponent(MovementComp);

			// 기준 컴포넌트 지정 (반드시 Register 이후에 호출)
			MovementComp->SetUpdatedComponent(RangeActor->GetRootComponent());

			// 중요: UProjectileMovementComponent는 기본적으로 bInitialVelocityInLocalSpace = true 입니다.
			// 즉, 여기에 넣은 Velocity 값은 '로컬 좌표계'로 취급되어, 컴포넌트 내부에서 RangeActor의 회전값만큼 또 한 번 회전이 적용됩니다.
			// 이전에 RangeActor->GetActorForwardVector() (월드 좌표)를 넣었기 때문에 "월드 방향이 다시 한번 꺾이는" 이중 회전 현상이 발생하여 이상한 방향으로 날아간 것입니다.
			// 따라서, 로컬 기준 정면(X축)인 FVector::ForwardVector를 넣어주어야 올바르게 조준 방향으로 날아갑니다.
			MovementComp->Velocity = FVector::ForwardVector * Speed;
			MovementComp->Activate(true);
		}

		UWorld* World = RangeActor->GetWorld();
		if (IsValid(World))
		{
			FVector StartLoc = RangeActor->GetActorLocation();
			FVector EndLoc = StartLoc + (RangeActor->GetActorForwardVector() * 100.0f);
			DrawDebugLine(World, StartLoc, EndLoc, FColor::Blue, false, 10.0f, 0, 2.0f);
		}

		if (IsValid(World) && IsValid(MovementComp))
		{
			FVector StartLoc = RangeActor->GetActorLocation();

			// Velocity 벡터를 그대로 더하면 속도가 빠를 경우 선이 너무 길어질 수 있습니다.
			// 보통 방향 확인용으로는 길이를 적절히 조절(예: 150.f)해서 그립니다.
			FVector VelocityDir = MovementComp->Velocity.GetSafeNormal();
			FVector EndLoc = StartLoc + (VelocityDir * 150.0f);

			// 파란색은 Actor의 정면, 초록색은 실제 이동 속도(Velocity) 방향으로 구분하면 보기 편합니다.
			DrawDebugLine(World, StartLoc, EndLoc, FColor::White, false, 10.0f, 0, 3.0f);

			// 추가: 화살표로 그리면 방향 파악이 더 쉽습니다.
			DrawDebugDirectionalArrow(World, StartLoc, EndLoc, 50.f, FColor::Black, false, 10.0f, 0, 3.0f);
		}
	}
}

FTransform ULaunchProjectile::CalculateSpawnTransform(const FGameplayEffectSpec& GESpec, const AActor* Instigator) const
{
	const ULaunchProjectileConfig* Config = ResolveTypedConfigFromSpec<ULaunchProjectileConfig>(GESpec);
	if (!IsValid(Config) || !IsValid(Instigator))
	{
		return FTransform::Identity;
	}

	FVector SpawnLocation = Instigator->GetActorLocation();
	FRotator SpawnRotation = Instigator->GetActorRotation();

	// 1. 기본 위치 및 회전 결정 (Bone 고려)
	if (!Config->BoneName.IsNone())
	{
		const ACharacter* const Character = Cast<ACharacter>(Instigator);
		const USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : Instigator->FindComponentByClass<USkeletalMeshComponent>();
		
		if (IsValid(Mesh) && Mesh->DoesSocketExist(Config->BoneName))
		{
			SpawnLocation = Mesh->GetSocketLocation(Config->BoneName);
			
			// bUseInstigatorRotation이 false일 때만 본의 회전을 사용
			if (!Config->bUseInstigatorRotation)
			{
				SpawnRotation = Mesh->GetSocketRotation(Config->BoneName);
			}
		}
	}

	// 2. 발사 방향 결정 (Targeting - 타겟 위치가 있을 경우)
	if (Config->bUseEffectContextDirection)
	{
		FVector TargetLocation = SpawnLocation;
		const FGameplayEffectContextHandle &Context = GESpec.GetContext();
		if (Context.HasOrigin())
		{
			TargetLocation = Context.GetOrigin();
		}
		else if (const FHitResult *Hit = Context.GetHitResult())
		{
			if (!Hit->Location.IsZero())
			{
				TargetLocation = Hit->Location;
			}
			else if (Hit->GetActor())
			{
				TargetLocation = Hit->GetActor()->GetActorLocation();
			}
		}

		// 위치가 다를 때만 조준 회전값 적용
		if (!TargetLocation.Equals(SpawnLocation))
		{
			SpawnRotation = (TargetLocation - SpawnLocation).Rotation();
		}
	}

	// 3. Offset 및 특수 설정 적용
	if (Config->bSpawnZeroRotation)
	{
		SpawnRotation = FRotator::ZeroRotator;
	}
	
	SpawnRotation += Config->RotationOffset;

	// Local Offset 적용 (최종 회전값 기준)
	SpawnLocation += SpawnRotation.Quaternion().RotateVector(Config->LocationOffset);


	UWorld* const World = Instigator->GetWorld();
	if (IsValid(World))
	{
		FVector StartLoc = SpawnLocation;// 회전값에서 앞방향(X축) 벡터를 꺼내 100을 곱합니다.
		FVector EndLoc = StartLoc + SpawnRotation.Vector() * 200.0f;
		DrawDebugLine(World, StartLoc, EndLoc, FColor::Red, false, 10.0f, 0, 2.0f);
		//DrawDebugBox(World, SpawnLocation, Config->CollisionRadius, SpawnRotation.Quaternion(), FColor::Purple, false, 5.0f, 0, 2.0f);	
	}

	return FTransform(SpawnRotation, SpawnLocation);
}