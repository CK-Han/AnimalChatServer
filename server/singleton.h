#pragma once

template <typename T>
class TemplateSingleton
{
public:
	static T* GetInstance()
	{
		static T t;
		return &t;
	}

protected:
	TemplateSingleton() {}
	virtual ~TemplateSingleton() {}

	TemplateSingleton(const TemplateSingleton&) = delete;
	TemplateSingleton(TemplateSingleton&&) = delete;
	TemplateSingleton& operator=(const TemplateSingleton&) = delete;
};