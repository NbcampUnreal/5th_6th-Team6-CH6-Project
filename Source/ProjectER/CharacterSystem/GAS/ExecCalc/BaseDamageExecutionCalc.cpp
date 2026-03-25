#include "CharacterSystem/GAS/ExecCalc/BaseDamageExecutionCalc.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"
#include "CharacterSystem/GameplayTags/GameplayTags.h"

// 캡처할 속성 정의 (헬퍼 구조체)
struct FDamageStatics
{
	// 캡처할 속성 정의 (매크로 활용 가능)
	DECLARE_ATTRIBUTE_CAPTUREDEF(Defense);
	DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower); 
	DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingDamage);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalChance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalDamage);

	FDamageStatics()
	{
		// Target의 방어력을 가져옴 (Snapshot: false -> 적용 시점의 실시간 값 사용)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Defense, Target, false);
        
		// Source의 공격력을 가져옴
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, AttackPower, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, CriticalChance, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, CriticalDamage, Source, true);
	}
};

static const FDamageStatics& DamageStatics()
{
	static FDamageStatics DStatics;
	return DStatics;
}

UBaseDamageExecutionCalc::UBaseDamageExecutionCalc()
{
	// 캡처할 속성 등록
	RelevantAttributesToCapture.Add(DamageStatics().DefenseDef);
	RelevantAttributesToCapture.Add(DamageStatics().AttackPowerDef);
	RelevantAttributesToCapture.Add(DamageStatics().CriticalChanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().CriticalDamageDef);
}

void UBaseDamageExecutionCalc::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	Super::Execute_Implementation(ExecutionParams, OutExecutionOutput);
	
	// ASC 캐싱
    UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
    UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();

    // 태그 집계
    const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
    const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    FAggregatorEvaluateParameters EvaluationParameters;
    EvaluationParameters.SourceTags = SourceTags;
    EvaluationParameters.TargetTags = TargetTags;

    // Attribute 값 추출 (Capture)
    float Defense = 0.f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DefenseDef, EvaluationParameters, Defense);
    Defense = FMath::Max<float>(Defense, 0.0f); // 방어력은 음수가 되지 않도록
	
    float AttackPower = 0.f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().AttackPowerDef, EvaluationParameters, AttackPower);
	AttackPower = FMath::Max<float>(AttackPower, 0.0f);
	
	float CriticalChance = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalChanceDef, EvaluationParameters, CriticalChance);
	CriticalChance = FMath::Clamp<float>(CriticalChance, 0.0f, 100.0f); // 0~100% 제한

	float CriticalDamage = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalDamageDef, EvaluationParameters, CriticalDamage);
	CriticalDamage = FMath::Max<float>(CriticalDamage, 0.0f);
	
    // 기본 데미지 가져오기 (SetByCaller로 전달된 값 : ProjectER::Data::Amount::Damage)
    // 값이 없을 시 0.0f를 기본값으로 사용
    float BaseDamage = 0.f;
    BaseDamage = Spec.GetSetByCallerMagnitude(ProjectER::Data::Amount::Damage, false, 0.0f);

	// 공격력 계수
	float AttackCoefficient = 1.0f;
	
	// 공식: (기본 데미지 + 공격력 * 계수)
    float TotalDamage = BaseDamage + (AttackPower * AttackCoefficient);
    
	const bool bIsCritical = FMath::FRandRange(0.0f, 100.0f) < CriticalChance;
	
	if (bIsCritical)
	{
		float CritMultiplier = 1.5f + (CriticalDamage / 100.0f);
        
		TotalDamage *= CritMultiplier;
	}
	
    // 방어력 적용 (100 / (100 + 방어력)) -> 방어력이 높을수록 데미지 감소
    float Mitigation = 100.0f / (100.0f + Defense);
    float FinalDamage = TotalDamage * Mitigation;
	
    // 결과 출력 (IncomingDamage Attribute에 적용)
    if (FinalDamage > 0.f)
    {
        OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UBaseAttributeSet::GetIncomingDamageAttribute(), EGameplayModOp::Additive, FinalDamage));
    }
}