#include "Simulation/Factory/ObjectModelFactory.h"
#include "Simulation/RoomContext.h"
#include "Simulation/RoomInstance.h"

#include "Actor/ActorModel.h"
#include "Actor/ActorView.h"

#include "Setting/GamePlaySettings.h"

DEFINE_LOG_CATEGORY(LogObjectModelFactory)

void UObjectModelFactory::SetContext(FRoomContext& RoomContext)
{
	mRoomContext = &RoomContext;
}

UObjectModel* UObjectModelFactory::NewModel_Internal(const UClass* Class, const FTransform& ViewTransform, FName Name, EObjectFlags Flags, UObject* Template, bool CopyTransientsFromClassDefaults, FObjectInstancingGraph* InInstanceGraph, UPackage* InExternalPackage)
{
	UObjectModel* Model = NewModelDeferred_Internal(Class, Name, Flags, Template, CopyTransientsFromClassDefaults, InInstanceGraph, InExternalPackage);
	FinishCreatingModel(Model, ViewTransform);

	return Model;
}

UObjectModel* UObjectModelFactory::NewModelDeferred_Internal(const UClass* Class, FName Name, EObjectFlags Flags, UObject* Template, bool CopyTransientsFromClassDefaults, FObjectInstancingGraph* InInstanceGraph, UPackage* InExternalPackage)
{
	UObjectModel* Model = NewObject<UObjectModel>(mRoomContext->mRoomInstance.Get(), Class, Name, Flags, Template, CopyTransientsFromClassDefaults, InInstanceGraph, InExternalPackage);
	Model->mModelId = mRoomContext->mRoomInstance->mModelMaxId++;
	mRoomContext->mRoomInstance->mAliveWorldModels.Add(Model->mModelId, Model);
	checkf(Model != nullptr, TEXT("새로운 모델 nullptr"));

	return Model;
}

void UObjectModelFactory::FinishCreatingModel(UObjectModel* Model, const FTransform& ViewTransform)
{
	Model->Initialize();

	UActorModel* ActorModel = Cast<UActorModel>(Model);
	if (ActorModel != nullptr)
	{
		ActorModel->BeginPlay();
	}
	OnPostCreateNewModel(Model, ViewTransform);
}

void UObjectModelFactory::DestroyModel(UObjectModel* Model)
{
	const int32 ModelId = Model->GetModelId();

	bool IsFound = mRoomContext->mRoomInstance->mAliveWorldModels.Contains(ModelId);
	checkf(IsFound == true, TEXT("제거될 모델 찾지 못함"));

	OnPreRemoveModel(Model);
	UActorModel* ActorModel = Cast<UActorModel>(Model);
	if (ActorModel != nullptr)
	{
		ActorModel->EndPlay();
	}
	Model->Uninitialize();

	mRoomContext->mRoomInstance->mAliveWorldModels.Remove(ModelId);
}

void UGameObjectModelFactory::RegisterSubsystemModels()
{
	const UGamePlaySettings* GamePlaySettings = GetDefault<UGamePlaySettings>();
	checkf(GamePlaySettings != nullptr, TEXT("게임 플레이 세팅 nullptr"));

	const TSet<FSubsystemModelViewMapping>& SubsystemModelViewMappings = GamePlaySettings->mSubsystemModelViewMappings;
	for (const FSubsystemModelViewMapping& SubsystemModelViewMapping : SubsystemModelViewMappings)
	{
		const TSubclassOf<UObjectModel>& ModelClass = SubsystemModelViewMapping.mModelClass.LoadSynchronous();
		UObjectModel* Model = NewObject<UObjectModel>(mRoomContext->mRoomInstance.Get(), ModelClass);
		mRoomContext->mRoomInstance->mAliveSubsystemModels.Add(ModelClass, Model);
		checkf(Model != nullptr, TEXT("새로운 서브시스템 모델 nullptr"));

		Model->Initialize();

		const TSoftClassPtr<UWorldSubsystem>& ViewClass = SubsystemModelViewMapping.mViewClass;
		if (ViewClass != nullptr)
		{
			TScriptInterface<IObjectView> View = GetWorld()->GetSubsystemBase(ViewClass.LoadSynchronous());
			if (View != nullptr)
			{
				View->BindModel(Model);
				Model->PostBindView(View);
			}
		}
	}
}

void UGameObjectModelFactory::UnregisterSubsystemModels()
{
	for (auto& AliveSubsystemModelPair : mRoomContext->mRoomInstance->mAliveSubsystemModels)
	{
		UObjectModel* Model = AliveSubsystemModelPair.Value;
		checkf(Model != nullptr, TEXT("기존 서브시스템 모델 nullptr"));

		Model->Uninitialize();

		IObjectView* View = Model->GetView();
		if (View != nullptr)
		{
			View->UnbindModel(Model);
		}
	}
}

void UGameObjectModelFactory::OnPostCreateNewModel(UObjectModel* Model, const FTransform& ViewTransform)
{
	const UGamePlaySettings* GamePlaySettings = GetDefault<UGamePlaySettings>();
	checkf(GamePlaySettings != nullptr, TEXT("게임 플레이 세팅 nullptr"));

	const FWorldModelViewMapping* WorldModelViewMappings = GamePlaySettings->mWorldModelViewMappings.Find(Model->GetClass());
	if (WorldModelViewMappings != nullptr)
	{
		const TSoftClassPtr<AActor>& ViewClass = WorldModelViewMappings->mViewClass;
		if (ViewClass != nullptr)
		{
			// 스폰 위치에 지형지물이 있어도 무시하고 스폰하기 위해서 AlwaysSpawn 파라미터 추가
			// -> 어차피 타일맵 위치로 이동할 것이므로 스폰 안 할 이유가 없음
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			TScriptInterface<IActorView> View = GetWorld()->SpawnActor(ViewClass.LoadSynchronous(), &ViewTransform, SpawnParameters);
			if (View != nullptr)
			{
				View->BindModel(Model);
				Model->PostBindView(View);
			}
		}
	}
}

void UGameObjectModelFactory::OnPreRemoveModel(UObjectModel* Model)
{
	IObjectView* View = Model->GetView();
	if (View != nullptr)
	{
		View->UnbindModel(Model);

		AActor* ViewActor = Cast<AActor>(View);
		if (ViewActor != nullptr)
		{
			ViewActor->Destroy();
		}
	}
}

void USimulationObjectModelFactory::OnPostCreateNewModel(UObjectModel* Model, const FTransform& ViewTransform)
{
	// 아무것도 안함
}

void USimulationObjectModelFactory::OnPreRemoveModel(UObjectModel* Model)
{
	// 아무것도 안함
}
