#pragma once
#include <time.h>
#include <stdlib.h>
#include <iostream>
using std::cout;
using std::endl;

// 定长内存池
//template<size_t N>
//class ObjectPool
//{
//
//};

#ifdef _WIN32
	#include <Windows.h>
#else 
// 
#endif // _WIN32


// 这样我申请内存的时候跟malloc就没有关系了，直接去找堆了
// 直接去堆上按页申请空间
inline static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, kpage<<13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else 
	// linux 下brk mmap等
#endif // _WIN32
	if (ptr == nullptr)
	{
		throw std::bad_alloc();
	}
	return ptr;
}


template<class T>
class ObjectPool
{
private:
	char* _memory = nullptr; // 指向大块内存的指针
	void* _freeList = nullptr; // 还回来过程中组成的自由链表的头指针
	size_t _remainBytes = 0; // 大块内存在切分过程中剩余字节数
public:
	T* New()
	{
		T* obj = nullptr;
		// 优先把还回来的内存块对象再次重复利用
		if (_freeList)
		{
			void* next = *(void**)_freeList;
			obj = (T*)_freeList;
			_freeList = next;
		}
		else
		{
			// 剩余内存不够一个对象大小时，则重新开大块空间
			if (_remainBytes < sizeof(T))
			{
				_remainBytes = 128 * 1024;
				// 这个相当于是除8kb，可以换算出页数，1kb是2的10次方，因此2的13次方就是8kb
				_memory = (char*)SystemAlloc(_remainBytes >> 13);
				if (_memory == nullptr)
				{
					throw std::bad_alloc();
				}
			}
			obj = (T*)_memory;
			// 这里是为了防止，T这个对象是小于指针的内存的。如果小于指针的内存的话
			// 那么最后可能指针都凑不齐了
			size_t objSize = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);
			_memory += objSize;
			_remainBytes -= objSize;
		}
		// 定位new，显示调用T的构造函数初始化
		new(obj)T;
		return obj;
	}

	void Delete(T* obj)
	{
		// 显示调用析构函数清理对象
		obj->~T();
		// 头插
		*(void**)obj = _freeList;
		_freeList = obj;
	}
};
