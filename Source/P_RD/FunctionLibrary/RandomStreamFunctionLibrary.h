/*****************************************************************//**
 * @file   RandomStreamFunctionLibrary.h
 * @brief  랜덤 스트림 연관 헬퍼 함수 라이브러리 헤더
 * @author 모호재
 * @date   2026-05-11
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

/**
 * @brief  랜덤 스트림 연관 헬퍼 함수 라이브러리
 */
class P_RD_API URandomStreamFunctionLibrary
{
public:
	static const FRandomStream& GetStageBuildStream(const UObject* WorldContextObject);
	static const FRandomStream& GetEventStream(const UObject* WorldContextObject);

public:
	template<typename T>
	static const T& GetRandomItem(const FRandomStream& Stream, const TArray<T>& Array)
	{
		checkf(Array.IsEmpty() == false, TEXT("Random Array is empty"));
		return Array[Stream.RandRange(0, Array.Num() - 1)];
	}

	template<typename T>
	static T& GetRandomItem(const FRandomStream& Stream, TArray<T>& Array)
	{
		checkf(Array.IsEmpty() == false, TEXT("Random Array is empty"));
		return Array[Stream.RandRange(0, Array.Num() - 1)];
	}

	template<typename T>
	static TArray<T> GetRandomUniqueItemsUsingCopiedArray(const FRandomStream& Stream, const TArray<T>& Array, int32 N)
	{
		checkf(Array.Num() < N, TEXT("Random Array is not enough to get random items"));

		TArray<T> Shuffled = Array;
		ShuffleArray(Stream, OUT Shuffled);

		TArray<T> Result;
		Result.Reserve(N);

		for (int32 i = 0; i < N; ++i)
		{
			Result.Add(Shuffled[i]);
		}

		return Result;
	}

	template<typename T>
	static TArray<T> GetRandomUniqueItemsUsingNonCopiedArray(const FRandomStream& Stream, const TArray<T>& Array, int32 N)
	{
		checkf(Array.Num() < N, TEXT("Random Array is not enough to get random items"));

		TArray<T> Candidates;

		TSet<int32> SelectedIndices;
		SelectedIndices.Reserve(N);
		while (SelectedIndices.Num() < N)
		{
			const int32 RandomIndex = Stream.RandRange(0, Array.Num() - 1);
			if (SelectedIndices.Contains(RandomIndex) == false)
			{
				SelectedIndices.Add(RandomIndex);
			}
		}

		TArray<T> Result;
		Result.Reserve(N);
		for (const int32 Index : SelectedIndices)
		{
			Result.Add(Candidates[Index]);
		}

		return Result;
	}

public:
	template<typename T>
	static void ShuffleArray(const FRandomStream& Stream, TArray<T>& Array)
	{
		const int32 Num = Array.Num();
		for (int32 i = 0; i < Num - 1; ++i)
		{
			const int32 IndexToSwap = Stream.RandRange(i, Num - 1);
			if (i != IndexToSwap)
			{
				Array.Swap(i, IndexToSwap);
			}
		}
	}

public:
	static int32 GetRandomFromInterval(const FRandomStream& Stream, const FInt32Interval& Interval);
	static float GetRandomFromInterval(const FRandomStream& Stream, const FFloatInterval& Interval);
};
