#include "Simulation/Factory/ModelFactory.h"

DEFINE_LOG_CATEGORY(LogModelFactory)

void IModelFactory::SetModelDefaultOuter(UObject* DefaultOuter)
{
	mModelDefaultOuter = DefaultOuter;
}

IObjectModel* UGameModelFactory::NewModel_Internal(const UClass* Class, FName Name, EObjectFlags Flags, UObject* Template, bool CopyTransientsFromClassDefaults, FObjectInstancingGraph* InInstanceGraph, UPackage* InExternalPackage)
{
	IObjectModel* Model = NewObject<IObjectModel>(mModelDefaultOuter.Get(), Class, Name, Flags, Template, CopyTransientsFromClassDefaults, InInstanceGraph, InExternalPackage);
	

	
	return Model;
}

IObjectModel* UGameModelFactory::NewModel_Internal()
{
	IObjectModel* Model = NewObject<IObjectModel>(mModelDefaultOuter.Get());
	

	
	return Model;
}

IObjectModel* UGameModelFactory::NewModel_Internal(FName Name, EObjectFlags Flags, UObject* Template, bool CopyTransientsFromClassDefaults, FObjectInstancingGraph* InInstanceGraph, UPackage* InExternalPackage)
{
	IObjectModel* Model = NewObject<IObjectModel>(mModelDefaultOuter.Get(), Name, Flags, Template, CopyTransientsFromClassDefaults, InInstanceGraph, InExternalPackage);
	

	
	return Model;
}

IObjectModel* USimulationModelFactory::NewModel_Internal(const UClass* Class, FName Name, EObjectFlags Flags, UObject* Template, bool CopyTransientsFromClassDefaults, FObjectInstancingGraph* InInstanceGraph, UPackage* InExternalPackage)
{
	IObjectModel* Model = NewObject<IObjectModel>(mModelDefaultOuter.Get(), Class, Name, Flags, Template, CopyTransientsFromClassDefaults, InInstanceGraph, InExternalPackage);



	return Model;
}

IObjectModel* USimulationModelFactory::NewModel_Internal()
{
	IObjectModel* Model = NewObject<IObjectModel>(mModelDefaultOuter.Get());



	return Model;
}

IObjectModel* USimulationModelFactory::NewModel_Internal(FName Name, EObjectFlags Flags, UObject* Template, bool CopyTransientsFromClassDefaults, FObjectInstancingGraph* InInstanceGraph, UPackage* InExternalPackage)
{
	IObjectModel* Model = NewObject<IObjectModel>(mModelDefaultOuter.Get(), Class, Name, Flags, Template, CopyTransientsFromClassDefaults, InInstanceGraph, InExternalPackage);



	return Model;
}
