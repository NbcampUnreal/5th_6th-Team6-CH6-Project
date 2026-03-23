#include "SkillSystem/GameplayEffectComponent/SummonRangeBaseGEC.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"
#include "GameplayEffect.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "SkillSystem/Actor/BaseRangeOverlapEffectActor/BaseRangeOverlapEffectActor.h"
#include "SkillSystem/GameAbility/SkillBase.h"
#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"
#include "SkillSystem/SkillNiagaraSpawnConfig.h"
#include "SkillSystem/GameplayEffectComponent/SummonRangeAtBone.h"

FText USummonRangeBaseConfig::BuildTooltipDescription(float InLevel) const
{
	TArray<FString> AppliedDescriptions;
	for (const USkillEffectDataAsset* const SkillEffectDataAsset : Applied)
	{
		if (!IsValid(SkillEffectDataAsset))
		{
			continue;
		}

		const FString Description = SkillEffectDataAsset->BuildEffectDescription(InLevel).ToString();
		if (Description.IsEmpty())
		{
			continue;
		}

		AppliedDescriptions.Add(Description);
	}

	if (AppliedDescriptions.IsEmpty())
	{
		return FText::GetEmpty();
	}

	return FText::FromString(FString::Join(AppliedDescriptions, TEXT("\n")));
}

void USummonRangeBaseGEC::OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectApplied(ActiveGEContainer, GESpec, PredictionKey);

	const FGameplayEffectContextHandle& ContextHandle = GESpec.GetEffectContext();
	const FGameplayEffectContext* const EffectContext = ContextHandle.Get();
	if (EffectContext == nullptr)
	{
		return;
	}

	AActor* const EffectInstigator = IsValid(ContextHandle.GetInstigator())
		? ContextHandle.GetInstigator()
		: ContextHandle.GetEffectCauser();
	if (!IsValid(EffectInstigator))
	{
		return;
	}

	if (!ShouldProcessOnInstigator(EffectInstigator))
	{
		return;
	}

	UWorld* const World = EffectInstigator->GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const USummonRangeBaseConfig* const SpawnConfig = ResolveTypedConfigFromSpec<USummonRangeBaseConfig>(GESpec);
	if (!IsValid(SpawnConfig) || !SpawnConfig->IsA(GetRequiredConfigClass()) || !IsValid(SpawnConfig->RangeActorClass))
	{
		return;
	}

	// 1. 타겟 식별 및 위치 계산
	AActor* const TargetActor = GetTargetActorFromContainer(ActiveGEContainer);
	const FTransform OriginTransform = CalculateOriginTransform(GESpec, EffectInstigator, TargetActor);
	const FTransform SpawnTransform = ApplyCommonSpawnOptionsToTransform(OriginTransform, SpawnConfig, EffectInstigator);
	const FVector RangeSpawnLocation = SpawnTransform.GetLocation();

	// 2. 액터 소환 (지연 생성)
	APawn* const SpawnInstigator = Cast<APawn>(ContextHandle.GetInstigator());
	ABaseRangeOverlapEffectActor* const DeferredSpawnedActor = World->SpawnActorDeferred<ABaseRangeOverlapEffectActor>(
		SpawnConfig->RangeActorClass,
		SpawnTransform,
		EffectInstigator,
		SpawnInstigator,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!IsValid(DeferredSpawnedActor))
	{
		return;
	}

	// 3. 초기화 및 마무리
	const FGameplayCueParameters HitTargetCueParameters = BuildNiagaraCueParameters(
		GESpec,
		IsValid(SpawnConfig->HitTargetVfx) ? SpawnConfig->HitTargetVfx->CueTag : FGameplayTag(),
		ContextHandle,
		DeferredSpawnedActor,
		RangeSpawnLocation,
		SpawnConfig->HitTargetVfx);

	InitializeRangeActor(DeferredSpawnedActor, SpawnConfig, EffectInstigator, ContextHandle, HitTargetCueParameters);
	DeferredSpawnedActor->FinishSpawning(SpawnTransform);

	// 4. 시각 효과 실행
	ExecuteGameplayCues(GESpec, ContextHandle, EffectInstigator, DeferredSpawnedActor, SpawnTransform, OriginTransform, SpawnConfig);
}

const USummonRangeBaseConfig* USummonRangeBaseGEC::GetSummonConfig(const FGameplayEffectSpec& GESpec) const
{
	return nullptr;
}

FTransform USummonRangeBaseGEC::CalculateSpawnTransform(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const AActor* TargetActor) const
{
	const FTransform OriginTransform = CalculateOriginTransform(GESpec, Instigator, TargetActor);
	const USummonRangeBaseConfig* const SpawnConfig = ResolveTypedConfigFromSpec<USummonRangeBaseConfig>(GESpec);
	return ApplyCommonSpawnOptionsToTransform(OriginTransform, SpawnConfig, Instigator);
}

FTransform USummonRangeBaseGEC::CalculateOriginTransform(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const AActor* TargetActor) const
{
	return FTransform::Identity;
}

void USummonRangeBaseGEC::ExecuteGameplayCues(const FGameplayEffectSpec& GESpec, const FGameplayEffectContextHandle& ContextHandle, AActor* EffectInstigator, ABaseRangeOverlapEffectActor* RangeActor, const FTransform& SpawnTransform, const FTransform& OriginTransform, const USummonRangeBaseConfig* Config) const
{
	UAbilitySystemComponent* const InstigatorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(EffectInstigator);
	if (!IsValid(InstigatorASC) || !IsValid(Config))
	{
		return;
	}

	FScopedPredictionWindow ForcedWindow(InstigatorASC, FPredictionKey(), false);

	if (IsValid(Config->SummonerSpawnVfx) && Config->SummonerSpawnVfx->CueTag.IsValid())
	{
		// 시전자 나이아가라: 원점 좌표(OriginTransform) 사용, EffectCauser = RangeActor (기본 동작)
		const FGameplayCueParameters SummonerCueParams = BuildNiagaraCueParameters(GESpec, Config->SummonerSpawnVfx->CueTag, ContextHandle, RangeActor, OriginTransform.GetLocation(), Config->SummonerSpawnVfx);
		InstigatorASC->ExecuteGameplayCue(Config->SummonerSpawnVfx->CueTag, SummonerCueParams);
	}

	if (IsValid(Config->RangeSpawnVfx) && Config->RangeSpawnVfx->CueTag.IsValid())
	{
		// 범위 나이아가라: 원점 좌표(OriginTransform) 사용, EffectCauser = RangeActor (기본 동작)
		FGameplayCueParameters RangeCueParams = BuildNiagaraCueParameters(GESpec, Config->RangeSpawnVfx->CueTag, ContextHandle, RangeActor, OriginTransform.GetLocation(), Config->RangeSpawnVfx, OriginTransform.GetRotation().GetForwardVector());
		if (IsValid(RangeActor))
		{
			RangeCueParams.TargetAttachComponent = RangeActor->GetRootComponent();
		}
		InstigatorASC->ExecuteGameplayCue(Config->RangeSpawnVfx->CueTag, RangeCueParams);
	}
}

AActor* USummonRangeBaseGEC::GetTargetActorFromContainer(FActiveGameplayEffectsContainer& ActiveGEContainer) const
{
	return ActiveGEContainer.Owner ? ActiveGEContainer.Owner->GetOwner() : nullptr;
}

bool USummonRangeBaseGEC::ShouldProcessOnInstigator(const AActor* Instigator) const
{
	return IsValid(Instigator);
}

FGameplayCueParameters USummonRangeBaseGEC::BuildNiagaraCueParameters(const FGameplayEffectSpec& GESpec, const FGameplayTag& OriginalTag, const FGameplayEffectContextHandle& EffectContext, AActor* EffectCauser, const FVector& CueLocation, const UObject* SourceObject, const FVector& CueNormal) const
{
	FGameplayCueParameters CueParams(GESpec);
	CueParams.OriginalTag = OriginalTag;
	CueParams.Instigator = EffectContext.GetInstigator();
	CueParams.EffectCauser = EffectCauser;
	CueParams.Location = CueLocation;
	CueParams.Normal = CueNormal;
	CueParams.GameplayEffectLevel = GESpec.GetLevel();

	if (SourceObject != nullptr)
	{
		CueParams.SourceObject = SourceObject;
	}

	return CueParams;
}

void USummonRangeBaseGEC::InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, const USummonRangeBaseConfig* Config, AActor* Instigator, const FGameplayEffectContextHandle& Context, const FGameplayCueParameters& HitTargetCueParameters) const
{
	if (!IsValid(RangeActor) || !IsValid(Config) || !IsValid(Instigator))
	{
		return;
	}

	UAbilitySystemComponent* const CauserASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator);
	USkillBase* const NonConstSkill = const_cast<USkillBase*>(Cast<USkillBase>(Context.GetAbility()));
	if (!IsValid(CauserASC) || !IsValid(NonConstSkill))
	{
		return;
	}

	TArray<FGameplayEffectSpecHandle> InitGEHandles;
	for (USkillEffectDataAsset* const SkillEffectDataAsset : Config->Applied)
	{
		if (!IsValid(SkillEffectDataAsset))
		{
			continue;
		}

		InitGEHandles.Append(SkillEffectDataAsset->MakeSpecs(CauserASC, NonConstSkill, RangeActor, Context));
	}

	// 강화 효과(SkillProc) 확인 및 전이
	UBaseGEC::GetSkillProcEffects(CauserASC, NonConstSkill, RangeActor, Context, InitGEHandles);

	RangeActor->InitializeEffectData(InitGEHandles, Instigator, Config->CollisionRadius, Config->bHitOncePerTarget, Config, HitTargetCueParameters);
	RangeActor->SetLifeSpan(Config->LifeSpan);
}

void USummonRangeBaseGEC::SnapLocationToGround(FVector& InOutLocation, const USummonRangeBaseConfig* Config, const AActor* Instigator) const
{
	if (!Config || !Config->bSnapToGround)
	{
		return;
	}

	UWorld* const World = IsValid(Instigator) ? Instigator->GetWorld() : nullptr;
	if (!IsValid(World))
	{
		return;
	}

	FHitResult FloorHit;
	FVector TraceEnd = InOutLocation;
	TraceEnd.Z -= 1000.0f;

	FCollisionQueryParams QueryParams;
	if (IsValid(Instigator))
	{
		QueryParams.AddIgnoredActor(Instigator);
	}

	if (World->LineTraceSingleByChannel(FloorHit, InOutLocation, TraceEnd, Config->GroundTraceChannel, QueryParams))
	{
		InOutLocation.X = FloorHit.Location.X;
		InOutLocation.Y = FloorHit.Location.Y;

		float FinalZOffset = Config->FloatingHeight;
		if (Config->bUseBoxExtentOffset)
		{
			FinalZOffset += Config->CollisionRadius.Z;
		}

		InOutLocation.Z = FloorHit.Location.Z + FinalZOffset;
	}
}

void USummonRangeBaseGEC::ApplyCommonSpawnOptions(FVector& InOutLocation, FRotator& InOutRotation, const USummonRangeBaseConfig* Config, const AActor* Instigator) const
{
	if (!Config)
	{
		return;
	}

	// 1. 회전 오프셋 적용
	InOutRotation += Config->RotationOffset;

	// 2. 위치 오프셋 적용 (최종 회전값 기준)
	InOutLocation += InOutRotation.Quaternion().RotateVector(Config->LocationOffset);

	// 3. 지면 스냅
	SnapLocationToGround(InOutLocation, Config, Instigator);
}

FTransform USummonRangeBaseGEC::ApplyCommonSpawnOptionsToTransform(const FTransform& InOriginTransform, const USummonRangeBaseConfig* Config, const AActor* Instigator) const
{
	FVector TargetLocation = InOriginTransform.GetLocation();
	FRotator CombinedRotation = InOriginTransform.Rotator();

	ApplyCommonSpawnOptions(TargetLocation, CombinedRotation, Config, Instigator);

	return FTransform(CombinedRotation, TargetLocation);
}