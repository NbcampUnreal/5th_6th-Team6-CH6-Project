// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI_ChatSystem.generated.h"

class UTextBlock;
class UScrollBox;
class UEditableTextBox;

UCLASS()
class PROJECTER_API UUI_ChatSystem : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnChatCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UPROPERTY(EditAnywhere, Category = "Chat")
	FSlateColor ChatColor = FSlateColor(FLinearColor::White);

protected:
	UPROPERTY(meta = (BindWidget))
	UScrollBox* SB_TextScrollBox;

	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* ET_ChatInput;
public:
	UFUNCTION()
	void SetChatInputVisible(bool bVisible);

	UFUNCTION()
	void AddMessageToScrollBox(const FString& Message);

	UFUNCTION()
	void setPlayerState(class AER_PlayerState* InPS) { MyPS = InPS; }

	UFUNCTION()
	AER_PlayerState* getPlayerState() const { return MyPS; }

private:
	class AER_PlayerState* MyPS;

};
