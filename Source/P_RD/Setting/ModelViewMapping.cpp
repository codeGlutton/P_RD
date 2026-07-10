#include "Setting/ModelViewMapping.h"

FSubsystemModelViewMapping::FSubsystemModelViewMapping(UClass* ModelClass) : mModelClass(ModelClass)
{

}

FSubsystemModelViewMapping::FSubsystemModelViewMapping(TSoftClassPtr<UObjectModel> ModelClass) : mModelClass(ModelClass)
{
}

FWorldModelViewMapping::FWorldModelViewMapping(UClass* ModelClass) : mModelClass(ModelClass)
{
}

FWorldModelViewMapping::FWorldModelViewMapping(TSoftClassPtr<UObjectModel> ModelClass) : mModelClass(ModelClass)
{
}