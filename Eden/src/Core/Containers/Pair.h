#pragma once

template <class T, class V>
class Pair
{
public:
	Pair()
	{

	}

	Pair(T key, V val)
	{
		Key = key;
		Value = val;
	}

	~Pair()
	{

	}
	T Key;
	V Value;
private:
};