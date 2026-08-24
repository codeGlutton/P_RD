#include "DataAsset/ObstacleSpawnData/StaticObstacleSpawnData.h"
#include "Setting/GamePlaySettings.h"
#include "Actor/BoardActor/BoardActorModel.h"

#include "AttributeSet/AttributeSetMinimal.h"

#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

void UStaticObstacleSpawnData::PostLoad()
{
	Super::PostLoad();

	mViewClass = nullptr;
	if (mModelClass.IsNull() == false)
	{
		const FWorldModelViewMapping* Mapping = GetDefault<UGamePlaySettings>()->mWorldModelViewMappings.Find(FWorldModelViewMapping(mModelClass));
		if (Mapping != nullptr)
		{
			if (mViewClass.ToSoftObjectPath() != Mapping->mViewClass.ToSoftObjectPath())
			{
#if WITH_EDITOR
				Modify();
#endif
				mViewClass = Mapping->mViewClass;
#if WITH_EDITOR
				MarkPackageDirty();
#endif
			}
		}
	}
}

#if WITH_EDITOR
void UStaticObstacleSpawnData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UStaticObstacleSpawnData, mModelClass))
	{
		mViewClass = nullptr;
		if (mModelClass.IsNull() == false)
		{
			const FWorldModelViewMapping* Mapping = GetDefault<UGamePlaySettings>()->mWorldModelViewMappings.Find(FWorldModelViewMapping(mModelClass));
			if (Mapping != nullptr)
			{
				mViewClass = Mapping->mViewClass;
			}
		}
	}
}

EDataValidationResult UStaticObstacleSpawnData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult SuperResult = Super::IsDataValid(Context);
	EDataValidationResult ThisResult = EDataValidationResult::Valid;

	if (mModelClass == nullptr || mViewClass == nullptr || mDisplayName.IsEmpty() == true)
	{
		Context.AddError(FText::FromString(TEXT("모델 클래스 or 액터 클래스 or 이름 미지정")));
		ThisResult = EDataValidationResult::Invalid;
	}

	return CombineDataValidationResults(SuperResult, ThisResult);
}
#endif

FName UStaticObstacleSpawnData::GetKeyName() const
{
	// 속성 그룹 키는 데이터 키다. 번역된 표시 문자열(ToString)을 쓰면 언어에
	// 따라 키가 바뀌어 커브 테이블(영문 행)과 어긋난다 — ko 번역이 채워진 뒤
	// 한국어 모드에서 스탯 초기화가 전멸해 전투 진입이 죽었다(0823).
	// 원문(소스) 문자열로 고정해 어떤 언어에서도 같은 키를 쓴다.
	const FString* SourceString = FTextInspector::GetSourceString(mDisplayName);
	FString Key = SourceString != nullptr ? *SourceString : mDisplayName.ToString();
	Key.RemoveSpacesInline();
	return *Key;
}

float UStaticObstacleSpawnData::GetDefaultAttributeValue(UWorld* World, TSubclassOf<UTacticalAttributeSet> AttributeSetClass, const FTacticalAttribute& Attribute, int32 Level) const
{
	UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(World);
	checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));

	return TacticalFrameworkModel->GetAttributeSetInitter()->GetAttributeSetValue(
		AttributeSetClass,
		Attribute.GetUProperty(),
		GetKeyName(),
		Level
	);
}
