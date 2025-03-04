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

#include <AK/StdLibExtras.h>
#include <AK/Types.h>

namespace AK {

struct DefaultComparator {
    template<typename T, typename S>
    constexpr int operator()(T& lhs, S& rhs)
    {
        if (lhs > rhs)
            return 1;

        if (lhs < rhs)
            return -1;

        return 0;
    }
};

template<typename Container, typename Needle, typename Comparator = DefaultComparator>
constexpr auto binary_search(
    Container&& haystack,
    Needle&& needle,
    size_t* nearby_index = nullptr,
    Comparator comparator = Comparator {}) -> decltype(&haystack[0])
{
    if (haystack.size() == 0) {
        if (nearby_index)
            *nearby_index = 0;
        return nullptr;
    }

    size_t low = 0;
    size_t high = haystack.size() - 1;
    while (low <= high) {
        size_t middle = low + (high - low) / 2;

        int comparison = comparator(needle, haystack[middle]);

        if (comparison < 0)
            if (middle != 0)
                high = middle - 1;
            else
                break;
        else if (comparison > 0)
            low = middle + 1;
        else {
            if (nearby_index)
                *nearby_index = middle;
            return &haystack[middle];
        }
    }

    if (nearby_index)
        *nearby_index = min(low, high);

    return nullptr;
}

}

using AK::binary_search;
