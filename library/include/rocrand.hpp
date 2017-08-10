// Copyright (c) 2017 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef ROCRAND_HPP_
#define ROCRAND_HPP_

// At least C++11 required
#if defined(__cplusplus) && __cplusplus >= 201103L

#include <random>
#include <exception>
#include <string>
#include <sstream>
#include <type_traits>

#include "rocrand.h"
#include "rocrand_kernel.h"

namespace rocrand_cpp {

/// \class error
/// \brief A run-time rocRAND error.
///
/// The error class represents an error returned from an rocRAND
/// function.
///
/// \see context_error
class error : public std::exception
{
public:
    typedef rocrand_status error_type;

    error(error_type error) noexcept
        : m_error(error),
          m_error_string(to_string(error))
    {

    }

    ~error() noexcept
    {
    }

    /// Returns the numeric error code.
    error_type error_code() const noexcept
    {
        return m_error;
    }

    /// Returns a string description of the error.
    std::string error_string() const noexcept
    {
        return m_error_string;
    }

    /// Returns a C-string description of the error.
    const char* what() const noexcept
    {
        return m_error_string.c_str();
    }

    static std::string to_string(error_type error)
    {
        switch(error)
        {
            // TODO
            default: {
                std::stringstream s;
                s << "Unknown rocRAND Error (" << error << ")";
                return s.str();
            }
        }
    }

private:
    error_type m_error;
    std::string m_error_string;
};

namespace detail {

template<rocrand_rng_type GeneratorType>
class rng_engine
{
public:
    typedef unsigned int result_type; // 32bit uint

    rng_engine()
    {
        rocrand_status status;
        status = rocrand_create_generator(&m_generator, GeneratorType);
        if(status != ROCRAND_STATUS_SUCCESS) throw rocrand_cpp::error(status);
    }

    ~rng_engine()
    {
        rocrand_status status = rocrand_destroy_generator(m_generator);
        if(status != ROCRAND_STATUS_SUCCESS) throw rocrand_cpp::error(status);
    }

    void stream(hipStream_t value)
    {
        rocrand_status status = rocrand_set_stream(m_generator, value);
        if(status != ROCRAND_STATUS_SUCCESS) throw rocrand_cpp::error(status);
    }

protected:
    rocrand_generator m_generator;
};

template<rocrand_rng_type GeneratorType, unsigned long long DefaultSeed>
class prng_engine : public rng_engine<GeneratorType>
{
    typedef rng_engine<GeneratorType> base_type;

public:

    typedef unsigned long long seed_type; // 64bit uint
    typedef unsigned long long offset_type;
    typedef typename base_type::result_type result_type;

    prng_engine(seed_type seed_value = DefaultSeed,
                offset_type offset_value = 0)
        : base_type()
    {
        this->seed(seed_value);
        if(offset_value > 0)
        {
            this->offset(offset_value);
        }
    }

    ~prng_engine()
    {
    }

    void seed(seed_type value)
    {
        rocrand_status status = rocrand_set_seed(this->m_generator, value);
        if(status != ROCRAND_STATUS_SUCCESS) throw rocrand_cpp::error(status);
    }

    void offset(offset_type value)
    {
        rocrand_status status = rocrand_set_offset(this->m_generator, value);
        if(status != ROCRAND_STATUS_SUCCESS) throw rocrand_cpp::error(status);
    }
};

} // end detail namespace

template<class IntType = unsigned int>
class uniform_int_distribution
{
public:
    typedef IntType result_type;

    uniform_int_distribution()
    {
        static_assert(
            std::is_same<unsigned int, IntType>::value,
             "Only unsigned int type is supported in uniform_int_distribution"
        );
    }

    void reset()
    {
    }

    template<class Generator>
    void operator()(Generator& g, IntType * output, size_t size)
    {
        rocrand_status status = rocrand_generate(g.m_generator, output, size);
        if(status != ROCRAND_STATUS_SUCCESS) throw rocrand_cpp::error(status);
    }
};

template<class RealType = float>
class uniform_real_distribution
{
public:
    typedef RealType result_type;

    uniform_real_distribution()
    {
        static_assert(
             std::is_same<float, RealType>::value
                || std::is_same<double, RealType>::value,
             "Only float and double types are supported in uniform_real_distribution"
        );
    }

    void reset()
    {
    }

    template<class Generator>
    void operator()(Generator& g, RealType * output, size_t size)
    {
        rocrand_status status;
        status = this->generate(g, output, size);
        if(status != ROCRAND_STATUS_SUCCESS) throw rocrand_cpp::error(status);
    }

private:
    template<class Generator>
    rocrand_status generate(Generator& g, float * output, size_t size)
    {
        return rocrand_generate_uniform(g.m_generator, output, size);
    }

    template<class Generator>
    rocrand_status generate(Generator& g, double * output, size_t size)
    {
        return rocrand_generate_uniform_double(g.m_generator, output, size);
    }
};

template<class RealType = float>
class normal_distribution
{
public:
    typedef RealType result_type;

    class param_type
    {
    public:
        using distribution_type = normal_distribution<RealType>;
        param_type(RealType mean = 0.0, RealType stddev = 1.0)
            : m_mean(mean), m_stddev(stddev)
        {

        }

        RealType mean() const
        {
            return m_mean;
        }

        RealType stddev() const
        {
            return m_stddev;
        }

        bool operator==(const param_type& other)
        {
            return m_mean == other.m_mean && m_stddev == other.m_stddev;
        }

        bool operator!=(const param_type& other)
        {
            return !(*this == other);
        }

        RealType m_mean;
        RealType m_stddev;
    };

    normal_distribution(RealType mean = 0.0, RealType stddev = 1.0)
        : m_params(mean, stddev)
    {
        static_assert(
             std::is_same<float, RealType>::value
                || std::is_same<double, RealType>::value,
             "Only float and double types are supported in normal_distribution"
        );
    }

    void reset()
    {
    }

    RealType mean() const
    {
        return m_params.mean();
    }

    RealType stddev() const
    {
        return m_params.stddev();
    }

    param_type param() const
    {
        return m_params;
    }

    void param(const param_type& params)
    {
        m_params = params;
    }

    template<class Generator>
    void operator()(Generator& g, RealType * output, size_t size)
    {
        rocrand_status status;
        status = this->generate(g, output, size);
        if(status != ROCRAND_STATUS_SUCCESS) throw rocrand_cpp::error(status);
    }

private:
    template<class Generator>
    rocrand_status generate(Generator& g, float * output, size_t size)
    {
        return rocrand_generate_normal(
            g.m_generator, output, size, this->mean(), this->stddev()
        );
    }

    template<class Generator>
    rocrand_status generate(Generator& g, double * output, size_t size)
    {
        return rocrand_generate_normal_double(
            g.m_generator, output, size, this->mean(), this->stddev()
        );
    }

    param_type m_params;
};

class philox4x32_10_engine : public detail::prng_engine<ROCRAND_RNG_PSEUDO_PHILOX4_32_10, ROCRAND_PHILOX4x32_DEFAULT_SEED>
{
    typedef detail::prng_engine<ROCRAND_RNG_PSEUDO_PHILOX4_32_10, ROCRAND_PHILOX4x32_DEFAULT_SEED> base_type;
public:
    using base_type::base_type;

    template<class T>
    friend class ::rocrand_cpp::uniform_int_distribution;

    template<class T>
    friend class ::rocrand_cpp::uniform_real_distribution;

    template<class T>
    friend class ::rocrand_cpp::normal_distribution;
};

class xorwow_engine : public detail::prng_engine<ROCRAND_RNG_PSEUDO_XORWOW, ROCRAND_XORWOW_DEFAULT_SEED>
{
    typedef detail::prng_engine<ROCRAND_RNG_PSEUDO_XORWOW, ROCRAND_XORWOW_DEFAULT_SEED> base_type;
public:
    using base_type::base_type;

    template<class T>
    friend class ::rocrand_cpp::uniform_int_distribution;

    template<class T>
    friend class ::rocrand_cpp::uniform_real_distribution;

    template<class T>
    friend class ::rocrand_cpp::normal_distribution;
};

class mrg32k3a_engine : public detail::prng_engine<ROCRAND_RNG_PSEUDO_MRG32K3A, ROCRAND_MRG32K3A_DEFAULT_SEED>
{
    typedef detail::prng_engine<ROCRAND_RNG_PSEUDO_MRG32K3A, ROCRAND_MRG32K3A_DEFAULT_SEED> base_type;
public:
    using base_type::base_type;

    template<class T>
    friend class ::rocrand_cpp::uniform_int_distribution;

    template<class T>
    friend class ::rocrand_cpp::uniform_real_distribution;

    template<class T>
    friend class ::rocrand_cpp::normal_distribution;
};

} // end namespace rocrand

#endif // #if __cplusplus >= 201103L
#endif // ROCRAND_HPP_
