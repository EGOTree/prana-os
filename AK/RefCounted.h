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
#include <AK/Atomic.h>
#include <AK/Checked.h>
#include <AK/Noncopyable.h>
#include <AK/Platform.h>
#include <AK/StdLibExtras.h>

namespace AK {

template<class T>
constexpr auto call_will_be_destroyed_if_present(const T* object) -> decltype(const_cast<T*>(object)->will_be_destroyed(), TrueType {})
{
    const_cast<T*>(object)->will_be_destroyed();
    return {};
}

constexpr auto call_will_be_destroyed_if_present(...) -> FalseType
{
    return {};
}

template<class T>
constexpr auto call_one_ref_left_if_present(const T* object) -> decltype(const_cast<T*>(object)->one_ref_left(), TrueType {})
{
    const_cast<T*>(object)->one_ref_left();
    return {};
}

constexpr auto call_one_ref_left_if_present(...) -> FalseType
{
    return {};
}

class RefCountedBase {
    AK_MAKE_NONCOPYABLE(RefCountedBase);
    AK_MAKE_NONMOVABLE(RefCountedBase);

public:
    using RefCountType = unsigned int;
    using AllowOwnPtr = FalseType;

    ALWAYS_INLINE void ref() const
    {
        auto old_ref_count = m_ref_count.fetch_add(1, AK::MemoryOrder::memory_order_relaxed);
        VERIFY(old_ref_count > 0);
        VERIFY(!Checked<RefCountType>::addition_would_overflow(old_ref_count, 1));
    }

    [[nodiscard]] ALWAYS_INLINE bool try_ref() const
    {
        RefCountType expected = m_ref_count.load(AK::MemoryOrder::memory_order_relaxed);
        for (;;) {
            if (expected == 0)
                return false;
            VERIFY(!Checked<RefCountType>::addition_would_overflow(expected, 1));
            if (m_ref_count.compare_exchange_strong(expected, expected + 1, AK::MemoryOrder::memory_order_acquire))
                return true;
        }
    }

    ALWAYS_INLINE RefCountType ref_count() const
    {
        return m_ref_count.load(AK::MemoryOrder::memory_order_relaxed);
    }

protected:
    RefCountedBase() = default;
    ALWAYS_INLINE ~RefCountedBase()
    {
        VERIFY(m_ref_count.load(AK::MemoryOrder::memory_order_relaxed) == 0);
    }

    ALWAYS_INLINE RefCountType deref_base() const
    {
        auto old_ref_count = m_ref_count.fetch_sub(1, AK::MemoryOrder::memory_order_acq_rel);
        VERIFY(old_ref_count > 0);
        return old_ref_count - 1;
    }

    mutable Atomic<RefCountType> m_ref_count { 1 };
};

template<typename T>
class RefCounted : public RefCountedBase {
public:
    bool unref() const
    {
        auto new_ref_count = deref_base();
        if (new_ref_count == 0) {
            call_will_be_destroyed_if_present(static_cast<const T*>(this));
            delete static_cast<const T*>(this);
            return true;
        } else if (new_ref_count == 1) {
            call_one_ref_left_if_present(static_cast<const T*>(this));
        }
        return false;
    }
};

}

using AK::RefCounted;
