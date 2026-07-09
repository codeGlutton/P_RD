/*****************************************************************//**
 * @file   CircularList.h
 * @brief  순환 리스트 구현 헤더
 * @details
 * 언리얼의 List.h를 기반으로 순환 리스트 자료구조 제작
 * 동일한 API를 제공하고 싶어, 예외적으로 언리얼 코드 규칙을 따름
 * @author 모호재
 * @date   2026-04-30
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

/**
 * @brief 순환 더블 링크드 리스트의 반복자
 * @tparam NodeType 리스트 자료구조의 노드 타입
 * @tparam ElementType 항목 데이터의 타입
 */
template<class NodeType, class ElementType>
class TCircularDoubleLinkedListIterator
{
public:
	[[nodiscard]] explicit TCircularDoubleLinkedListIterator(NodeType* StartingNode)
		: CurrentNode(StartingNode)
	{
	}

	[[nodiscard]] UE_FORCEINLINE_HINT explicit operator bool() const
	{
		return CurrentNode != nullptr;
	}

	TCircularDoubleLinkedListIterator& operator++()
	{
		checkSlow(CurrentNode);
		CurrentNode = CurrentNode->GetNextNode();
		return *this;
	}

	TCircularDoubleLinkedListIterator operator++(int)
	{
		auto Tmp = *this;
		++(*this);
		return Tmp;
	}

	TCircularDoubleLinkedListIterator& operator--()
	{
		checkSlow(CurrentNode);
		CurrentNode = CurrentNode->GetPrevNode();
		return *this;
	}

	TCircularDoubleLinkedListIterator operator--(int)
	{
		auto Tmp = *this;
		--(*this);
		return Tmp;
	}

	[[nodiscard]] ElementType& operator->() const
	{
		checkSlow(CurrentNode);
		return CurrentNode->GetValue();
	}

	[[nodiscard]] ElementType& operator*() const
	{
		checkSlow(CurrentNode);
		return CurrentNode->GetValue();
	}

	[[nodiscard]] NodeType* GetNode() const
	{
		checkSlow(CurrentNode);
		return CurrentNode;
	}

	[[nodiscard]] bool operator==(const TCircularDoubleLinkedListIterator& Rhs) const { return CurrentNode == Rhs.CurrentNode; }
	[[nodiscard]] bool operator!=(const TCircularDoubleLinkedListIterator& Rhs) const { return CurrentNode != Rhs.CurrentNode; }

private:
	NodeType* CurrentNode;
};

/**
 * @brief 순환 더블 링크드 리스트
 * @tparam ElementType 항목 데이터의 타입
 */
template<typename ElementType>
class TCircularDoubleLinkedList
{
public:
	class TCircularDoubleLinkedListNode
	{
	public:
		friend class TCircularDoubleLinkedList;

		[[nodiscard]] TCircularDoubleLinkedListNode(const ElementType& InValue)
			: Value(InValue), NextNode(this), PrevNode(this)
		{
		}

		[[nodiscard]] const ElementType& GetValue() const
		{
			return Value;
		}

		[[nodiscard]] ElementType& GetValue()
		{
			return Value;
		}

		[[nodiscard]] TCircularDoubleLinkedListNode* GetNextNode()
		{
			return NextNode;
		}

		[[nodiscard]] const TCircularDoubleLinkedListNode* GetNextNode() const
		{
			return NextNode;
		}

		[[nodiscard]] TCircularDoubleLinkedListNode* GetPrevNode()
		{
			return PrevNode;
		}

		[[nodiscard]] const TCircularDoubleLinkedListNode* GetPrevNode() const
		{
			return PrevNode;
		}

	protected:
		ElementType					Value;
		TCircularDoubleLinkedListNode*	NextNode;
		TCircularDoubleLinkedListNode*	PrevNode;
	};

	typedef TCircularDoubleLinkedListIterator<TCircularDoubleLinkedListNode, ElementType> TIterator;
	typedef TCircularDoubleLinkedListIterator<TCircularDoubleLinkedListNode, const ElementType> TConstIterator;

	[[nodiscard]] TCircularDoubleLinkedList()
		: HeadNode(nullptr)
		, ListSize(0)
	{
	}

	virtual ~TCircularDoubleLinkedList()
	{
		Empty();
	}

	TCircularDoubleLinkedList(const TCircularDoubleLinkedList&) = delete;
	TCircularDoubleLinkedList& operator=(const TCircularDoubleLinkedList&) = delete;

	[[nodiscard]] TCircularDoubleLinkedList(TCircularDoubleLinkedList&& Other)
		: TCircularDoubleLinkedList()
	{
		Swap(HeadNode, Other.HeadNode);
		Swap(ListSize, Other.ListSize);
	}

	TCircularDoubleLinkedList& operator=(TCircularDoubleLinkedList&& Other)
	{
		Swap(HeadNode, Other.HeadNode);
		Swap(ListSize, Other.ListSize);
		return *this;
	}

	bool AddHead(const ElementType& InElement)
	{
		return AddHead(new TCircularDoubleLinkedListNode(InElement));
	}

	bool AddHead(TCircularDoubleLinkedListNode* NewNode)
	{
		if (NewNode == nullptr)
		{
			return false;
		}

		if (HeadNode != nullptr)
		{
			TCircularDoubleLinkedListNode* TailNode = HeadNode->PrevNode;

			NewNode->NextNode = HeadNode;
			NewNode->PrevNode = TailNode;
			TailNode->NextNode = NewNode;
			HeadNode->PrevNode = NewNode;

			HeadNode = NewNode;
		}
		else
		{
			HeadNode = NewNode;
		}

		SetListSize(ListSize + 1);
		return true;
	}

	bool AddTail(const ElementType& InElement)
	{
		return AddTail(new TCircularDoubleLinkedListNode(InElement));
	}

	bool AddTail(TCircularDoubleLinkedListNode* NewNode)
	{
		if (NewNode == nullptr)
		{
			return false;
		}

		if (HeadNode != nullptr)
		{
			TCircularDoubleLinkedListNode* TailNode = HeadNode->PrevNode;

			TailNode->NextNode = NewNode;
			HeadNode->PrevNode = NewNode;
			NewNode->NextNode = HeadNode;
			NewNode->PrevNode = TailNode;
		}
		else
		{
			HeadNode = NewNode;
		}

		SetListSize(ListSize + 1);
		return true;
	}

	bool InsertNode(const ElementType& InElement, TCircularDoubleLinkedListNode* NodeToInsertBefore = nullptr)
	{
		return InsertNode(new TCircularDoubleLinkedListNode(InElement), NodeToInsertBefore);
	}

	bool InsertNode(TCircularDoubleLinkedListNode* NewNode, TCircularDoubleLinkedListNode* NodeToInsertBefore = nullptr)
	{
		if (NewNode == nullptr)
		{
			return false;
		}

		if (NodeToInsertBefore == nullptr)
		{
			return AddHead(NewNode);
		}

		NewNode->PrevNode = NodeToInsertBefore->PrevNode;
		NewNode->NextNode = NodeToInsertBefore;

		NodeToInsertBefore->PrevNode->NextNode = NewNode;
		NodeToInsertBefore->PrevNode = NewNode;

		SetListSize(ListSize + 1);
		return true;
	}

	TCircularDoubleLinkedListNode* RemoveNode(const ElementType& InElement)
	{
		TCircularDoubleLinkedListNode* ExistingNode = FindNode(InElement);
		return RemoveNode(ExistingNode);
	}

	TCircularDoubleLinkedListNode* RemoveNode(TCircularDoubleLinkedListNode* NodeToRemove, bool bDeleteNode = true)
	{
		if (NodeToRemove != nullptr)
		{
			if (Num() == 1)
			{
				checkSlow(NodeToRemove == HeadNode);
				if (bDeleteNode)
				{
					Empty();
				}
				else
				{
					NodeToRemove->NextNode = NodeToRemove->PrevNode = NodeToRemove;
					HeadNode = nullptr;
					SetListSize(0);
				}
				return nullptr;
			}

			TCircularDoubleLinkedListNode* NextNode = NodeToRemove->NextNode;
			TCircularDoubleLinkedListNode* PreNode = NodeToRemove->PrevNode;
			if (NodeToRemove == HeadNode)
			{
				HeadNode = NextNode;
			}
			NodeToRemove->NextNode->PrevNode = PreNode;
			NodeToRemove->PrevNode->NextNode = NextNode;

			if (bDeleteNode)
			{
				delete NodeToRemove;
			}
			else
			{
				NodeToRemove->NextNode = NodeToRemove->PrevNode = NodeToRemove;
			}
			SetListSize(ListSize - 1);

			return NextNode;
		}

		return nullptr;
	}

	void Empty()
	{
		TCircularDoubleLinkedListNode* Node;
		for (int32 i = 0; i < ListSize; ++i)
		{
			Node = HeadNode->NextNode;
			delete HeadNode;
			HeadNode = Node;
		}

		HeadNode = nullptr;
		SetListSize(0);
	}

	[[nodiscard]] TCircularDoubleLinkedListNode* GetHead() const
	{
		return HeadNode;
	}

	[[nodiscard]] TCircularDoubleLinkedListNode* GetTail() const
	{
		if (HeadNode != nullptr)
		{
			return HeadNode->PrevNode;
		}
		return nullptr;
	}

	[[nodiscard]] TCircularDoubleLinkedListNode* FindNode(const ElementType& InElement)
	{
		TCircularDoubleLinkedListNode* Node = HeadNode;
		for (int32 i = 0; i < ListSize; ++i)
		{
			if (Node->GetValue() == InElement)
			{
				return Node;
			}

			Node = Node->NextNode;
		}

		return nullptr;
	}

	[[nodiscard]] bool Contains(const ElementType& InElement)
	{
		return (FindNode(InElement) != nullptr);
	}

	[[nodiscard]] bool IsEmpty() const
	{
		return ListSize == 0;
	}

	[[nodiscard]] int32 Num() const
	{
		return ListSize;
	}

protected:
	virtual void SetListSize(int32 NewListSize)
	{
		ListSize = NewListSize;
	}

private:
	TCircularDoubleLinkedListNode* HeadNode;
	int32 ListSize;
};

