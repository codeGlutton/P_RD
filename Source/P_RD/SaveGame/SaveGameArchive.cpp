#include "SaveGame/SaveGameArchive.h"

FSaveGameArchive::FSaveGameArchive(FArchive& InInnerArchive) : FObjectAndNameAsStringProxyArchive(InInnerArchive, false)
{
    // SaveGame 지정자 속성만 직렬화/역직렬화
    ArIsSaveGame = true;
    // 기본값과 동일하더라도 저장
    ArNoDelta = true;
}