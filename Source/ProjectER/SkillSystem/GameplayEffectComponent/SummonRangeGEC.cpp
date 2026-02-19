// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayEffectComponent/SummonRangeGEC.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayEffect.h"
#include "GameFramework/Actor.h"
#include "SkillSystem/GameplayEffectComponent/BaseGECConfig.h"
#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"

USummonRangeGEC::USummonRangeGEC()
{
	ConfigClass = USummonRangeByWorldOriginGECConfig::StaticClass();
}

TSubclassOf<UBaseGECConfig> USummonRangeGEC::GetRequiredConfigClass() const
{
	return USummonRangeByWorldOriginGECConfig::StaticClass();
}

void USummonRangeGEC::OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectExecuted(ActiveGEContainer, GESpec, PredictionKey);

	const FGameplayEffectContextHandle& EffectContext = GESpec.GetEffectContext();
	const FGameplayEffectContext* EffectContextData = EffectContext.Get();
	const UObject* SourceObject = EffectContext.GetSourceObject();
	const USkillEffectDataAsset* SkillDataAsset = Cast<USkillEffectDataAsset>(SourceObject);
	const FGameplayTag IndexTag = SkillDataAsset ? SkillDataAsset->GetIndexTag() : FGameplayTag();
	const float DataIndex = IndexTag.IsValid() ? GESpec.GetSetByCallerMagnitude(IndexTag, false, -1.f) : -1.f;

	if (EffectContextData == nullptr || !EffectContextData->HasOrigin())
	{
		return;
	}

	const FSkillEffectDefinition& SkillDef = SkillDataAsset->GetData().SkillEffectDefinition[DataIndex];
	USummonRangeByWorldOriginGECConfig* SpawnConfig = Cast<USummonRangeByWorldOriginGECConfig>(SkillDef.Config);

	if (!IsValid(SpawnConfig) || !IsValid(SpawnConfig->RangeActorClass))
	{
		return;
	}

	AActor* EffectCauser = EffectContext.GetEffectCauser();
	if (!IsValid(EffectCauser) || !EffectCauser->HasAuthority())
	{
		return;
	}

	UWorld* World = EffectCauser->GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	FVector SpawnLocation = EffectContextData->GetOrigin();
	SpawnLocation.Z += SpawnConfig->ZOffset;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* SpawnedActor = World->SpawnActor<AActor>(
		SpawnConfig->RangeActorClass,
		SpawnLocation,
		SpawnConfig->SpawnRotation,
		SpawnParams
	);

	if (IsValid(SpawnedActor) && SpawnConfig->LifeSpan > 0.0f)
	{
		SpawnedActor->SetLifeSpan(SpawnConfig->LifeSpan);
	}
}
