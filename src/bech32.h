// Copyright (c) 2017 Pieter Wuille
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Bech32 is a string encoding format used in newer address types.
// The output consists of a human-readable part (alphanumeric), a
// separator character (1), and a base32 data section, the last
// 6 characters of which are a checksum.
//
// For more information, see BIP 173.

#ifndef BITCOIN_BECH32_H
#define BITCOIN_BECH32_H

#include <stdint.h>
#include <string>
#include <vector>

#include "allocators.h"

namespace bech32
{

/** Encode a Bech32 string. If hrp contains uppercase characters, this will cause an assertion error. */
std::string Encode(const std::string& hrp, const std::vector<uint8_t>& values);

/** Decode a Bech32 string. Returns (hrp, data). Empty hrp means failure. */
std::pair<std::string, std::vector<uint8_t,zero_after_free_allocator<uint8_t>>> Decode(const std::string& str);

} // namespace bech32

class CBTCUAddress;
class CBTCUSecret;

/**
 * Base class for all bech32-encoded data
 */
class CBech32Data
{
    friend class CBTCUAddress;
    friend class CBTCUSecret;

    int nVersion = 255;

    //! the actually encoded data
    typedef std::vector<uint8_t, zero_after_free_allocator<uint8_t> > vector_uchar;
    vector_uchar vchData;

public:
    CBech32Data() = default;
    ~CBech32Data() = default;
    void SetData(int version, const void* pdata, size_t nSize);
    void Clear();

    bool SetString(const std::string& str);
    std::string ToString() const;
    int CompareTo(const CBech32Data& b32) const;
};

#endif // BITCOIN_BECH32_H
