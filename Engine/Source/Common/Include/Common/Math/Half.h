//
// Created by johnk on 2023/5/9.
//

#pragma once

#include <cstdint>
#include <cmath>
#include <bit>

#include <Common/Math/Common.h>

namespace Common::Internal {
    template <std::endian E>
    requires (E == std::endian::little) || (E == std::endian::big)
    struct FullFloatBase {};

    template <>
    struct FullFloatBase<std::endian::little> {
        union {
            float value;
            struct {
                uint32_t mantissa : 23;
                uint32_t exponent : 8;
                uint32_t sign : 1;
            };
        };

        explicit FullFloatBase(float inValue) : value(inValue) {}
    };

    template <>
    struct FullFloatBase<std::endian::big> {
        union {
            float value;
            struct {
                uint32_t sign : 1;
                uint32_t exponent : 8;
                uint32_t mantissa : 23;
            };
        };

        explicit FullFloatBase(float inValue) : value(inValue) {}
    };

    template <std::endian E>
    struct FullFloat : public FullFloatBase<E> {
        FullFloat() : FullFloatBase<E>(0) {}
        FullFloat(float inValue) : FullFloatBase<E>(inValue) {} // NOLINT
        FullFloat(const FullFloat& inValue) : FullFloatBase<E>(inValue.value) {}
        FullFloat(FullFloat&& inValue) noexcept : FullFloatBase<E>(inValue.value) {}
    };
}

namespace Common {
    template <std::endian E>
    requires (E == std::endian::little) || (E == std::endian::big)
    struct HalfFloatBase {};

    template <>
    struct HalfFloatBase<std::endian::little> {
        union {
            uint16_t value;
            struct {
                uint16_t mantissa : 10;
                uint16_t exponent : 5;
                uint16_t sign : 1;
            };
        };

        explicit HalfFloatBase(uint16_t inValue) : value(inValue) {}
    };

    template <>
    struct HalfFloatBase<std::endian::big> {
        union {
            uint16_t value;
            struct {
                uint16_t sign : 1;
                uint16_t exponent : 5;
                uint16_t mantissa : 10;
            };
        };

        explicit HalfFloatBase(uint16_t inValue) : value(inValue) {}
    };

    template <std::endian E>
    struct HalfFloat : public HalfFloatBase<E> {
        inline HalfFloat() : HalfFloatBase<E>(0) {}
        inline HalfFloat(const HalfFloat& inValue) : HalfFloatBase<E>(inValue.value) {}
        inline HalfFloat(HalfFloat&& inValue) noexcept : HalfFloatBase<E>(inValue.value) {}

        inline HalfFloat(float inValue) : HalfFloatBase<E>(0) // NOLINT
        {
            Set(inValue);
        }

        inline HalfFloat& operator=(float inValue)
        {
            Set(inValue);
            return *this;
        }

        inline HalfFloat& operator=(const HalfFloat& inValue)
        {
            this->value = inValue.value;
            return *this;
        }

        void Set(float inValue)
        {
            Internal::FullFloat<E> full(inValue);

            this->sign = full.sign;
            if (full.exponent <= 112)
            {
                this->exponent = 0;
                this->mantissa = 0;

                const int32_t newExp = full.exponent - 127 + 15;

                if ((14 - newExp) <= 24)
                {
                    const uint32_t tMantissa = full.mantissa | 0x800000;
                    this->mantissa = static_cast<uint16_t>(tMantissa >> (14 - newExp));
                    if (((this->mantissa >> (13 - newExp)) & 1) != 0u)
                    {
                        this->value++;
                    }
                }
            }
            else if (full.exponent >= 143)
            {
                this->exponent = 30;
                this->mantissa = 1023;
            }
            else
            {
                // TODO static_cast
                this->exponent = static_cast<uint16_t>(static_cast<int32_t>(full.exponent) - 127 + 15);
                this->mantissa = static_cast<uint16_t>(full.mantissa >> 13);
            }
        }

        float AsFloat() const
        {
            Internal::FullFloat<E> result;

            result.sign = this->sign;
            if (this->exponent == 0)
            {
                uint32_t tMantissa = this->mantissa;
                if(tMantissa == 0)
                {
                    result.exponent = 0;
                    result.mantissa = 0;
                }
                else
                {
                    uint32_t mantissaShift = 10 - static_cast<uint32_t>(std::trunc(std::log2(static_cast<float>(tMantissa))));
                    result.exponent = 127 - (15 - 1) - mantissaShift;
                    result.mantissa = tMantissa << (mantissaShift + 23 - 10);
                }
            }
            else if (this->exponent == 31)
            {
                result.exponent = 142;
                result.mantissa = 8380416;
            }
            else
            {
                result.exponent = static_cast<int32_t>(this->exponent) - 15 + 127;
                result.mantissa = static_cast<uint32_t>(this->mantissa) << 13;
            }
            return result.value;
        }

        inline operator float() const // NOLINT
        {
            return AsFloat();
        }

        inline bool operator==(float rhs) const
        {
            return std::abs(AsFloat() - rhs) < halfEpsilon;
        }

        inline bool operator!=(float rhs) const
        {
            return std::abs(AsFloat() - rhs) >= halfEpsilon;
        }

        inline bool operator==(const HalfFloat& rhs) const
        {
            return std::abs(AsFloat() - rhs.AsFloat()) < halfEpsilon;
        }

        inline bool operator!=(const HalfFloat& rhs) const
        {
            return std::abs(AsFloat() - rhs.AsFloat()) >= halfEpsilon;
        }

        inline bool operator>(const HalfFloat& rhs) const
        {
            return AsFloat() > rhs.AsFloat();
        }

        inline bool operator<(const HalfFloat& rhs) const
        {
            return AsFloat() < rhs.AsFloat();
        }

        inline bool operator>=(const HalfFloat& rhs) const
        {
            return AsFloat() >= rhs.AsFloat();
        }

        inline bool operator<=(const HalfFloat& rhs) const
        {
            return AsFloat() <= rhs.AsFloat();
        }

        inline HalfFloat operator+(float rhs) const
        {
            return { AsFloat() + rhs };
        }

        inline HalfFloat operator-(float rhs) const
        {
            return { AsFloat() - rhs };
        }

        inline HalfFloat operator*(float rhs) const
        {
            return { AsFloat() * rhs };
        }

        inline HalfFloat operator/(float rhs) const
        {
            return { AsFloat() / rhs };
        }

        inline HalfFloat operator+(const HalfFloat& rhs) const
        {
            return { AsFloat() + rhs.AsFloat() };
        }

        inline HalfFloat operator-(const HalfFloat& rhs) const
        {
            return { AsFloat() - rhs.AsFloat() };
        }

        inline HalfFloat operator*(const HalfFloat& rhs) const
        {
            return { AsFloat() * rhs.AsFloat() };
        }

        inline HalfFloat operator/(const HalfFloat& rhs) const
        {
            return { AsFloat() / rhs.AsFloat() };
        }

        inline HalfFloat& operator+=(float rhs)
        {
            Set(AsFloat() + rhs);
            return *this;
        }

        inline HalfFloat& operator-=(float rhs)
        {
            Set(AsFloat() - rhs);
            return *this;
        }

        inline HalfFloat& operator*=(float rhs)
        {
            Set(AsFloat() * rhs);
            return *this;
        }

        inline HalfFloat& operator/=(float rhs)
        {
            Set(AsFloat() / rhs);
            return *this;
        }

        inline HalfFloat& operator+=(const HalfFloat& rhs)
        {
            Set(AsFloat() + rhs.AsFloat());
            return *this;
        }

        inline HalfFloat& operator-=(const HalfFloat& rhs)
        {
            Set(AsFloat() - rhs.AsFloat());
            return *this;
        }

        inline HalfFloat& operator*=(const HalfFloat& rhs)
        {
            Set(AsFloat() * rhs.AsFloat());
            return *this;
        }

        inline HalfFloat& operator/=(const HalfFloat& rhs)
        {
            Set(AsFloat() / rhs.AsFloat());
            return *this;
        }
    };

    using HFloat = HalfFloat<std::endian::native>;

    template <typename T>
    constexpr bool isHalfFloatingPointV = std::is_same_v<T, HFloat>;

    template <typename T>
    constexpr bool isFloatingPointV = std::is_floating_point_v<T> || isHalfFloatingPointV<T>;
}
