// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UI_ToolTipManager.h"
#include "Blueprint/SlateBlueprintLibrary.h" // 툴팁용
#include "Blueprint/WidgetLayoutLibrary.h" // 툴팁용
#include "UI/UI_ToolTip.h" // 툴팁용
#include "Kismet/GameplayStatics.h"

UUI_ToolTipManager::UUI_ToolTipManager()
{
}

void UUI_ToolTipManager::ShowTooltip(UWidget* AnchorWidget, UTexture2D* Icon, FText Name, FText ShortDesc, FText DetailDesc, bool showUpper)
{
    if (!TooltipInstance || !AnchorWidget) return;
    
    TooltipInstance->UpdateTooltip(Icon, Name, ShortDesc, DetailDesc);
    TooltipInstance->SetVisibility(ESlateVisibility::HitTestInvisible);

    // 위젯의 위치 쓰던 말던 일단 가져오기
    FGeometry WidgetGeom = AnchorWidget->GetCachedGeometry();
    FVector2D PixelPos, ViewportPos, FinalPos;
    FVector2D ButtonSize = AnchorWidget->GetDesiredSize();

    // 버튼의 왼쪽 상단 0, 0의 절대 좌표 가져오기
    USlateBlueprintLibrary::LocalToViewport(GetWorld(), WidgetGeom, FVector2D(0, 0), PixelPos, ViewportPos);

    // 툴 팁 크기
    TooltipInstance->ForceLayoutPrepass();
    FVector2D DesiredSize = TooltipInstance->GetDesiredSize();
   
    /// 가변 해상도를 고려한 크기 계산을 위해 반드시 DPI 스케일을 곱해줘야 한다!!!!!!!!!!!!!!!!!!!
    float DPIScale = UWidgetLayoutLibrary::GetViewportScale(TooltipInstance);
    FVector2D ActualSize = DesiredSize * DPIScale;

    //UE_LOG(LogTemp, Error, TEXT("PixelPos : %f, %f"), PixelPos.X, PixelPos.Y);
    //UE_LOG(LogTemp, Error, TEXT("ViewportPos : %f, %f"), ViewportPos.X, ViewportPos.Y);
    //UE_LOG(LogTemp, Error, TEXT("ActualSize Size : %f, %f"), ActualSize.X, ActualSize.Y);
    //UE_LOG(LogTemp, Error, TEXT("ButtonSize Size : %f, %f"), ButtonSize.X, ButtonSize.Y);
   
    FinalPos.X = PixelPos.X - (ActualSize.X / 2) + (ButtonSize.X / 2);
    if(showUpper)
        FinalPos.Y = PixelPos.Y - ActualSize.Y;
    else
	    FinalPos.Y = PixelPos.Y + (ButtonSize.Y * 2);

    /// 툴팁이 밖으로 나갈 경우 안으로 들여보내기
    FVector2D ViewportSize;
    GEngine->GameViewport->GetViewportSize(ViewportSize);
    // 좌측 보정
    if (FinalPos.X + ActualSize.X > ViewportSize.X)
    {
        FinalPos.X = ViewportSize.X - ActualSize.X;
    }
    // 우측 보정
    if (FinalPos.X < 0.f)
    {
        FinalPos.X = 0.f;
    }

    // 상단 보정
    if (FinalPos.Y < 0.f)
    {
        // FinalPos.Y = 0.f;
        FinalPos.Y = PixelPos.Y + (ButtonSize.Y * 2);
    }
    // 하단 보정
    if (FinalPos.Y + ActualSize.Y > ViewportSize.Y)
    {
        // FinalPos.Y = ViewportSize.Y - ActualSize.Y;
        FinalPos.Y = PixelPos.Y - ActualSize.Y;
    }
    // 상단 하단의 경우 넘어서면 그냥 upper / lower를 토글하는 방식으로 처리하는게 이쁜것 갓다??
    // 주석 코드는 단순히 툴팁 크기만큼만 안 빠져 나가게 한다.

    TooltipInstance->SetPositionInViewport(FinalPos);
}
