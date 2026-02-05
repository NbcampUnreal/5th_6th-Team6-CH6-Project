// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/NoExportTypes.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "UI/UI_MainHUD.h"
#include "UI_HUDController.generated.h"

USTRUCT(BlueprintType)
struct FWidgetControllerParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<APlayerController> PlayerController = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<APlayerState> PlayerState = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAttributeSet> AttributeSet = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributeChangedSignature, float, NewValue);

UCLASS()
class PROJECTER_API UUI_HUDController : public UObject
{
	GENERATED_BODY()

public:
    UPROPERTY()
    TObjectPtr<UUI_MainHUD> MainHUDWidget;

    void BroadcastLVChanges(float CurrentLV);
    void BroadcastHPChanges(float CurrentHP, float MaxHP);
    void BroadcastStaminaChanges(float CurrentST, float MaxST);
    void BroadcastXPChanges(float CurrentXP, float MaxXP);
    void BroadcastATKChanges(float CurrentATK);
    void BroadcastSPChanges(float CurrentSP); // 스증
    void BroadcastASChanges(float CurrentAS);
    void BroadcastARChanges(float CurrentAR); // 사거리
    void BroadcastCCChanges(float CurrentCC); // 크리티컬 찬스 = CC
    void BroadcastDEFChanges(float CurrentDEF);
    void BroadcastSpeedChanges(float CurrentSpeed);
    void BroadcastCooldownReduction(float Cooldown);




    // 초기 설정!
    void SetParams(const FWidgetControllerParams& Params);

    // 바인딩하기
    virtual void BindCallbacksToDependencies();

    // 블루프린트용
    UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
    FOnAttributeChangedSignature OnHealthChanged;

    UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
    FOnAttributeChangedSignature OnMaxHealthChanged;

protected:
    // 데이터 보관용
    TObjectPtr<APlayerController> PlayerController;
    TObjectPtr<APlayerState> PlayerState;
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
    TObjectPtr<UAttributeSet> AttributeSet;
	
};
