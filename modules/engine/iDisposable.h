#pragma once

namespace BreadEngine
{
    class IDisposable
    {
    public:
        [[nodiscard]] bool getIsDisposed() const { return isDisposed; };
        void resetDisposed() { isDisposed = false; };

        virtual void dispose() = 0;

    protected:
        ~IDisposable() = default;

        bool isDisposed = false;
    };
} // BreadEngine
