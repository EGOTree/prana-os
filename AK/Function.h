/*
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <AK/Assertions.h>
#include <AK/OwnPtr.h>
#include <AK/StdLibExtras.h>

namespace AK {

template<typename>
class Function;

template<typename Out, typename... In>
class Function<Out(In...)> {
public:
    Function() = default;

    template<typename CallableType, class = typename EnableIf<!(IsPointer<CallableType>::value && IsFunction<typename RemovePointer<CallableType>::Type>::value) && IsRvalueReference<CallableType&&>::value>::Type>
    Function(CallableType&& callable)
        : m_callable_wrapper(make<CallableWrapper<CallableType>>(move(callable)))
    {
    }

    template<typename FunctionType, class = typename EnableIf<IsPointer<FunctionType>::value && IsFunction<typename RemovePointer<FunctionType>::Type>::value>::Type>
    Function(FunctionType f)
        : m_callable_wrapper(make<CallableWrapper<FunctionType>>(move(f)))
    {
    }

    Out operator()(In... in) const
    {
        VERIFY(m_callable_wrapper);
        return m_callable_wrapper->call(forward<In>(in)...);
    }

    explicit operator bool() const { return !!m_callable_wrapper; }

    template<typename CallableType, class = typename EnableIf<!(IsPointer<CallableType>::value && IsFunction<typename RemovePointer<CallableType>::Type>::value) && IsRvalueReference<CallableType&&>::value>::Type>
    Function& operator=(CallableType&& callable)
    {
        m_callable_wrapper = make<CallableWrapper<CallableType>>(move(callable));
        return *this;
    }

    template<typename FunctionType, class = typename EnableIf<IsPointer<FunctionType>::value && IsFunction<typename RemovePointer<FunctionType>::Type>::value>::Type>
    Function& operator=(FunctionType f)
    {
        m_callable_wrapper = make<CallableWrapper<FunctionType>>(move(f));
        return *this;
    }

    Function& operator=(std::nullptr_t)
    {
        m_callable_wrapper = nullptr;
        return *this;
    }

private:
    class CallableWrapperBase {
    public:
        virtual ~CallableWrapperBase() = default;
        virtual Out call(In...) const = 0;
    };

    template<typename CallableType>
    class CallableWrapper final : public CallableWrapperBase {
    public:
        explicit CallableWrapper(CallableType&& callable)
            : m_callable(move(callable))
        {
        }

        CallableWrapper(const CallableWrapper&) = delete;
        CallableWrapper& operator=(const CallableWrapper&) = delete;

        Out call(In... in) const final override
        {
            if constexpr (requires { m_callable(forward<In>(in)...); }) {
                return m_callable(forward<In>(in)...);
            } else if constexpr (requires { m_callable(); }) {
                return m_callable();
            } else if constexpr (IsSame<void, Out>::value) {
                return;
            } else {
                return {};
            }
        }

    private:
        CallableType m_callable;
    };

    OwnPtr<CallableWrapperBase> m_callable_wrapper;
};

}

using AK::Function;
