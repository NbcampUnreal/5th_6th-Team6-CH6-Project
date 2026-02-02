// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UI_ToolTip.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void UUI_ToolTip::UpdateTooltip(UTexture2D* Icon, FText Name, FText ShortDesc, FText DetailDesc)
{
    if (IsValid(IconImage)) IconImage->SetBrushFromTexture(Icon);
    if (IsValid(txtName)) txtName->SetText(Name);
    if (IsValid(txtShortDesc)) txtShortDesc->SetText(ShortDesc);
    if (IsValid(txtLongDesc)) txtLongDesc->SetText(DetailDesc);

    // 레이아웃을 재계산
    ForceLayoutPrepass();
}
