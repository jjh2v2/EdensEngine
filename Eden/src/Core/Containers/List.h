#pragma once

template <class T>
class ListNode
{
public:
	ListNode()
	{
		Prev = nullptr;
		Next = nullptr;
	}
	ListNode(T val)
	{
		Prev = nullptr;
		Next = nullptr;
		Value = val;
	}
	ListNode(ListNode<T> *prev, ListNode<T> *next, T val)
	{
		Prev = prev;
		Next = next;
		Value = val;
	}
	~ListNode()
	{
		Prev = nullptr;
		Next = nullptr;
	}

	ListNode<T> *Prev;
	ListNode<T> *Next;
	T Value;
};

template <class T>
class List
{
public:
	List()
	{
		mCount = 0;
		mHead = nullptr;
		mTail = nullptr;
	}
	~List()
	{
		Clear();
	}

	ListNode<T> *Begin()
	{
		return mHead;
	}

	ListNode<T> *End()
	{
		return nullptr;
	}

	int Count()
	{
		return mCount;
	}

	T& ElementAt(int index)
	{
		ListNode<T> *temp = mHead;
		for(int i = 0; i < index; i++)
		{
			temp = temp->Next;
		}
		return temp->Value;
	}

	void AddToEnd(T val)
	{
		if(!mHead)
		{
			mHead = new ListNode<T>(nullptr, mTail, val);
		}
		else if(!mTail)
		{
			mTail = new ListNode<T>(mHead, nullptr, val);
			mHead->Next = mTail;
		}
		else
		{
			mTail->Next = new ListNode<T>(mTail, nullptr, val);
			mTail = mTail->Next;
		}

		mCount++;
	}

	void AddToFront(T val)
	{
		if(!mHead)
		{
			mHead = new ListNode<T>(nullptr, mTail, val);
		}
		else if(!mTail)
		{
			mHead = new ListNode<T>(nullptr, mHead, val);
			mTail = mHead->Next;
		}
		else
		{
			mHead = new ListNode<T>(nullptr, mHead, val);
			mHead->Next->Prev = mHead;
		}

		mCount++;
	}

	void AddAt(int index, T val)
	{
		if(index > mCount)
		{
			return;
		}
		if(index == 0)
		{
			AddToFront(val);
			return;
		}
		if(index == mCount)
		{
			AddToEnd(val);
			return;
		}
		
		ListNode<T> *temp = mHead;
		for(int i = 0; i < index; i++)
		{
			temp = temp->Next;
		}

		ListNode<T> *newNode = new ListNode<T>(temp->Prev, temp, val);
		temp->Prev->Next = newNode;
		temp->Prev = newNode;
		mCount++;
	}

	void Clear()
	{
		if(mHead == nullptr && mTail == nullptr){return;}

		ListNode<T> *temp = mHead;
		while(temp->Next != nullptr)
		{
			temp = temp->Next;
			delete temp->Prev;
		}
		delete temp;

		mCount = 0;
		mHead = nullptr;
		mTail = nullptr;
	}

	void Remove(int index)
	{
		if(mCount == 0){return;}
		if(index >= mCount){return;}

		if(index == 0)
		{
			ListNode<T> *temp = mHead;
			if(mHead->Next == nullptr)
			{
				delete mHead;
				mHead = nullptr;
			}
			else
			{
				temp = mHead->Next;
				delete mHead;
				temp->Prev = nullptr;
				mHead = temp;
			}
			mCount--;

			if(mCount == 1)
			{
				mTail = nullptr;
			}
			return;
		}

		if(index == mCount - 1)
		{
			ListNode<T> *temp = mTail->Prev;
			delete mTail;
			temp->Next = nullptr;
			mTail = temp;
			mCount--;
			return;
		}

		ListNode<T> *temp = mHead;
		for(int i = 0; i < index; i++)
		{
			temp = temp->Next;
		}

		temp->Prev->Next = temp->Next;
		temp->Next->Prev = temp->Prev;
		delete temp;
		mCount--;
	}

private:
	ListNode<T> *mHead;
	ListNode<T> *mTail;
	int mCount;
};