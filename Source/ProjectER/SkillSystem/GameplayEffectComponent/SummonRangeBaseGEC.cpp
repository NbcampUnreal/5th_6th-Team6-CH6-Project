#include "SkillSystem/GameplayEffectComponent/SummonRangeBaseGEC.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"
#include "GameplayEffect.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "SkillSystem/Actor/BaseRangeOverlapEffectActor.h"
#include "SkillSystem/GameAbility/SkillBase.h"
#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"

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

	const USummonRangeBaseConfig* const SpawnConfig = GetSummonConfig(GESpec);
	if (!IsValid(SpawnConfig) || !IsValid(SpawnConfig->RangeActorClass))
	{
		return;
	}

	const FTransform SpawnTransform = CalculateSpawnTransform(GESpec, EffectInstigator, SpawnConfig);
	const FVector RangeSpawnLocation = SpawnTransform.GetLocation();
	const FTransform SummonerTransform = EffectInstigator->GetActorTransform();

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

	const FGameplayCueParameters HitTargetCueParameters = BuildNiagaraCueParameters(
		GESpec,
		SpawnConfig->HitTargetVfx.CueTag,
		ContextHandle,
		DeferredSpawnedActor,
		RangeSpawnLocation,
		SpawnConfig);

	InitializeRangeActor(DeferredSpawnedActor, SpawnConfig, EffectInstigator, ContextHandle, HitTargetCueParameters);
	DeferredSpawnedActor->FinishSpawning(SpawnTransform);

	const FGameplayCueParameters SummonerCueParams = BuildNiagaraCueParameters(
		GESpec,
		SpawnConfig->SummonerSpawnVfx.CueTag,
		ContextHandle,
		DeferredSpawnedActor,
		SummonerTransform.GetLocation(),
		SpawnConfig);

	const FGameplayCueParameters RangeCueParams = BuildNiagaraCueParameters(
		GESpec,
		SpawnConfig->RangeSpawnVfx.CueTag,
		ContextHandle,
		DeferredSpawnedActor,
		RangeSpawnLocation,
		SpawnConfig);

	UAbilitySystemComponent* const InstigatorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(EffectInstigator);
	if (!IsValid(InstigatorASC))
	{
		return;
	}

	FScopedPredictionWindow ForcedWindow(InstigatorASC, FPredictionKey(), false);
	if (SpawnConfig->SummonerSpawnVfx.CueTag.IsValid())
	{
		InstigatorASC->ExecuteGameplayCue(SpawnConfig->SummonerSpawnVfx.CueTag, SummonerCueParams);
	}

	if (SpawnConfig->RangeSpawnVfx.CueTag.IsValid())
	{
		InstigatorASC->ExecuteGameplayCue(SpawnConfig->RangeSpawnVfx.CueTag, RangeCueParams);
	}
}

const USummonRangeBaseConfig* USummonRangeBaseGEC::GetSummonConfig(const FGameplayEffectSpec& GESpec) const
{
	return nullptr;
}

FTransform USummonRangeBaseGEC::CalculateSpawnTransform(const FGameplayEffectSpec& GESpec, const AActor* Instigator) const
{
	return FTransform();
}

bool USummonRangeBaseGEC::ShouldProcessOnInstigator(const AActor* Instigator) const
{
	return IsValid(Instigator);
}

const UBaseGECConfig* USummonRangeBaseGEC::ResolveBaseConfigFromSpec(const FGameplayEffectSpec& GESpec) const
{
	const USkillEffectDataAsset* const SkillDataAsset = Cast<USkillEffectDataAsset>(GESpec.GetEffectContext().GetSourceObject());
	if (!IsValid(SkillDataAsset))
	{
		return nullptr;
	}

	const FGameplayTag IndexTag = SkillDataAsset->GetIndexTag();
	const int32 DataIndex = FMath::RoundToInt(GESpec.GetSetByCallerMagnitude(IndexTag, false, -1.0f));
	const FSkillEffectContainer& SkillContainer = SkillDataAsset->GetData();
	if (!SkillContainer.SkillEffectDefinition.IsValidIndex(DataIndex))
	{
		return nullptr;
	}

	return SkillContainer.SkillEffectDefinition[DataIndex].Config;
}

FGameplayCueParameters USummonRangeBaseGEC::BuildNiagaraCueParameters(const FGameplayEffectSpec& GESpec, const FGameplayTag& OriginalTag, const FGameplayEffectContextHandle& EffectContext, AActor* EffectCauser, const FVector& CueLocation, const UObject* SourceObject) const
{
	FGameplayCueParameters CueParams(GESpec);
	CueParams.OriginalTag = OriginalTag;
	CueParams.Instigator = EffectContext.GetInstigator();
	CueParams.EffectCauser = EffectCauser;
	CueParams.Location = CueLocation;
	CueParams.Normal = FVector::UpVector;
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

	RangeActor->InitializeEffectData(InitGEHandles, Instigator, Config->CollisionRadius, Config->bHitOncePerTarget, Config, HitTargetCueParameters);
	RangeActor->SetLifeSpan(Config->LifeSpan);
}