#pragma once
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

template<typename T>
class dynamicArray final
{
public:
	T*			data;
	uint32_t	_allocated_size;
	uint32_t	_size;

public:
	dynamicArray() : _size(0), _allocated_size(0), data(nullptr){};
	~dynamicArray()
	{
		if(data) free(data);
	}

	void init_array(const uint32_t init_size = 10)
	{
		_size = 0;
		data = (T*)malloc(sizeof(T) * init_size);
		_allocated_size = init_size;
	}
	void dispose_array()
	{
		free(data);
		data = NULL;
	}
	inline uint32_t get_size()
	{
		return _size;
	}

	uint32_t get_allocated_size()
	{
		return _allocated_size;
	}

	void push_back(T& obj)
	{
		if (_allocated_size == _size)
		{
			_allocated_size *= 2;
			T* temp = data;
			data = (T*)realloc(data, sizeof(T) *(size_t)_allocated_size);
			assert(data);
			if(!data)
			{
				data = temp;
			}
		}
		data[_size++] = obj;
	}

	void delete_back()
	{
		assert(_size > 0);
		_size--;
	}
	T* get_back()
	{
		if (_size > 0)
		{
			return &data[_size - 1];
		}
		return nullptr;
	}
	void clear_array()
	{
		_size = 0;
	}
	void resize_array(const uint32_t new_size)
	{
		if (new_size > _allocated_size)
		{
			_allocated_size = new_size;
			realloc(data, _sizeof(T) *(size_t)_allocated_size);
		}
	}
	T* get_new_item()
	{
		if (_allocated_size == _size)
		{
			_allocated_size *= 2;
			T* temp = data;
			data = (T*)realloc(data, sizeof(T) *(size_t)_allocated_size);
			assert(data);
			if (!data)
			{
				data = temp;
			}
		}
		return &data[_size++];
	}
	void fast_remove(const uint32_t index)
	{
		assert(index < _size);
		if(index == _size - 1) 
		{
			_size--;
		}
		else
		{
			data[index] = data[--_size];
		}

	}
};

//todo
//[] operaattori?
//copy construcrori?
//itaraattori?
//quicksort?
//resize v�hent�� kokoa?
//pidet��nk� push back reffin�?

// test get_new item


template<typename T, const uint32_t poolSize = 50>
class pool final
{
public:
	uint32_t			_currentArraySize;
	dynamicArray<T*>	_data;
	dynamicArray<T*>	_freelist;
public:
	pool() :_currentArraySize(0) {}
	~pool()
	{
        if(_data.data)
        {      
            for (uint32_t i = 0; i < _data.get_size(); i++)
            {
                if (_data.data[i])free(_data.data[i]);
            }
        }
	}
	void init_pool()
	{
		_currentArraySize = 0;
		T* newdata = (T*)malloc(sizeof(T)* poolSize);
        _data.init_array();
        _freelist.init_array();
		_data.push_back(newdata);
	}
	void dispose_pool()
	{
		for (uint32_t i = 0; i < _data.get_size(); i++)
		{
			free(_data.data[i]);
		}
		_data.dispose_array();
		_freelist.dispose_array();
	}
	T* new_item()
	{
		if (_freelist.get_size())
		{
			T** freeObj = _freelist.get_back();
			_freelist.delete_back();
			return *freeObj;
		}

		if (_currentArraySize >= poolSize)
		{
			_currentArraySize = 0;
			T* newdata = (T*)calloc(0,sizeof(T)* poolSize);
			_data.push_back(newdata);
		}
		T** currentArray = _data.get_back();
		return &((*currentArray)[_currentArraySize++]);
	}

	void delete_obj(T *obj)
	{
		_freelist.push_back(obj);
	}

	uint32_t get_pool_size()
	{
		return (_data.get_size() -1)* poolSize + _currentArraySize;
	}

	uint32_t get_allocated_size()
	{
		return _data.get_size()* poolSize;
	}
};
