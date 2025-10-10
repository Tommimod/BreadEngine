#pragma once

#include <functional>
#include <type_traits>
#include "iDisposable.h"

namespace BreadEngine
{
    template<class T, std::enable_if_t<std::is_base_of_v<IDisposable, T>, int> = 0>
    class ObjectPool
    {
    public:
        explicit ObjectPool(std::function<T*()> createFactory = [] { return static_cast<T *>(nullptr); }, int capacity = 10);

        ~ObjectPool();

        T &get();

        void release(T &obj);

    private:
        std::vector<T *> pool;
        std::vector<T *> available;
        std::function<T*()> factory = nullptr;
    };

    template<class T, std::enable_if_t<std::is_base_of_v<IDisposable, T>, int> E0>
    ObjectPool<T, E0>::ObjectPool(std::function<T*()> createFactory, int capacity) : factory(std::move(createFactory))
    {
        pool.reserve(capacity);
        available.reserve(capacity);
    }

    template<class T, std::enable_if_t<std::is_base_of_v<IDisposable, T>, int> E0>
    ObjectPool<T, E0>::~ObjectPool()
    {
        for (T *p: pool)
        {
            if (p && !p->getIsDisposed())
            {
                p->dispose();
            }

            delete p;
        }

        pool.clear();
        available.clear();
        factory = nullptr;
    }

    template<class T, std::enable_if_t<std::is_base_of_v<IDisposable, T>, int> E0>
    T &ObjectPool<T, E0>::get()
    {
        if (available.empty())
        {
            T *obj = factory();
            pool.emplace_back(obj);
            return *obj;
        }

        T *obj = available.back();
        available.pop_back();
        obj->resetDisposed();
        return *obj;
    }

    template<class T, std::enable_if_t<std::is_base_of_v<IDisposable, T>, int> E0>
    void ObjectPool<T, E0>::release(T &obj)
    {
        if (!obj.getIsDisposed())
        {
            obj.dispose();
        }

        available.emplace_back(&obj);
    }
} // BreadEngine
