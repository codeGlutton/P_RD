/*****************************************************************//**
 * @file   PartyModel.h
 * @brief  파티 모델 클래스 정의 헤더
 * @author 모호재
 * @date   2026-07-26
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Actor/ActorModel.h"
#include "PartyModel.generated.h"

class UPlayerUnitModel;
class UAttributeSetComponentModel;
class UPartyArtifactComponentModel;
class UPartyAttributeSet;

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnChangePartyPlayer, int32 /*PlayerIndex*/, UPlayerUnitModel* /*PreModel*/, UPlayerUnitModel* /*NextModel*/);

/**
 * @brief  플레이어 모델을 관리하는 파티 모델 클래스
 */
UCLASS()
class P_RD_API UPartyModel : public UActorModel
{
	GENERATED_BODY()

public:
	UPartyModel();

public:
	void SetPlayerUnitModel(int32 PlayerIndex, UPlayerUnitModel* PlayerUnitModel);
	bool AddPlayerUnitModel(UPlayerUnitModel* NewPlayerUnitModel);
	bool RemovePlayerUnitModel(UPlayerUnitModel* ExistPlayerUnitModel);

	void SetDifficulty(int32 Difficulty);

public:
	TArray<TObjectPtr<UPlayerUnitModel>>& GetPlayerUnitModels();
	const TArray<TObjectPtr<UPlayerUnitModel>>& GetPlayerUnitModels() const;

	UPlayerUnitModel* GetPlayerUnitModel(int32 PlayerIndex);
	const UPlayerUnitModel* GetPlayerUnitModel(int32 PlayerIndex) const;

public:
	UAttributeSetComponentModel* GetAttributeComponentModel() const;
	UPartyArtifactComponentModel* GetPartyArtifactComponentModel() const;

public:
	int32 GetDifficulty() const;

public:
	FOnChangePartyPlayer OnChangePartyPlayer;

private:
	UPROPERTY(Category = Player, VisibleAnywhere, meta = (AllowPrivateAccess = "true", DisplayName = "PlayerUnitModels"))
	TArray<TObjectPtr<UPlayerUnitModel>> mPlayerUnitModels;

private:
	UPROPERTY(Category = Attribute, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "AttributeCompModel"))
	TObjectPtr<UAttributeSetComponentModel> mAttributeCompModel;

	UPROPERTY(Category = Artifact, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "PartyArtifactCompModel"))
	TObjectPtr<UPartyArtifactComponentModel> mPartyArtifactCompModel;

private:
	UPROPERTY(Category = AttributeSet, VisibleAnywhere, meta = (DisplayName = "PartyAttributeSet"))
	TObjectPtr<UPartyAttributeSet> mPartyAttributeSet;

protected:
	UPROPERTY(Category = Equipment, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "Difficulty"))
	int32 mDifficulty = 1;
};
