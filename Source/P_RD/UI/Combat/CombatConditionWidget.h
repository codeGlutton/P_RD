#pragma once

#include "Components/Widget.h"
#include "Pawn/UnitCombatCondition.h"
#include "CombatConditionWidget.generated.h"

/** Resolution-independent face badge for a unit's actual turn condition. */
UCLASS()
class P_RD_API UCombatConditionWidget : public UWidget
{
	GENERATED_BODY()
public:
	void SetCondition(EUnitCombatCondition Value);
	EUnitCombatCondition GetCondition() const { return Condition; }
	static FText Describe(EUnitCombatCondition Value);
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
private:
	EUnitCombatCondition Condition = EUnitCombatCondition::Normal;
	TSharedPtr<SWidget> Face;
};
