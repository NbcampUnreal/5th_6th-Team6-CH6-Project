// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Containers/Array.h"
#include "UI/UI_ToolTipManager.h" // 툴팁용
#include "UI_MainHUD.generated.h"

#define MAX_TEAMMATE	2

class UTextBlock;
class UButton;
class UProgressBar;
class UImage;
class UUI_ToolTip;
class UCharacterData;
class UAbilitySystemComponent;


UENUM(BlueprintType)
enum class ECharacterStat : uint8
{
	AD			UMETA(DisplayName = "Attack Damage"),
	AP			UMETA(DisplayName = "Ability Power"),
	DEF			UMETA(DisplayName = "Defence"),
	COOL		UMETA(DisplayName = "CoolDown"),
	AS			UMETA(DisplayName = "Attack Speed"),
	ATRAN		UMETA(DisplayName = "Attack Range"),
	CC			UMETA(DisplayName = "Critical Chance"),
	SPD			UMETA(DisplayName = "Speed"),
};

UENUM(BlueprintType)
enum class ESkillKey : uint8
{
	Q			UMETA(DisplayName = "Q Skill"),
	W			UMETA(DisplayName = "W Skill"),
	E			UMETA(DisplayName = "E Skill"),
	R			UMETA(DisplayName = "R Skill")
};
/**
 * 
 */
UCLASS()
class PROJECTER_API UUI_MainHUD : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "UI_MainHUD")
	void Update_LV(float CurrentLV);
	UFUNCTION(BlueprintCallable, Category = "UI_MainHUD")
	void Update_XP(float CurrentXP, float MaxXP);
	UFUNCTION(BlueprintCallable, Category = "UI_MainHUD")
	void Update_HP(float CurrentHP, float MaxHP);
	UFUNCTION(BlueprintCallable, Category = "UI_MainHUD")
	void UPdate_MP(float CurrentHP, float MaxHP);
	UFUNCTION(BlueprintCallable, Category = "UI_MainHUD")
	void ShowSkillUp(bool show);

	UFUNCTION(BlueprintCallable, Category = "UI_MainHUD")
	void setStat(ECharacterStat stat, int32 Value);
	
	UFUNCTION()
	void InitMinimapCompo(USceneCaptureComponent2D* SceneCapture2D);
	UFUNCTION()
	void InitHeroDataHUD(UCharacterData* HeroData);
	UFUNCTION()
	void InitASCHud(UAbilitySystemComponent* _ASC);

	UFUNCTION()
	void StartRespawn(float _RespawnTime);

protected:
	// 마우스 우클릭 확인용
	virtual void NativeConstruct() override; // 생성자
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// 툴팁 클래스 (에디터에서 할당)
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> TooltipClass;

	// 툴팁 인스턴스
	UPROPERTY()
	UUI_ToolTip* TooltipInstance;

	UPROPERTY()
	UUI_ToolTipManager* TooltipManager;

	// --- 마우스 오버 이벤트 핸들러 ---
	// 버튼용
	UFUNCTION() void OnSkill01Hovered();
	UFUNCTION() void OnSkill02Hovered();
	UFUNCTION() void OnSkill03Hovered();
	UFUNCTION() void OnSkill04Hovered();
	// .............

	void ShowTooltip(UWidget* AnchorWidget, UTexture2D* Icon, FText Name, FText ShortDesc, FText DetailDesc, bool showUpper);
	UFUNCTION()
	void HideTooltip();

private:
	void HandleMinimapClicked(const FPointerEvent& InMouseEvent);
	class USceneCaptureComponent2D* MinimapCaptureComponent;
	class UCharacterData* HeroData;
	class UAbilitySystemComponent* ASC;

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* stat_LV;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* PB_XP;

	///  stat_nn = 임시명칭
	UPROPERTY(meta = (BindWidget))
	UTextBlock* stat_01;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* stat_02;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* stat_03;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* stat_04;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* stat_05;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* stat_06;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* stat_07;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* stat_08;

	UPROPERTY(meta = (BindWidget))
	UButton* skill_01;

	UPROPERTY(meta = (BindWidget))
	UButton* skill_02;

	UPROPERTY(meta = (BindWidget))
	UButton* skill_03;

	UPROPERTY(meta = (BindWidget))
	UButton* skill_04;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* skill_cool_01;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* skill_cool_02;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* skill_cool_03;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* skill_cool_04;

	UPROPERTY(meta = (BindWidget))
	UButton* skill_up_01;

	UPROPERTY(meta = (BindWidget))
	UButton* skill_up_02;

	UPROPERTY(meta = (BindWidget))
	UButton* skill_up_03;

	UPROPERTY(meta = (BindWidget))
	UButton* skill_up_04;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* PB_HP;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* PB_MP;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* current_HP;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* max_HP;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* current_MP;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* max_MP;

	UPROPERTY(meta = (BindWidget))
	UImage* TEX_Minimap;

	UPROPERTY()
	UTexture2D* TEX_TempIcon;

	UPROPERTY(meta = (BindWidget))
	UImage* KillNumber_01;

	UPROPERTY(meta = (BindWidget))
	UImage* KillNumber_02;

	UPROPERTY(meta = (BindWidget))
	UImage* DeathNumber_01;

	UPROPERTY(meta = (BindWidget))
	UImage* DeathNumber_02;

	UPROPERTY(meta = (BindWidget))
	UImage* AssistNumber_01;

	UPROPERTY(meta = (BindWidget))
	UImage* AssistNumber_02;

	UPROPERTY(meta = (BindWidget))
	UImage* TeamHead_01;

	UPROPERTY(meta = (BindWidget))
	UImage* TeamHead_02;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* PB_TeamHP_01;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* PB_TeamHP_02;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TeamLevel_01;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TeamLevel_02;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextPhase;

	UPROPERTY(meta = (BindWidget))
	UImage* PhaseTimerMinTen;
	UPROPERTY(meta = (BindWidget))
	UImage* PhaseTimerMinOne;
	UPROPERTY(meta = (BindWidget))
	UImage* PhaseTimerSecTen;
	UPROPERTY(meta = (BindWidget))
	UImage* PhaseTimerSecOne;
	UPROPERTY(meta = (BindWidget))
	UImage* NowCurrentPhase;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* NowRespawnTime;

	UFUNCTION()
	void OnSkillClicked_Q();
	UFUNCTION()
	void OnSkillReleased_Q();
	UFUNCTION()
	void OnSkillClicked_W();
	UFUNCTION()
	void OnSkillReleased_W();
	UFUNCTION()
	void OnSkillClicked_E();
	UFUNCTION()
	void OnSkillReleased_E();
	UFUNCTION()
	void OnSkillClicked_R();
	UFUNCTION()
	void OnSkillReleased_R();
	UFUNCTION()
	void SkillFirePressed(ESkillKey index);
	UFUNCTION()
	void SkillFireReleased(ESkillKey index);

	UFUNCTION()
	void OnAbilityActivated(class UGameplayAbility* ActivatedAbility);

	UFUNCTION()
	void OnActivateSkillCoolTime(ESkillKey Skill_Index);

	// cool down 관리
protected:
	FTimerHandle SkillTimerHandles[4];
	float RemainingTimes[4];
	UPROPERTY()
	UTextBlock* SkillCoolTexts[4];
	void UpdateSkillCoolDown(int32 SkillIndex);

private:
	float nowSkillCoolReduc = 0.f;

	// SEVEN SEGMENT MAKER
protected:
	TArray<int32> GetDigitsFromNumber(int32 InNumber);
	
	UPROPERTY(EditAnywhere, Category = "UI_Resources")
	TArray<UTexture2D*> SegmentTextures;

public :
	void SetKillCount(int32 InKillCount);
	void SetDeathCount(int32 InDeathCount);
	void SetAssistCount(int32 InAssistCount);
	void UpdatePhaseAndTimeText();
	FTimerHandle PhaseAndTimeTimer;
private:
	UPROPERTY()
	class AER_GameState* GS;


	// DEBUG
private:
	FTimerHandle KillTimerHandle;
	void AddKillPerSecond();
	int32 CurrentKillCount = 0;

	// TEAM HUD Management
public:
	void UpdateTeamHP(int32 TeamIndex, float CurrentHP, float MaxHP);
	void UpdateTeamLV(int32 TeamIndex, int32 CurrentLV);

	// 체력 빨강 반짝 애니메이션 용
protected:
	UWidgetAnimation* GetWidgetAnimationByName(FName AnimName) const;

	UPROPERTY(Transient, BlueprintReadOnly)
	UWidgetAnimation* HeadHitAnim_01;

	UPROPERTY(Transient, BlueprintReadOnly)
	UWidgetAnimation* HeadHitAnim_02;

	float LastHP_01;
	float LastHP_02;

	float debugHP_01 = 1000.f;
	float debugHP_02 = 1000.f;
};
