#pragma once

#include "Commandlets/Commandlet.h"
#include "RefreshPullDescriptionsCommandlet.generated.h"

// Preview stale generated pull descriptions; pass -Apply to save replacements.
// Use -Asset=<full object path> to regenerate one explicitly selected skill.
UCLASS()
class URefreshPullDescriptionsCommandlet : public UCommandlet
{

	GENERATED_BODY()
public:
	URefreshPullDescriptionsCommandlet();
	int32 Main(const FString& Params) override;
};
