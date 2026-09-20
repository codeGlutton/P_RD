#pragma once
#include "Blueprint/UserWidget.h"
#include "CombatSpeedWidget.generated.h"
class UButton;
class UTextBlock;
class UCombatUIModel;

UCLASS()
class P_RD_API UCombatSpeedWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UCombatSpeedWidget(const FObjectInitializer& Initializer = FObjectInitializer::Get());
    void Configure(UCombatUIModel* Model);
    UButton* GetSpeedButton() const { return Button; }
protected:
    virtual bool Initialize() override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
        FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool Enabled) const override;
private:
    UFUNCTION() void Clicked();
    UPROPERTY() TObjectPtr<class UTexture2D> Frame;
    UPROPERTY() TObjectPtr<UButton> Button;
    UPROPERTY() TObjectPtr<UTextBlock> Label;
    TWeakObjectPtr<UCombatUIModel> ViewModel;
};
