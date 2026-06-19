#include "Simulation/Factory/ObjectModelFactory.h"
#include "Simulation/RoomContext.h"
#include "Simulation/RoomInstance.h"

#include "ObjectModel.h"
#include "ObjectView.h"

#include "Setting/GamePlaySettings.h"

DEFINE_LOG_CATEGORY(LogObjectModelFactory)

void UObjectModelFactory::SetContext(FRoomContext& RoomContext)
{
	mRoomContext = &RoomContext;
}

UObjectModel* UObjectModelFactory::NewModel_Internal(const UClass* Class, FName Name, EObjectFlags Flags, UObject* Template, bool CopyTransientsFromClassDefaults, FObjectInstancingGraph* InInstanceGraph, UPackage* InExternalPackage)
{
	UObjectModel* Model = NewObject<UObjectModel>(mRoomContext->mRoomInstance.Get(), Class, Name, Flags, Template, CopyTransientsFromClassDefaults, InInstanceGraph, InExternalPackage);
	mRoomContext->mRoomInstance->mAliveModels.Add(Model);
	checkf(Model != nullptr, TEXT("새로운 모델 nullptr"));
	
	Model->Initialize();
	OnPostCreateNewModel(Model);
	Model->BeginPlay();

	return Model;
}

void UObjectModelFactory::DestroyModel(UObjectModel* Model)
{
	const int32 TargetModelIndex = mRoomContext->mRoomInstance->mAliveModels.IndexOfByPredicate([Model](const TObjectPtr<UObjectModel>& Entry) {
		return Entry == Model;
		});
	
	checkf(TargetModelIndex != INDEX_NONE, TEXT("제거될 모델 찾지 못함"));

	Model->EndPlay();
	OnPreRemoveModel(Model);
	Model->Uninitialize();

	mRoomContext->mRoomInstance->mAliveModels.RemoveAtSwap(TargetModelIndex);
}

void UGameObjectModelFactory::OnPostCreateNewModel(UObjectModel* Model)
{
	const UGamePlaySettings* GamePlaySettings = GetDefault<UGamePlaySettings>();
	checkf(GamePlaySettings != nullptr, TEXT("게임 플레이 세팅 nullptr"));

	const FModelViewMapping* ModelViewMapping = GamePlaySettings->mModelViewMappings.Find(Model->GetClass());
	if (ModelViewMapping != nullptr)
	{
		const TSoftClassPtr<AActor>& ViewClass = ModelViewMapping->mViewClass;
		checkf(ViewClass != nullptr, TEXT("새로운 뷰 클래스 nullptr"));

		TScriptInterface<IObjectView> View = GetWorld()->SpawnActor(ViewClass.LoadSynchronous());
		checkf(View != nullptr, TEXT("새로운 뷰 객체 nullptr"));

		View->BindModel(Model);
		Model->PostBindView(View);
	}
}

void UGameObjectModelFactory::OnPreRemoveModel(UObjectModel* Model)
{
	IObjectView* View = Model->GetView();
	if (View != nullptr)
	{
		View->UnbindModel(Model);
	}
}

void USimulationObjectModelFactory::OnPostCreateNewModel(UObjectModel* Model)
{
	// 아무것도 안함
}

void USimulationObjectModelFactory::OnPreRemoveModel(UObjectModel* Model)
{
	// 아무것도 안함
}
