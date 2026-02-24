#pragma once

#include <functional>
#include <type_traits>
#include <memory>
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
        std::vector<std::unique_ptr<T>> _pool;
        std::vector<T *> _available;
        std::function<T*()> _factory = nullptr;
    };

    template<class T, std::enable_if_t<std::is_base_of_v<IDisposable, T>, int> E0>
    ObjectPool<T, E0>::ObjectPool(std::function<T*()> createFactory, int capacity) : _factory(std::move(createFactory))
    {
        _pool.reserve(capacity);
        _available.reserve(capacity);
    }

    template<class T, std::enable_if_t<std::is_base_of_v<IDisposable, T>, int> E0>
    ObjectPool<T, E0>::~ObjectPool()
    {
        for (auto& p: _pool)
        {
            if (p && !p->getIsDisposed())
            {
                p->dispose();
            }
        }

        _pool.clear();
        _available.clear();
        _factory = nullptr;
    }

    template<class T, std::enable_if_t<std::is_base_of_v<IDisposable, T>, int> E0>
    T &ObjectPool<T, E0>::get()
    {
        if (_available.empty())
        {
            auto obj = std::unique_ptr<T>(_factory());
            T *ptr = obj.get();
            _pool.emplace_back(std::move(obj));
            return *ptr;
        }

        T *obj = _available.back();
        _available.pop_back();
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

        _available.emplace_back(&obj);
    }
} // BreadEngine
