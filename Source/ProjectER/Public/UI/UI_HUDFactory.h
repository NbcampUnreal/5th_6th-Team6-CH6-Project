// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "UI_HUDFactory.generated.h"

class UUserWidget;
class UUI_HUDController;
class UUI_MainHUD;
class UCharacterData;
class UAbilitySystemComponent;
struct FWidgetControllerParams;

UCLASS()
class PROJECTER_API AUI_HUDFactory : public AHUD
{
	GENERATED_BODY()

public:

    // 2. 실제 생성된 위젯 인스턴스를 보관할 변수
    UPROPERTY()
    TObjectPtr<UUI_MainHUD> MainWidget;
	virtual void BeginPlay() override;

    // 캐릭터에서 호출할 초기화 함수
    UFUNCTION()
    void InitOverlay(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS);
    UFUNCTION()
    void InitMinimapComponent(class USceneCaptureComponent2D* SceneCapture2D);
    UFUNCTION()
    void InitHeroDataFactory(UCharacterData* HeroData);
    UFUNCTION()
    void InitASCFactory(UAbilitySystemComponent* _ASC);

    UFUNCTION()
    UUI_MainHUD* getMainHUD();


private:
    // 1. 위젯 클래스를 담을 변수 (에디터에서 선택 가능하게)
    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UUI_MainHUD> MainHUDWidgetClass;

    UPROPERTY()
    TObjectPtr<UUI_HUDController> WidgetController;

    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UUI_HUDController> WidgetControllerClass; // 블루프린트에서 컨트롤러 클래스 할당용
	
};
