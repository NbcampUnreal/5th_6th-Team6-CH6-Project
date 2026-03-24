// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UI_ToolTip.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void UUI_ToolTip::UpdateTooltip(FText Name, FText ShortDesc, FText DetailDesc, FText CostDesc)
{
    if (IsValid(txtName)) txtName->SetText(Name);
    if (IsValid(txtShortDesc)) txtShortDesc->SetText(ShortDesc);
    if (IsValid(txtLongDesc)) txtLongDesc->SetText(DetailDesc);
	if (IsValid(txtCostDesc)) txtCostDesc->SetText(CostDesc);

    // 레이아웃을 재계산
    ForceLayoutPrepass();
}
