// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI_ToolTip.generated.h"


class UImage;
class UTextBlock;
/**
 * 
 */
UCLASS(Blueprintable, BlueprintType) // 블루프린트에 넣으려면 이거 꼭 넣어야됨;
class PROJECTER_API UUI_ToolTip : public UUserWidget
{
	GENERATED_BODY()
public:
	void UpdateTooltip(UTexture2D* Icon, FText Name, FText ShortDesc, FText DetailDesc);

protected:
	UPROPERTY(meta = (BindWidget))
	UImage* IconImage;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* txtName;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* txtShortDesc;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* txtLongDesc;
};
