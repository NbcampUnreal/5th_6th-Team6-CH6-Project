// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayEffectComponent/LaunchHomingMissile.h"
#include "SkillSystem/Actor/BaseMissileActor/BaseMissileActor.h"
#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"
#include "SkillSystem/GameAbility/SkillBase.h"
#include "SkillSystem/SkillNiagaraSpawnConfig.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

// ============================================================================
// Config
// ============================================================================

FText ULaunchHomingMissileConfig::BuildTooltipDescription(float InLevel) const
{
	TArray<FString> Descriptions;
	for (const USkillEffectDataAsset* const EffectData : Applied)
	{
		if (!IsValid(EffectData))
		{
			continue;
		}
		const FString Desc = EffectData->BuildEffectDescription(InLevel).ToString();
		if (!Desc.IsEmpty())
		{
			Descriptions.Add(Desc);
		}
	}

	if (Descriptions.IsEmpty())
	{
		return FText::GetEmpty();
	}

	return FText::FromString(FString::Join(Descriptions, TEXT("\n")));
}

// ============================================================================
// GEC
// ============================================================================

ULaunchHomingMissile::ULaunchHomingMissile()
{
	ConfigClass = ULaunchHomingMissileConfig::StaticClass();
}

TSubclassOf<UBaseGECConfig> ULaunchHomingMissile::GetRequiredConfigClass() const
{
	return ULaunchHomingMissileConfig::StaticClass();
}

void ULaunchHomingMissile::OnGameplayEffectApplied(
	FActiveGameplayEffectsContainer& ActiveGEContainer,
	FGameplayEffectSpec& GESpec,
	FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectApplied(ActiveGEContainer, GESpec, PredictionKey);

	// --- Context 검증 ---
	const FGameplayEffectContextHandle& ContextHandle = GESpec.GetEffectContext();
	const FGameplayEffectContext* EffectContext = ContextHandle.Get();
	if (!EffectContext)
	{
		return;
	}

	AActor* const Instigator = IsValid(ContextHandle.GetInstigator())
		? ContextHandle.GetInstigator()
		: ContextHandle.GetEffectCauser();
	if (!IsValid(Instigator))
	{
		return;
	}

	UWorld* const World = Instigator->GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	// --- Config 검증 ---
	const ULaunchHomingMissileConfig* const MissileConfig = ResolveTypedConfigFromSpec<ULaunchHomingMissileConfig>(GESpec);
	if (!IsValid(MissileConfig) || !IsValid(MissileConfig->MissileActorClass))
	{
		return;
	}

	// --- 타겟 및 스폰 위치 계산 ---
	AActor* const TargetActor = ContextHandle.GetHitResult()->GetActor();
	if (!IsValid(TargetActor)) return;
	//AActor* const TargetActor = GetTargetActorFromContainer(ContextHandle);
	const FTransform SpawnTransform = CalculateSpawnTransform(MissileConfig, Instigator, TargetActor);

	// --- 미사일 액터 지연 생성 ---
	APawn* const SpawnInstigator = Cast<APawn>(ContextHandle.GetInstigator());
	ABaseMissileActor* const MissileActor = World->SpawnActorDeferred<ABaseMissileActor>(
		MissileConfig->MissileActorClass,
		SpawnTransform,
		Instigator,
		SpawnInstigator,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!IsValid(MissileActor))
	{
		return;
	}

	// --- 효과 Spec 생성 ---
	UAbilitySystemComponent* const CauserASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator);
	USkillBase* const Skill = const_cast<USkillBase*>(Cast<USkillBase>(ContextHandle.GetAbility()));

	TArray<FGameplayEffectSpecHandle> EffectSpecs;
	if (IsValid(CauserASC) && IsValid(Skill))
	{
		for (USkillEffectDataAsset* const EffectData : MissileConfig->Applied)
		{
			if (IsValid(EffectData))
			{
				EffectSpecs.Append(EffectData->MakeSpecs(CauserASC, Skill, MissileActor, ContextHandle));
			}
		}

		// 강화 효과(SkillProc) 확인 및 전이
		UBaseGEC::GetSkillProcEffects(CauserASC, Skill, MissileActor, ContextHandle, EffectSpecs);
	}
	
	// --- 적중 VFX 큐 파라미터 구성 ---
	FGameplayCueParameters HitCueParams(GESpec);
	if (IsValid(MissileConfig->ImpactVfx) && MissileConfig->ImpactVfx->CueTag.IsValid())
	{
		HitCueParams.OriginalTag = MissileConfig->ImpactVfx->CueTag;
		HitCueParams.Instigator = ContextHandle.GetInstigator();
		HitCueParams.EffectCauser = MissileActor;
		HitCueParams.Location = SpawnTransform.GetLocation();
		HitCueParams.SourceObject = MissileConfig->ImpactVfx;
		HitCueParams.GameplayEffectLevel = GESpec.GetLevel();
	}

	// --- 미사일 초기화 ---
	MissileActor->InitializeMissile(
		EffectSpecs,
		Instigator,
		TargetActor,
		HitCueParams,
		MissileConfig->InitialSpeed,
		MissileConfig->MaxSpeed,
		MissileConfig->HomingAccelerationMagnitude,
		MissileConfig->ReachThreshold,
		MissileConfig->bDestroyOnHit
	);
	MissileActor->SetLifeSpan(MissileConfig->LifeSpan);

	// --- 스폰 완료 ---
	MissileActor->FinishSpawning(SpawnTransform);

	// --- Summoner / Missile VFX 실행 ---
	ExecuteVfx(GESpec, ContextHandle, Instigator, MissileActor, MissileConfig);
}

FTransform ULaunchHomingMissile::CalculateSpawnTransform(
	const ULaunchHomingMissileConfig* Config,
	const AActor* Instigator,
	const AActor* TargetActor) const
{
	if (!IsValid(Config) || !IsValid(Instigator))
	{
		return FTransform::Identity;
	}

	FVector SpawnLocation = Instigator->GetActorLocation();
	FRotator SpawnRotation = Instigator->GetActorRotation();

	// 1. 본(Bone) 위치 사용
	if (!Config->BoneName.IsNone())
	{
		const ACharacter* Character = Cast<ACharacter>(Instigator);
		const USkeletalMeshComponent* Mesh = Character
			? Character->GetMesh()
			: Instigator->FindComponentByClass<USkeletalMeshComponent>();

		if (IsValid(Mesh) && Mesh->DoesSocketExist(Config->BoneName))
		{
			SpawnLocation = Mesh->GetSocketLocation(Config->BoneName);
		}
	}

	// 2. 타겟을 향한 방향 계산
	if (IsValid(TargetActor))
	{
		FVector Dir = TargetActor->GetActorLocation() - SpawnLocation;
		if (!Dir.IsNearlyZero())
		{
			SpawnRotation = Dir.Rotation();
		}
	}

	return FTransform(SpawnRotation, SpawnLocation);
}

AActor* ULaunchHomingMissile::GetTargetActorFromContainer(FActiveGameplayEffectsContainer& ActiveGEContainer) const
{
	return ActiveGEContainer.Owner ? ActiveGEContainer.Owner->GetOwner() : nullptr;
}

void ULaunchHomingMissile::ExecuteVfx(
	const FGameplayEffectSpec& GESpec,
	const FGameplayEffectContextHandle& ContextHandle,
	AActor* Instigator,
	ABaseMissileActor* MissileActor,
	const ULaunchHomingMissileConfig* Config) const
{
	UAbilitySystemComponent* const InstigatorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator);
	if (!IsValid(InstigatorASC) || !IsValid(Config))
	{
		return;
	}

	{
		FScopedPredictionWindow ForcedWindow(InstigatorASC, FPredictionKey(), false);

		// Summoner VFX (시전자 위치에 재생)
		if (IsValid(Config->SummonerVfx) && Config->SummonerVfx->CueTag.IsValid())
		{
			FGameplayCueParameters SummonerParams(GESpec);
			SummonerParams.OriginalTag = Config->SummonerVfx->CueTag;
			SummonerParams.Instigator = ContextHandle.GetInstigator();
			SummonerParams.EffectCauser = MissileActor;
			SummonerParams.Location = Instigator->GetActorLocation();
			SummonerParams.SourceObject = Config->SummonerVfx;
			SummonerParams.GameplayEffectLevel = GESpec.GetLevel();
			InstigatorASC->ExecuteGameplayCue(Config->SummonerVfx->CueTag, SummonerParams);
		}

		// Missile VFX (미사일 본체에 부착되어 이동하며 재생)
		if (IsValid(Config->MissileVfx) && Config->MissileVfx->CueTag.IsValid())
		{
			FGameplayCueParameters MissileParams(GESpec);
			MissileParams.OriginalTag = Config->MissileVfx->CueTag;
			MissileParams.Instigator = ContextHandle.GetInstigator();
			MissileParams.EffectCauser = MissileActor;
			MissileParams.Location = MissileActor->GetActorLocation();
			MissileParams.Normal = MissileActor->GetActorForwardVector();
			MissileParams.SourceObject = Config->MissileVfx;
			MissileParams.GameplayEffectLevel = GESpec.GetLevel();
			if (IsValid(MissileActor))
			{
				MissileParams.TargetAttachComponent = MissileActor->GetRootComponent();
			}
			InstigatorASC->ExecuteGameplayCue(Config->MissileVfx->CueTag, MissileParams);
		}
	}
}
