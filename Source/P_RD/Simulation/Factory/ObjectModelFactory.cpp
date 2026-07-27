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
	if (mRoomContext == nullptr || IsValid(mRoomContext->mRoomInstance.Get()) == false)
	{
		UE_LOG(LogObjectModelFactory, Error, TEXT("모델 생성 실패: RoomContext 또는 RoomInstance가 유효하지 않음"));
		return nullptr;
	}

	if (IsValid(Class) == false || Class->IsChildOf(UObjectModel::StaticClass()) == false)
	{
		UE_LOG(LogObjectModelFactory, Error, TEXT("모델 생성 실패: 유효하지 않은 모델 클래스 %s"), *GetNameSafe(Class));
		return nullptr;
	}

	UObjectModel* Model = NewObject<UObjectModel>(mRoomContext->mRoomInstance.Get(), Class, Name, Flags, Template, CopyTransientsFromClassDefaults, InInstanceGraph, InExternalPackage);
	if (IsValid(Model) == false)
	{
		UE_LOG(LogObjectModelFactory, Error, TEXT("모델 생성 실패: NewObject가 nullptr 반환 (%s)"), *GetNameSafe(Class));
		return nullptr;
	}

	Model->mModelId = mRoomContext->mRoomInstance->mModelMaxId++;
	mRoomContext->mRoomInstance->mAliveWorldModels.Add(Model->mModelId, Model);

	return Model;
}

void UObjectModelFactory::FinishCreatingModel(UObjectModel* Model, const FTransform& ViewTransform)
{
	if (IsValid(Model) == false || mRoomContext == nullptr || IsValid(mRoomContext->mRoomInstance.Get()) == false)
	{
		return;
	}

	if (Model->GetOuter() != mRoomContext->mRoomInstance)
	{
		return;
	}

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
	if (IsValid(Model) == false || mRoomContext == nullptr || IsValid(mRoomContext->mRoomInstance.Get()) == false)
	{
		return;
	}

	if (Model->GetOuter() != mRoomContext->mRoomInstance)
	{
		return;
	}

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
	if (IsValid(GamePlaySettings) == false || mRoomContext == nullptr || IsValid(mRoomContext->mRoomInstance.Get()) == false)
	{
		UE_LOG(LogObjectModelFactory, Error, TEXT("서브시스템 모델 등록 실패: GamePlaySettings 또는 RoomContext가 유효하지 않음"));
		return;
	}

	const TSet<FSubsystemModelViewMapping>& SubsystemModelViewMappings = GamePlaySettings->mSubsystemModelViewMappings;
	for (const FSubsystemModelViewMapping& SubsystemModelViewMapping : SubsystemModelViewMappings)
	{
		const FSoftObjectPath ModelPath = SubsystemModelViewMapping.mModelClass.ToSoftObjectPath();
		UClass* ModelClass = SubsystemModelViewMapping.mModelClass.LoadSynchronous();
		if (IsValid(ModelClass) == false)
		{
			UE_LOG(LogObjectModelFactory, Error, TEXT("서브시스템 모델 클래스 로드 실패: %s"), *ModelPath.ToString());
			continue;
		}
		if (ModelClass->IsChildOf(UObjectModel::StaticClass()) == false)
		{
			UE_LOG(LogObjectModelFactory, Error, TEXT("서브시스템 모델 타입 불일치: %s (%s 파생 아님)"), *ModelPath.ToString(), *UObjectModel::StaticClass()->GetName());
			continue;
		}

		UObjectModel* Model = NewObject<UObjectModel>(mRoomContext->mRoomInstance.Get(), ModelClass);
		if (IsValid(Model) == false)
		{
			UE_LOG(LogObjectModelFactory, Error, TEXT("서브시스템 모델 생성 실패: %s"), *ModelPath.ToString());
			continue;
		}

		mRoomContext->mRoomInstance->mAliveSubsystemModels.Add(ModelClass, Model);
		Model->Initialize();

		const TSoftClassPtr<UWorldSubsystem>& ViewClass = SubsystemModelViewMapping.mViewClass;
		if (ViewClass.IsNull() == false)
		{
			const FSoftObjectPath ViewPath = ViewClass.ToSoftObjectPath();
			UClass* LoadedViewClass = ViewClass.LoadSynchronous();
			if (IsValid(LoadedViewClass) == false)
			{
				UE_LOG(LogObjectModelFactory, Error, TEXT("서브시스템 뷰 클래스 로드 실패: model=%s view=%s"), *ModelPath.ToString(), *ViewPath.ToString());
				continue;
			}

			UWorld* World = GetWorld();
			if (IsValid(World) == false)
			{
				UE_LOG(LogObjectModelFactory, Error, TEXT("서브시스템 뷰 바인딩 실패: World 없음, model=%s view=%s"), *ModelPath.ToString(), *ViewPath.ToString());
				continue;
			}

			TScriptInterface<IObjectView> View = World->GetSubsystemBase(LoadedViewClass);
			if (View != nullptr)
			{
				View->BindModel(Model);
				Model->PostBindView(View);
			}
			else
			{
				UE_LOG(LogObjectModelFactory, Warning, TEXT("서브시스템 뷰 인스턴스 조회 실패: model=%s view=%s"), *ModelPath.ToString(), *ViewPath.ToString());
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
