#pragma once


namespace fast_io::crypto::speck
{

namespace details
{

template<std::size_t alpha = 8, std::size_t beta = 3>
inline constexpr std::pair<uint64_t, uint64_t> speck_round(uint64_t x, uint64_t y, uint64_t k) {
    x = (x >> alpha) | (x << (8 * sizeof(x) - alpha));  // x = ROTR(x, 8)
    x += y;
    x ^= k;
    y = (y << beta) | (y >> (8 * sizeof(y) - beta));  // y = ROTL(y, 3)
    y ^= x;
    return {x, y};
}

template<std::size_t alpha = 8, std::size_t beta = 3>
inline constexpr std::pair<uint64_t, uint64_t> speck_back(uint64_t x, uint64_t y, uint64_t k) {
    y ^= x;
    y = (y >> beta) | (y << (8 * sizeof(y) - beta));  // y = ROTR(y, 3)
    x ^= k;
    x -= y;
    x = (x << alpha) | (x >> (8 * sizeof(x) - alpha));  // x = ROTL(x, 8)
    return {x, y};
}

}

template<bool encrypt, std::size_t blocksize, std::size_t keysize, std::size_t rounds>
struct speck
{
    static std::size_t constexpr block_size = blocksize;
    static std::size_t constexpr key_size = keysize;
    std::array<uint64_t, rounds + 1> key_schedule{};
    constexpr speck(uint8_t const *key)
    {
        std::array<uint64_t, key_size / sizeof(uint64_t)> subkeys{};
        memcpy(subkeys.data(), key, key_size);

        key_schedule[0] = subkeys[0];
        for (uint64_t i = 0; i != rounds - 1; ++i) {
            auto [a, b] = details::speck_round(subkeys[1], subkeys[0], i);
            if constexpr (key_size == 32)
            {
                subkeys[0] = b;
                subkeys[1] = subkeys[2];
                subkeys[2] = subkeys[3];
                subkeys[3] = a;
            } else if constexpr (key_size == 24) {
                subkeys[0] = b;
                subkeys[1] = subkeys[2];
                subkeys[2] = a;
            } else {
                subkeys[0] = b;
                subkeys[1] = a;
            }
            key_schedule[i + 1] = subkeys[0];
        }
    }
    constexpr auto operator()(uint8_t const *plaintext_or_ciphertext)
    {
        if constexpr (encrypt) {
            std::array<uint8_t, block_size> ciphertext{};
            auto cipher_as_uint64_t = static_cast<uint64_t*>(static_cast<void*>(ciphertext.data()));
            auto plain_as_uint64_t = static_cast<uint64_t const*>(static_cast<void const*>(plaintext_or_ciphertext));

            cipher_as_uint64_t[0] = plain_as_uint64_t[0];
            cipher_as_uint64_t[1] = plain_as_uint64_t[1];
            for (std::size_t i = 0; i != rounds; ++i) {
                auto [a, b] = details::speck_round(cipher_as_uint64_t[1], cipher_as_uint64_t[0], key_schedule[i]);
                cipher_as_uint64_t[1] = a;
                cipher_as_uint64_t[0] = b;
            }

            return ciphertext;
        } else {
            std::array<uint8_t, block_size> plaintext{};

            auto plain_as_uint64_t = static_cast<uint64_t*>(static_cast<void*>(plaintext.data()));
            auto cipher_as_uint64_t = static_cast<uint64_t const*>(static_cast<void const*>(plaintext_or_ciphertext));

            plain_as_uint64_t[0] = cipher_as_uint64_t[0];
            plain_as_uint64_t[1] = cipher_as_uint64_t[1];
            for (std::size_t i(rounds); i--;) {
                auto [a, b] = details::speck_back(plain_as_uint64_t[1], plain_as_uint64_t[0], key_schedule[i]);
                plain_as_uint64_t[1] = a;
                plain_as_uint64_t[0] = b;
            }

            return plaintext;
        }
    }
};

using speck_enc_128_128 = speck<true, 16, 16, 32>;
using speck_dec_128_128 = speck<false, 16, 16, 32>;

using speck_enc_128_192 = speck<true, 16, 24, 33>;
using speck_dec_128_192 = speck<false, 16, 24, 33>;

using speck_enc_128_256 = speck<true, 16, 32, 34>;
using speck_dec_128_256 = speck<false, 16, 32, 34>;

}

