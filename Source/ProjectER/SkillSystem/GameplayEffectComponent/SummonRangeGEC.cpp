// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayEffectComponent/SummonRangeGEC.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "SkillSystem/GameplayEffectComponent/BaseGECConfig.h"
#include "SkillSystem/Actor/BaseRangeOverlapEffectActor.h"
#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"
#include "SkillSystem/GameAbility/SkillBase.h"

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
	if (EffectContextData == nullptr || !EffectContextData->HasOrigin())
	{
		return;
	}

	const USkillEffectDataAsset* SkillDataAsset = Cast<USkillEffectDataAsset>(EffectContext.GetSourceObject());
	if (!IsValid(SkillDataAsset))
	{
		return;
	}

	const FGameplayTag IndexTag = SkillDataAsset->GetIndexTag();
	if (!IndexTag.IsValid())
	{
		return;
	}

	const int32 DataIndex = FMath::RoundToInt(GESpec.GetSetByCallerMagnitude(IndexTag, false, -1.f));
	const FSkillEffectContainer SkillContainer = SkillDataAsset->GetData();
	if (!SkillContainer.SkillEffectDefinition.IsValidIndex(DataIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("USummonRangeGEC::OnGameplayEffectExecuted invalid DataIndex: %d"), DataIndex);
		return;
	}

	const FSkillEffectDefinition& SkillDef = SkillContainer.SkillEffectDefinition[DataIndex];
	const USummonRangeByWorldOriginGECConfig* SpawnConfig = Cast<USummonRangeByWorldOriginGECConfig>(SkillDef.Config);
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
	
	UAbilitySystemComponent* CauserASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(EffectCauser);
	if (!IsValid(CauserASC))
	{
		return;
	}

	FVector SpawnLocation = EffectContextData->GetOrigin();
	SpawnLocation.Z += SpawnConfig->ZOffset;

	FTransform SpawnTransform(SpawnConfig->SpawnRotation, SpawnLocation);
	APawn* SpawnInstigator = Cast<APawn>(EffectContext.GetInstigator());

	AActor* DeferredSpawnedActor = World->SpawnActorDeferred<AActor>(SpawnConfig->RangeActorClass, SpawnTransform, EffectCauser, SpawnInstigator, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	USkillBase* NonConstInstigatorSkill = const_cast<USkillBase*>(Cast<USkillBase>(EffectContext.GetAbility()));
	if (!IsValid(NonConstInstigatorSkill)) return;

	if (!IsValid(DeferredSpawnedActor)) return;
	ABaseRangeOverlapEffectActor* RangeActor = Cast<ABaseRangeOverlapEffectActor>(DeferredSpawnedActor);

	if (IsValid(RangeActor))
	{
		TArray<FGameplayEffectSpecHandle> InitGEHandles;
		for (USkillEffectDataAsset* SkillEffectDataAsset : SpawnConfig->Applied)
		{
			const TArray<FGameplayEffectSpecHandle> GameplayEffectSpecHandles = SkillEffectDataAsset->MakeSpecs(CauserASC, NonConstInstigatorSkill, EffectCauser, EffectContext);
			InitGEHandles.Append(GameplayEffectSpecHandles);
		}
		RangeActor->InitializeEffectData(InitGEHandles, EffectCauser, SpawnConfig->CollisionRadius, SpawnConfig->bHitOncePerTarget);
		RangeActor->SetLifeSpan(SpawnConfig->LifeSpan);
	}

	DeferredSpawnedActor->FinishSpawning(SpawnTransform);
}