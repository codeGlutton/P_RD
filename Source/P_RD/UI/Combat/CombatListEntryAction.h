#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "CombatListEntryAction.generated.h"

/** Retains the item identity for a button in a dynamically sized list. */
UCLASS()
class P_RD_API UCombatListEntryAction : public UObject
{
    GENERATED_BODY()
  public:
    FSimpleDelegate Action;
    UFUNCTION() void Invoke()
    {
        Action.ExecuteIfBound();
    }
};
