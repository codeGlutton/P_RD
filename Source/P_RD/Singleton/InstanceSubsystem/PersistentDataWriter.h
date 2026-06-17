/*****************************************************************//**
 * @file   PersistentDataWriter.h
 * @brief  영구 데이터를 수정할 수 있도록 해주는 인터페이스 정의 헤더
 * @author 모호재
 * @date   2026-05-19
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "UObject/Interface.h"
#include "PersistentDataWriter.generated.h"

class UUserPersistData;
class URunPersistData;

UINTERFACE(MinimalAPI)
class UUserDataWriter : public UInterface
{
	GENERATED_BODY()
};

/**
 * @brief  영구 유저 데이터를 수정할 수 있도록 해주는 인터페이스
 */
class P_RD_API IUserDataWriter
{
	GENERATED_BODY()

protected:
	UUserPersistData* GetUserMutableData() const;
};

UINTERFACE(MinimalAPI)
class URunDataWriter : public UInterface
{
	GENERATED_BODY()
};

/**
 * @brief  영구 런 데이터를 수정할 수 있도록 해주는 인터페이스
 */
class P_RD_API IRunDataWriter
{
	GENERATED_BODY()

protected:
	URunPersistData* GetRunMutableData() const;
};

UINTERFACE(MinimalAPI)
class UOptionDataWriter : public UInterface
{
	GENERATED_BODY()
};

/**
 * @brief  영구 런 데이터를 수정할 수 있도록 해주는 인터페이스
 */
class P_RD_API IOptionDataWriter
{
	GENERATED_BODY()

protected:
	UOptionPersistData* GetOptionMutableData() const;
};