// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UI/UI_ToolTip.h"
#include "Components/Widget.h"
#include "UI_ToolTipManager.generated.h"

UCLASS() // 언리얼 시스템에 등록
class PROJECTER_API UUI_ToolTipManager : public UObject
{
    GENERATED_BODY() // 언리얼 코드 생성 매크로

public:
    UUI_ToolTipManager();


    void setTooltipInstance(UUI_ToolTip* InTooltipInstance) { TooltipInstance = InTooltipInstance; }

    void ShowTooltip(UWidget* AnchorWidget, class UTexture2D* Icon, FText Name, FText ShortDesc, FText DetailDesc, bool showUpper);

private:
    UPROPERTY()
    UUI_ToolTip* TooltipInstance;
};