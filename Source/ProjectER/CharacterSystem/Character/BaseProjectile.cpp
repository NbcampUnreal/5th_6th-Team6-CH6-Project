// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterSystem/Character/BaseProjectile.h"
#include "CharacterSystem/Interface/TargetableInterface.h"
#include "CharacterSystem/Data/ProjectileData.h" 

#include "Net/UnrealNetwork.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"

ABaseProjectile::ABaseProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	
	bReplicates = true; // 서버에서 생성 및 위치 동기화
	
	SetReplicateMovement(true);
	
	// 충돌체 설정
	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->InitSphereRadius(15.0f);
	SphereComp->SetCollisionProfileName(TEXT("Projectile")); // Projectile 프로필 필요 (Pawn과 Overlap)
	RootComponent = SphereComp;
	
	// 메시 설정
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(RootComponent);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// MeshComp->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	
	// 발사체 움직임 설정
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 2000.f;
	ProjectileMovement->MaxSpeed = 2000.f;
	ProjectileMovement->bRotationFollowsVelocity = true; // 날아가는 방향으로 회전
	ProjectileMovement->ProjectileGravityScale = 0.0f; // 직선으로 날아가게 (필요 시 조절)
	
	// [추가] 나이아가라 컴포넌트 생성
	ProjectileVFXComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ProjectileVFXComp"));
	ProjectileVFXComp->SetupAttachment(RootComponent); // 루트(Sphere)에 붙임
	ProjectileVFXComp->SetAutoActivate(false); // 데이터가 들어오면 켤 것이므로 일단 끔
}

void ABaseProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		// Instigator는 물리적으로 부딪히지 않게 설정
		if (GetInstigator())
		{
			SphereComp->IgnoreActorWhenMoving(GetInstigator(), true);
            
			// 만약 캐릭터 메쉬도 막고 싶다면
			if (UPrimitiveComponent* InstigatorComp = Cast<UPrimitiveComponent>(GetInstigator()->GetRootComponent()))
			{
				SphereComp->IgnoreComponentWhenMoving(InstigatorComp, true);
			}
			
			if (ACharacter* InstigatorChar = Cast<ACharacter>(GetInstigator()))
			{
				if (InstigatorChar->GetMesh())
				{
					SphereComp->IgnoreComponentWhenMoving(InstigatorChar->GetMesh(), true);
				}
			}
		}
		
		SphereComp->OnComponentBeginOverlap.AddDynamic(this, &ABaseProjectile::OnOverlapBegin);
	}
	
	if (ProjectileData)
	{
		InitializeProjectile();
	}
	
	SetLifeSpan(3.0f); // 3초 후 자동 삭제 (메모리 관리) : 평타가 안 맞을 경우	
}

void ABaseProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// 타겟이 유효한지 검사
	if (IsValid(HomingTargetActor))
	{
		// 타겟이 타겟팅 불가능 상태(사망 등)가 되었는지 확인
		if (ITargetableInterface* TargetObj = Cast<ITargetableInterface>(HomingTargetActor))
		{
			if (!TargetObj->IsTargetable())
			{
				// [선택 A] 타겟이 죽으면 그냥 투사체도 소멸 (깔끔함)
				SetActorTickEnabled(false); 
				Destroy();
				return;
			}
		}
	}
	else
	{
		// 타겟 자체가 소멸했다면(Destroy) 투사체도 삭제
		Destroy();
	}
}

void ABaseProjectile::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ABaseProjectile, ProjectileData);
	DOREPLIFETIME(ABaseProjectile, HomingTargetActor);
}

void ABaseProjectile::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp,
	bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);
	
	// Block 판정이 났을 때 로그 출력
	if (Other)
	{
		UE_LOG(LogTemp, Error, TEXT("BLOCK HAPPENED! Stopped by: %s"), *Other->GetName());
	}
}

void ABaseProjectile::OnRep_ProjectileData()
{
	InitializeProjectile();
}

void ABaseProjectile::OnRep_HomingTargetActor()
{
	InitializeProjectile();
}

void ABaseProjectile::InitializeProjectile()
{
	if (!ProjectileData) return;
	
	// 메쉬 설정 
	if (MeshComp->GetStaticMesh() != ProjectileData->ProjectileMesh)
	{
		// 메쉬 설정 
		if (ProjectileData->ProjectileMesh) 
		{
			MeshComp->SetStaticMesh(ProjectileData->ProjectileMesh);
			MeshComp->SetHiddenInGame(false); 
			// MeshComp->SetRelativeScale3D(ProjectileData->Scale); // <-- 아래 2번 항목 참조
		}
		else 
		{
			MeshComp->SetStaticMesh(nullptr); 
			MeshComp->SetHiddenInGame(true);
		}
	}
	
	if (ProjectileData->FlyVFX && ProjectileVFXComp->GetAsset() != ProjectileData->FlyVFX)
	{
		ProjectileVFXComp->SetAsset(ProjectileData->FlyVFX);
		ProjectileVFXComp->Activate();
	}
	
	SetActorScale3D(ProjectileData->Scale);
	
	// 무브먼트 설정
	if (ProjectileMovement)
	{
		// 유도 설정 (타겟이 있을 때만)
		if (HomingTargetActor)
		{
			ProjectileMovement->bIsHomingProjectile = true;
			ProjectileMovement->HomingTargetComponent = HomingTargetActor->GetRootComponent();
			ProjectileMovement->HomingAccelerationMagnitude = 30000.0f; // 강한 유도
			SetActorTickEnabled(true); // 추적 개시
		}
		else
		{
			ProjectileMovement->bIsHomingProjectile = false; // 타겟 없으면 유도 끄기
		}

		// 속도 설정
		ProjectileMovement->InitialSpeed = ProjectileData->InitialSpeed;
		ProjectileMovement->MaxSpeed = ProjectileData->MaxSpeed;
		ProjectileMovement->ProjectileGravityScale = ProjectileData->GravityScale;

		// 속도 갱신 
		if (ProjectileMovement->Velocity.IsZero() == false)
		{
			ProjectileMovement->Velocity = ProjectileMovement->Velocity.GetSafeNormal() * ProjectileData->InitialSpeed;
		}
	}
}

void ABaseProjectile::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                     UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
#if WITH_EDITOR
	if (OtherActor)
	{
		FString InstigatorName = GetInstigator() ? GetInstigator()->GetName() : TEXT("NULL");
		FString HitActorName = OtherActor->GetName();
		FString HitCompName = OtherComp ? OtherComp->GetName() : TEXT("NULL");

		// 로그창(Output Log)에 출력
		// 노란색 경고로 띄워서 눈에 잘 띄게 함
		UE_LOG(LogTemp, Warning, TEXT("[Projectile Hit] HitActor: %s (Comp: %s) / Shooter: %s"), 
			*HitActorName, *HitCompName, *InstigatorName);

		// 화면에 붉은 공 그리기 (3초간 유지)
		DrawDebugSphere(GetWorld(), GetActorLocation(), 30.0f, 12, FColor::Red, false, 3.0f);
        
		// 화면에 텍스트 띄우기 (누구랑 부딪혔는지 이름표)
		DrawDebugString(GetWorld(), GetActorLocation() + FVector(0,0,50), *HitActorName, nullptr, FColor::Yellow, 3.0f);
	}
#endif
	
	// 나 자신이나 이미 죽은 대상은 무시
	if (!OtherActor || OtherActor == GetInstigator()) return;

	// 아군 오사 방지 (Targetable Interface 활용)
	if (ITargetableInterface* MyInstigator = Cast<ITargetableInterface>(GetInstigator()))
	{
		if (ITargetableInterface* Target = Cast<ITargetableInterface>(OtherActor))
		{
			// 같은 팀이면 통과 (무시)
			if (MyInstigator->GetTeamType() == Target->GetTeamType()) return;
		}
	}
	
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	if (TargetASC && DamageEffectSpecHandle.IsValid())
	{
		TargetASC->ApplyGameplayEffectSpecToSelf(*DamageEffectSpecHandle.Data.Get());
	}
	
	// 이펙트 Multicast
	Multicast_SpawnImpactEffect(GetActorLocation());
	
	Destroy();
}

void ABaseProjectile::Multicast_SpawnImpactEffect_Implementation(FVector Location)
{
	if (ProjectileData && ProjectileData->ImpactVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ProjectileData->ImpactVFX, Location);
	}
}


