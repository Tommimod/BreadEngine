#pragma once

namespace BreadEngine
{
    class IDisposable
    {
    public:
        [[nodiscard]] bool getIsDisposed() const { return _isDisposed; };
        void resetDisposed() { _isDisposed = false; };

        virtual void dispose() = 0;

    protected:
        ~IDisposable() = default;

        bool _isDisposed = false;
    };
} // BreadEngine
