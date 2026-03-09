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
#include "SkillSystem/SkillNiagaraSpawnHelper.h"

USummonRangeGEC::USummonRangeGEC()
{
	ConfigClass = USummonRangeByWorldOriginGECConfig::StaticClass();
}

TSubclassOf<UBaseGECConfig> USummonRangeGEC::GetRequiredConfigClass() const
{
	return USummonRangeByWorldOriginGECConfig::StaticClass();
}

void USummonRangeGEC::OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectApplied(ActiveGEContainer, GESpec, PredictionKey);

	const FGameplayEffectContextHandle& EffectContext = GESpec.GetEffectContext();
	const FGameplayEffectContext* EffectContextData = EffectContext.Get();
	if (EffectContextData == nullptr || !EffectContextData->HasOrigin())
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

	const USummonRangeByWorldOriginGECConfig* SpawnConfig = GetSpawnConfig(GESpec);
	if (!IsValid(SpawnConfig))
	{
		return;
	}

	const FTransform SpawnTransform = CalculateSpawnTransform(GESpec, SpawnConfig);
	const FTransform SummonerTransform = EffectCauser->GetActorTransform();
	const FVector RangeSpawnLocation = SpawnTransform.GetLocation();
	SkillNiagaraSpawnHelper::SpawnNiagaraBySettings(World, SpawnConfig->SummonerSpawnVfx, SummonerTransform, EffectCauser, &RangeSpawnLocation);
	SkillNiagaraSpawnHelper::SpawnNiagaraBySettings(World, SpawnConfig->RangeSpawnVfx, SpawnTransform, nullptr, nullptr);

	APawn* SpawnInstigator = Cast<APawn>(EffectContext.GetInstigator());
	AActor* DeferredSpawnedActor = World->SpawnActorDeferred<AActor>(SpawnConfig->RangeActorClass, SpawnTransform, EffectCauser, SpawnInstigator, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!IsValid(DeferredSpawnedActor))
	{
		return;
	}

	ABaseRangeOverlapEffectActor* RangeActor = Cast<ABaseRangeOverlapEffectActor>(DeferredSpawnedActor);
	if (!IsValid(RangeActor))
	{
		return;
	}

	InitializeRangeActor(RangeActor, SpawnConfig, EffectCauser, EffectContext);

	DeferredSpawnedActor->FinishSpawning(SpawnTransform);
}

const USummonRangeByWorldOriginGECConfig* USummonRangeGEC::GetSpawnConfig(const FGameplayEffectSpec& GESpec) const
{
	const USkillEffectDataAsset* SkillDataAsset = Cast<USkillEffectDataAsset>(GESpec.GetEffectContext().GetSourceObject());
	if (!IsValid(SkillDataAsset)) return nullptr;

	const FGameplayTag IndexTag = SkillDataAsset->GetIndexTag();
	const int32 DataIndex = FMath::RoundToInt(GESpec.GetSetByCallerMagnitude(IndexTag, false, -1.f));

	const FSkillEffectContainer& SkillContainer = SkillDataAsset->GetData();
	if (!SkillContainer.SkillEffectDefinition.IsValidIndex(DataIndex)) return nullptr;

	return Cast<USummonRangeByWorldOriginGECConfig>(SkillContainer.SkillEffectDefinition[DataIndex].Config);
}

FTransform USummonRangeGEC::CalculateSpawnTransform(const FGameplayEffectSpec& GESpec, const USummonRangeByWorldOriginGECConfig* Config) const
{
	if (!IsValid(Config)) return FTransform::Identity;

	const FGameplayEffectContextHandle& EffectContext = GESpec.GetEffectContext();
	const FGameplayEffectContext* EffectContextData = EffectContext.Get();

	// Origin 정보가 없거나 World가 없으면 실패
	if (EffectContextData == nullptr || !EffectContextData->HasOrigin()) return FTransform::Identity;
	UWorld* World = EffectContextData->GetInstigator() ? EffectContextData->GetInstigator()->GetWorld() : nullptr;
	if (!World) return FTransform::Identity;

	// 1. 초기 위치 설정 (World Origin)
	FVector TargetLocation = EffectContextData->GetOrigin();
	AActor* Instigator = EffectContextData->GetInstigator();

	// 2. 최종 회전값 계산
	FRotator CombinedRotation = FRotator::ZeroRotator;
	if (Config->bSpawnZeroRotation)
	{
		CombinedRotation = FRotator::ZeroRotator;
	}
	else if (Config->bLookAtTargetLocation && Instigator)
	{
		FVector InstigatorLoc = Instigator->GetActorLocation();
		CombinedRotation = FRotationMatrix::MakeFromX(TargetLocation - InstigatorLoc).Rotator();
		CombinedRotation.Pitch = 0.f;
		CombinedRotation.Roll = 0.f;
	}
	else
	{
		CombinedRotation = Config->RotationOffset;
	}

	// 3. 바닥 강제 고정 로직
	if (Config->bSnapToGround)
	{
		FHitResult FloorHit;
		FVector TraceStart = TargetLocation;
		FVector TraceEnd = TargetLocation;
		TraceEnd.Z -= 1000.f;

		FCollisionQueryParams Params;
		if (Instigator) Params.AddIgnoredActor(Instigator);

		if (World->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, Config->GroundTraceChannel, Params))
		{
			// X, Y는 유지, Z만 바닥에 맞춤
			TargetLocation.X = FloorHit.Location.X;
			TargetLocation.Y = FloorHit.Location.Y;

			float FinalZOffset = Config->FloatingHeight;
			if (Config->bUseBoxExtentOffset)
			{
				FinalZOffset += Config->CollisionRadius.Z;
			}

			TargetLocation.Z = FloorHit.Location.Z + FinalZOffset;
		}
	}

	DrawDebugBox(World, TargetLocation, Config->CollisionRadius, CombinedRotation.Quaternion(), FColor::Green, false, 5.0f, 0, 2.0f);

	return FTransform(CombinedRotation, TargetLocation);
}

void USummonRangeGEC::InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, const USummonRangeByWorldOriginGECConfig* Config, AActor* Causer, const FGameplayEffectContextHandle& Context) const
{
	UAbilitySystemComponent* CauserASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Causer);
	USkillBase* NonConstSkill = const_cast<USkillBase*>(Cast<USkillBase>(Context.GetAbility()));

	if (CauserASC && NonConstSkill)
	{
		TArray<FGameplayEffectSpecHandle> InitGEHandles;
		for (USkillEffectDataAsset* SkillEffectDataAsset : Config->Applied)
		{
			if (IsValid(SkillEffectDataAsset))
			{
				InitGEHandles.Append(SkillEffectDataAsset->MakeSpecs(CauserASC, NonConstSkill, Causer, Context));
			}
		}
		RangeActor->InitializeEffectData(InitGEHandles, Causer, Config->CollisionRadius, Config->bHitOncePerTarget, Config->HitTargetVfx);
		RangeActor->SetLifeSpan(Config->LifeSpan);
	}
}

FText USummonRangeByWorldOriginGECConfig::BuildTooltipDescription(float InLevel) const
{
	TArray<FString> AppliedDescriptions;

	for (const USkillEffectDataAsset* SkillEffectDataAsset : Applied)
	{
		if (!IsValid(SkillEffectDataAsset))
		{
			continue;
		}

		const FString Desc = SkillEffectDataAsset->BuildEffectDescription(InLevel).ToString();
		if (!Desc.IsEmpty())
		{
			AppliedDescriptions.Add(Desc);
		}
	}

	if (AppliedDescriptions.IsEmpty())
	{
		return FText::GetEmpty();
	}

	return FText::FromString(FString::Join(AppliedDescriptions, TEXT("\n")));
}
