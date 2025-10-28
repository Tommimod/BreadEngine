#pragma once
#include <functional>
#include <optional>
#include <vector>

namespace BreadEngine
{
struct SubscriptionHandle
{
    int id = -1;
    [[nodiscard]] bool isValid() const
    {
        return id >= 0;
    }
};

template <typename... Args> class Action
{
  public:
    Action() = default;
    ~Action() = default;

    template <typename F> SubscriptionHandle subscribe(F &&func)
    {
        act.emplace_back(std::forward<F>(func));
        return {static_cast<int>(act.size() - 1)};
    }

    void unsubscribe(SubscriptionHandle handle)
    {
        if (!handle.isValid() || handle.id >= static_cast<int>(act.size()))
        {
            return;
        }
        std::swap(act[handle.id], act.back());
        act.pop_back();
    }

    void unsubscribeAll()
    {
        act.clear();
    }

    template <typename F> void operator+=(F &&func)
    {
        subscribe(std::forward<F>(func));
    }

    template <typename F> Action &operator=(F &&func)
    {
        act.clear();
        subscribe(std::forward<F>(func));
        return *this;
    }

    void invoke(Args... args)
    {
        auto copy = act;
        for (auto &f : copy)
        {
            if (f)
            {
                std::invoke(f, std::forward<Args>(args)...);
            }
        }
    }

  private:
    std::vector<std::function<void(Args...)>> act;
};

template <> class Action<>
{
  public:
    Action() = default;
    ~Action() = default;

    template <typename F> SubscriptionHandle subscribe(F &&func)
    {
        act.emplace_back(std::forward<F>(func));
        return {static_cast<int>(act.size() - 1)};
    }

    void unsubscribe(SubscriptionHandle handle)
    {
        if (!handle.isValid() || handle.id >= static_cast<int>(act.size()))
        {
            return;
        }
        std::swap(act[handle.id], act.back());
        act.pop_back();
    }

    template <typename F> void operator+=(F &&func)
    {
        subscribe(std::forward<F>(func));
    }

    template <typename F> Action<> &operator=(F &&func)
    {
        act.clear();
        subscribe(std::forward<F>(func));
        return *this;
    }

    void invoke() const
    {
        for (const auto copy = act; auto &f : copy)
        {
            if (f)
            {
                f();
            }
        }
    }

  private:
    std::vector<std::function<void()>> act;
};
} // namespace BreadEngine