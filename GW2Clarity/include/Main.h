#pragma once
#include "Common.h"

namespace GW2Clarity
{
struct StackedBuff
{
    u32 id;
    i32 count;
};

union Id
{
    i32 id;
    struct
    {
        i16 grid;
        i16 item;
    };

    template<std::integral T1, std::integral T2>
    constexpr Id(T1 g, T2 i) : grid(i16(g)), item(i16(i)) { }

    constexpr bool operator==(Id other) const { return id == other.id; }
    constexpr bool operator!=(Id other) const { return id != other.id; }
};
static_assert(sizeof(Id) == sizeof(i32));

static inline constexpr i16 UnselectedSubId = -1;

template<std::integral T = i16>
static Id Unselected(T gid = T(UnselectedSubId)) {
    return { i16(gid), UnselectedSubId };
}

} // namespace GW2Clarity
