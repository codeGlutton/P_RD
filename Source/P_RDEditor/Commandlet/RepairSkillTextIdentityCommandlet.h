#pragma once
#include "Commandlets/Commandlet.h"
#include "RepairSkillTextIdentityCommandlet.generated.h"

UCLASS()
class URepairSkillTextIdentityCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    virtual int32 Main(const FString& Params) override;
};
