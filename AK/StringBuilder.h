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

#include <AK/ByteBuffer.h>
#include <AK/Format.h>
#include <AK/Forward.h>
#include <AK/StringView.h>
#include <stdarg.h>

namespace AK {

class StringBuilder {
public:
    using OutputType = String;

    explicit StringBuilder(size_t initial_capacity = inline_capacity);
    ~StringBuilder() = default;

    void append(const StringView&);
    void append(const Utf32View&);
    void append(char);
    void append_code_point(u32);
    void append(const char*, size_t);
    void appendf(const char*, ...) __attribute__((format(printf, 2, 3)));
    void appendvf(const char*, va_list);

    void append_escaped_for_json(const StringView&);

    template<typename... Parameters>
    void appendff(CheckedFormatString<Parameters...>&& fmtstr, const Parameters&... parameters)
    {
        vformat(*this, fmtstr.view(), VariadicFormatParams { parameters... });
    }

    String build() const;
    String to_string() const;
    ByteBuffer to_byte_buffer() const;

    StringView string_view() const;
    void clear();

    size_t length() const { return m_length; }
    bool is_empty() const { return m_length == 0; }
    void trim(size_t count) { m_length -= count; }

    template<class SeparatorType, class CollectionType>
    void join(const SeparatorType& separator, const CollectionType& collection)
    {
        bool first = true;
        for (auto& item : collection) {
            if (first)
                first = false;
            else
                append(separator);
            append(item);
        }
    }

private:
    void will_append(size_t);
    u8* data() { return m_buffer.is_null() ? m_inline_buffer : m_buffer.data(); }
    const u8* data() const { return m_buffer.is_null() ? m_inline_buffer : m_buffer.data(); }
    bool using_inline_buffer() const { return m_buffer.is_null(); }

    static constexpr size_t inline_capacity = 128;
    u8 m_inline_buffer[inline_capacity];
    ByteBuffer m_buffer;
    size_t m_length { 0 };
};

}

using AK::StringBuilder;
