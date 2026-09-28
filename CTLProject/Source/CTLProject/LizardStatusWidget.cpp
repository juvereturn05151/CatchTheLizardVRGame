#include "LizardStatusWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void ULizardStatusWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshDisplay();
}

void ULizardStatusWidget::SetStatus(float Percent, bool Stunned, bool Falling, float RemainingSeconds)
{
    // Float world-clock subtraction can leave 3.00000x at landing. Do not display an extra tenth.
    const float RoundedTime = FMath::CeilToFloat(FMath::Max(0.f, RemainingSeconds) * 10.f - .001f) / 10.f;
    if (FMath::IsNearlyEqual(ExposurePercent, Percent) && bStunned == Stunned && bFalling == Falling
        && FMath::IsNearlyEqual(StunSecondsRemaining, RoundedTime)) return;
    ExposurePercent = FMath::Clamp(Percent, 0.f, 100.f);
    bStunned = Stunned;
    bFalling = Falling;
    StunSecondsRemaining = RoundedTime;
    RefreshDisplay();
    OnStatusChanged();
}

void ULizardStatusWidget::RefreshDisplay()
{
    if (ExposureBar)
    {
        ExposureBar->SetPercent(ExposurePercent / 100.f);
        ExposureBar->SetVisibility(bStunned ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }
    if (StatusText)
    {
        if (bStunned && bFalling) StatusText->SetText(NSLOCTEXT("LizardSpray", "Falling", "STUNNED"));
        else if (bStunned)
        {
            FNumberFormattingOptions Format;
            Format.MinimumFractionalDigits = Format.MaximumFractionalDigits = 1;
            StatusText->SetText(FText::Format(NSLOCTEXT("LizardSpray", "StunTime", "STUNNED \u2014 {0}s"), FText::AsNumber(StunSecondsRemaining, &Format)));
        }
        else StatusText->SetText(FText::Format(NSLOCTEXT("LizardSpray", "Exposure", "EXPOSURE {0}%"), FText::AsNumber(FMath::RoundToInt(ExposurePercent))));
    }
}
