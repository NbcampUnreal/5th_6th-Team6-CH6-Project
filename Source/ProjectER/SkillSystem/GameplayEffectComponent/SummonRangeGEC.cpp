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


	const FGameplayEffectContextHandle& ContextHandle = GESpec.GetContext();
	if (ContextHandle.IsValid())
	{
		// 여기서 직접 체크
		FVector Origin = ContextHandle.GetOrigin();
		bool bHasOrigin = ContextHandle.HasOrigin();

		UE_LOG(LogTemp, Warning, TEXT("Direct Handle Check: %d, %s"), bHasOrigin, *Origin.ToString());
	}

	const FGameplayEffectContextHandle& EffectContext = GESpec.GetEffectContext();
	const FGameplayEffectContext* EffectContextData = EffectContext.Get();
	const UObject* SourceObject = EffectContext.GetSourceObject();
	const USkillEffectDataAsset* SkillDataAsset = Cast<USkillEffectDataAsset>(SourceObject);
	const FGameplayTag IndexTag = SkillDataAsset ? SkillDataAsset->GetIndexTag() : FGameplayTag();
	const float DataIndex = IndexTag.IsValid() ? GESpec.GetSetByCallerMagnitude(IndexTag, false, -1.f) : -1.f;
	UE_LOG(LogTemp, Warning, TEXT("[GEC_TRACE_V2] OnGameplayEffectExecuted:: GE=%s SourceObject=%s HasOrigin=%d Origin=%s IndexTag=%s DataIndex=%.0f"),
		*GetNameSafe(GESpec.Def),
		*GetNameSafe(SourceObject),
		EffectContextData && EffectContextData->HasOrigin() ? 1 : 0,
		EffectContextData ? *EffectContextData->GetOrigin().ToString() : TEXT("None"),
		*IndexTag.ToString(),
		DataIndex
	);

	if (EffectContextData == nullptr || !EffectContextData->HasOrigin())
	{
		UE_LOG(LogTemp, Warning, TEXT("OnGameplayEffectExecuted::2 is false"));
		return;
	}

	USummonRangeByWorldOriginGECConfig* SpawnConfig = nullptr;
	SkillDataAsset = Cast<USkillEffectDataAsset>(EffectContext.GetSourceObject());
	const FSkillEffectDefinition& SkillDef = SkillDataAsset->GetData().SkillEffectDefinition[DataIndex];
	SpawnConfig = Cast<USummonRangeByWorldOriginGECConfig>(SkillDef.Config);
	//const USkillEffectDataAsset* SkillDataAsset = Cast<USkillEffectDataAsset>(EffectContext.GetSourceObject());
	//if (IsValid(SkillDataAsset))
	//{
	//	const FGameplayTag IndexTag = SkillDataAsset->GetIndexTag();
	//	int32 DataIndex = FMath::RoundToInt(GESpec.GetSetByCallerMagnitude(IndexTag, false, -1.f));
	//	const FSkillEffectDefinition& SkillDef = SkillDataAsset->GetData().SkillEffectDefinition[DataIndex];
	//	SpawnConfig = Cast<USummonRangeByWorldOriginGECConfig>(SkillDef.Config);
	//	if (!IsValid(SpawnConfig))
	//	{
	//		return;
	//	}
	//}

	if (!IsValid(SpawnConfig) || !IsValid(SpawnConfig->RangeActorClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("OnGameplayEffectExecuted::1 is false"));
		return;
	}

	AActor* EffectCauser = EffectContext.GetEffectCauser();
	if (!IsValid(EffectCauser) || !EffectCauser->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("OnGameplayEffectExecuted::3 is false"));
		return;
	}

	UWorld* World = EffectCauser->GetWorld();
	if (!IsValid(World))
	{
		UE_LOG(LogTemp, Warning, TEXT("OnGameplayEffectExecuted::4 is false"));
		return;
	}

	FVector SpawnLocation = EffectContextData->GetOrigin();
	UE_LOG(LogTemp, Warning, TEXT("OnGameplayEffectExecuted : %s"), *EffectContextData->GetOrigin().ToString());
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
