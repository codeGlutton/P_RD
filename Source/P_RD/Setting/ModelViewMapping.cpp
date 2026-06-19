#include "Setting/ModelViewMapping.h"

FModelViewMapping::FModelViewMapping(UClass* ModelClass) : mModelClass(ModelClass)
{

}

FModelViewMapping::FModelViewMapping(TSoftClassPtr<UObjectModel> ModelClass) : mModelClass(ModelClass)
{
}
