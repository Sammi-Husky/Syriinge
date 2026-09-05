#pragma once

#include <types.h>

struct Version {
    union {
        struct {
            u8 major;
            u8 minor;
            u8 patch;
            u8 _pad; // alignment / reserved
        };
        u32 value; // Combined version as a single 32-bit integer
    };

    bool operator==(const Version& other) const
    {
        return value == other.value;
    }

    bool operator!=(const Version& other) const
    {
        return value != other.value;
    }

    bool operator<(const Version& other) const
    {
        return value < other.value;
    }
};

const Version SYRINGE_VERSION = { 0, 7, 0 };