// test_algorithm.cpp : Defines the entry point for the console application.
// test for arithmetic_algorithm.h, the templates for generic (iterator-based) arithmetic

//#include "stdafx.h"
#include "..\integer\integer.h"
#include <iostream>
#include <cstring>
#include <ctime>
#include <chrono>
#include <random>
#include <algorithm>
#include <iso646.h>   // so "not" 'or" and "and" work, VSC++ bug

using BNat = Big_numbers::Nat;
using BNat_mut = Big_numbers::Nat_mut;
using vec32 = std::vector<uint32_t>;

static vec32 make_random_vnat_of_size(size_t maxsize, std::minstd_rand0& generator)
{
  vec32 rnat;
  std::uniform_int_distribution<uint32_t> dist32(0, 0xffff'ffffu);
  std::uniform_int_distribution<uint32_t> dist1(1u, 0xffff'ffffu);
  std::uniform_int_distribution<size_t> dist(0, maxsize);

  size_t size = dist(generator);
  if (size == 0)
  {
    return rnat;
  }
  rnat.reserve(size);
  for (size_t i(0); i < size; ++i)
  {
    rnat.push_back(dist32(generator));
  }

  // eliminate MSW that are zero
  while ((rnat.size() != 0) && (rnat.back() == 0))
  {
    rnat.pop_back();
  }
  return rnat;
}

static vec32 make_random_nonzero_vnat_of_size(size_t maxsize, std::minstd_rand0& generator)
{
  vec32 rnat;
  std::uniform_int_distribution<uint32_t> dist32(0, 0xffffffffu);
  std::uniform_int_distribution<size_t> dist(1, maxsize);
  size_t size = dist(generator);
  if (size == 0)
  {
    return rnat;
  }
  rnat.reserve(size);
  for (size_t i(0); i < size; ++i)
  {
    rnat.push_back(dist32(generator));
  }
  if ((0 != rnat.size()) && (0u == rnat.back()))
  {
    rnat.back() = 1u;
  }
  return rnat;
}

// precondtion v.size() != 0
static vec32 make_random_nonzero_vnat_le(const vec32& v, std::minstd_rand0& generator)
{
  const size_t v_size(v.size());
#ifdef _DEBUG
  if (v_size == 0)
  {
    std::cerr << "make_random_nonzero_vnat_le: precondition violated, v.size == 0" << std::endl;
    exit(-1);
  }
#endif

  std::uniform_int_distribution<size_t> dist(1, v.size());
  size_t rnat_size = dist(generator);
  vec32 rnat(vec32(rnat_size, 0)); // zeros in rnat, to place MSWs (instead of pushing LSWs)

  if (v_size == 1u)
  {
    std::uniform_int_distribution<uint32_t> dist1v(1u, v[0]);
    rnat[0] = dist1v(generator);
  }

  std::uniform_int_distribution<uint32_t> dist32(0, 0xffffffffu);
  bool equal_so_far(true);
  for (size_t i(0); i < rnat_size; ++i)
  {
    const size_t index = rnat_size - (i + 1);
    const uint32_t vword = v[index];
    if (equal_so_far)
    {
      std::uniform_int_distribution<uint32_t> distsmall(0, vword);
      const uint32_t word = distsmall(generator);
      rnat[index] = word;
      equal_so_far &= (word == vword);
    }
    else
    {
      rnat[index] = dist32(generator);
    }
  }

  while ((rnat.size() != 0u) && (rnat.back() == 0))
  {
    rnat.pop_back();
  }
  if (rnat.size() == 0u)
  {
    // happened to choose the number zero, which is not supposed to be returned
    // since size of v is >1, any nonzero word will do
    std::uniform_int_distribution<uint32_t> dist1(1u, 0xffff'ffffu);
    rnat[0] = dist1(generator);
  }

  return rnat;
}




// mul_using_generator(), multiplies two vectors using a product generator.
// for testing Product_generator.
vec32 mul_using_generator(
  const vec32::const_iterator abegin,
  const vec32::const_iterator aend,
  const vec32::const_iterator bbegin,
  const vec32::const_iterator bend)
{
  vec32 result;
  arithmetic_algorithm::Product_generator<vec32::const_iterator, vec32::const_iterator> gen(abegin, aend, bbegin, bend);
  for (auto iter = gen.begin(); iter != gen.end(); ++iter)
  {
    const uint32_t value = *iter;
    result.push_back(value);
  }
  return result;
}

vec32 mul_word_to_vector_using_generator(
  const vec32::const_iterator abegin,
  const vec32::const_iterator aend,
  const uint32_t b)
{
  vec32 result;
  arithmetic_algorithm::Product_by_word_generator<vec32::const_iterator> gen(abegin, aend, b);
  for (auto iter = gen.begin(); iter != gen.end(); ++iter)
  {
    result.push_back(*iter);
  }
  return result;
}

vec32 add_word_to_vector_using_generator(const vec32::const_iterator abegin,
  const vec32::const_iterator aend,
  const uint32_t b)
{
  vec32 result;

  arithmetic_algorithm::Sum_by_word_generator<vec32::const_iterator> gen(abegin, aend, b);
  for (auto iter = gen.begin(); iter != gen.end(); ++iter)
  {
    result.push_back(*iter);
  }
  return result;
}

vec32 add_word_to_vector_using_generator_and_reverse(const vec32::const_iterator abegin,
  const vec32::const_iterator aend,
  const uint32_t b)
{
  vec32 result;
  arithmetic_algorithm::Sum_by_word_generator<vec32::const_iterator> gen(abegin, aend, b);
  auto riter = gen.rbegin();
  auto rend = gen.rend();
  for (; riter != rend; ++riter)
  {
    result.push_back(*riter);
  }
  std::reverse(result.begin(), result.end());
  return result;
}

vec32 add_using_generator(
  const vec32::const_iterator abegin,
  const vec32::const_iterator aend,
  const vec32::const_iterator bbegin,
  const vec32::const_iterator bend,
  bool carry_in = false)
{
  vec32 result;
  // TODO there is a bug, this loop is going on and on, expect iter==gen.end, what is happening? 
  arithmetic_algorithm::Sum_generator<vec32::const_iterator> gen(abegin, aend, bbegin, bend, carry_in);
  for (auto iter = gen.begin(); iter != gen.end(); ++iter)
  {
    result.push_back(*iter);
  }
  return result;
}

int main()
{
  typedef std::chrono::high_resolution_clock myclock;

  uint32_t num_failed(0);
  uint32_t num_passed(0);
  const unsigned seed1(0xa134f102);

  {
    vec32 test;
    std::cout << "starting test_big_numbers, implementation has max num of uint32_t =" << test.max_size() << std::endl;
  }

  {
    const std::string test_name = "add word using iterator";

    uint32_t num_failed_local(0);
    {
      vec32 vw0 = {};
      uint32_t addend = 0x0u;
      vec32 expected = {};  // the 1u is the carry into MSB
      vec32 result = add_word_to_vector_using_generator(vw0.begin(), vw0.end(), addend);
      if (result != expected)
      {
        ++num_failed_local;
        std::cout << "failed test " << test_name.c_str() << " part 0"
          << " result=" << result << " expected=" << expected << std::endl;
      }
    }

    if (num_failed_local == 0)
    {
      vec32 vw0 = {}; // 2 more till rollover
      uint32_t addend = 0xffffffffu;
      vec32 expected = { addend };  // the 1u is the carry into MSB
      vec32 result = add_word_to_vector_using_generator(vw0.begin(), vw0.end(), addend);
      if (result != expected)
      {
        ++num_failed_local;
        std::cout << "failed test " << test_name.c_str() << " part 1"
          << " result=" << result << " expected=" << expected << std::endl;
      }
    }
    if (num_failed_local == 0)
    {
      vec32 vw0 = { 0xffffffff }; // 2 more till rollover
      uint32_t addend = 0x3u;
      vec32 expected = { 0x2u, 0x1u };  // the 1u is the carry into MSB
      vec32 result = add_word_to_vector_using_generator(vw0.begin(), vw0.end(), addend);
      if (result != expected)
      {
        ++num_failed_local;
        std::cout << "failed test " << test_name.c_str() << " part 2"
          << " result=" << result << " expected=" << expected << std::endl;
      }
    }

    if (num_failed_local == 0)
    {
      vec32 vw1 = { 0xFFFFFFFEu }; // 2 more till rollover
      uint32_t addend = 0x33u;
      vec32 expected = { 0x31u, 0x1u };  // the 1u is the carry into MSB

      vec32 result = add_word_to_vector_using_generator(vw1.begin(), vw1.end(), addend);
      if (result != expected)
      {
        ++num_failed_local;
        std::cout << "failed test " << test_name.c_str() << " part 3"
          << " result=" << result << " expected=" << expected << std::endl;
      }
    }


    if (num_failed_local == 0)
    {
      vec32 vw1 = { 0xFFFFFFFFu, 0xFFFFFFFFu };
      uint32_t addend = 0x33u;
      vec32 expected = { 0x32u, 0x0u, 0x1u };  // the 1u is the carry into MSB

      vec32 result = add_word_to_vector_using_generator(vw1.begin(), vw1.end(), addend);
      if (result != expected)
      {
        ++num_failed_local;
        std::cout << "failed test " << test_name.c_str() << " part 4"
          << " result=" << result << " expected=" << expected << std::endl;
      }
    }

    if (num_failed_local == 0)
    {
      vec32 vw1 = { 0xffffffffu, 0x2u };
      uint32_t addend = 0x33u;
      vec32 expected = { 0x32u, 0x3u };  // the 1u is the carry into MSB

      vec32 result = add_word_to_vector_using_generator(vw1.begin(), vw1.end(), addend);
      if (result != expected)
      {
        ++num_failed_local;
        std::cout << "failed test " << test_name.c_str() << " part 5"
          << " result=" << result << " expected=" << expected << std::endl;
      }
    }

    if (num_failed_local == 0)
    {
      vec32 vw1 = { 0xfffffffe, 0xffffffffu, 0x2u };
      uint32_t addend = 0x33u;
      vec32 expected = { 0x31u, 0, 0x3u };  // the 1u is the carry into MSB

      vec32 result = add_word_to_vector_using_generator(vw1.begin(), vw1.end(), addend);
      if (result != expected)
      {
        ++num_failed;
        std::cout << "failed test " << test_name.c_str() << " part 6"
          << " result=" << result << " expected=" << expected << std::endl;
      }
    }

    if (num_failed_local == 0)
    {
      vec32 vw1 = { 0xFFFFFFFFu, 0xFFFFFFFFu };
      uint32_t addend = 0x1u;
      vec32 expected = { 0x0u, 0x0u, 0x1u };  // the 1u is the carry into MSB

      vec32 result = add_word_to_vector_using_generator(vw1.begin(), vw1.end(), addend);
      if (result != expected)
      {
        ++num_failed_local;
        std::cout << "failed test " << test_name.c_str() << " part 7"
          << " result=" << result << " expected=" << expected << std::endl;
      }
    }

    if (num_failed_local == 0)
    {
      ++num_passed;
      std::cout << "passed test " << test_name.c_str() << std::endl;
    }
    else
    {
      ++num_failed;
      std::cout << "failed test " << test_name.c_str() << std::endl;
    }

  }

  {
    const std::string test_name = "add word using rev_iterator";

    uint32_t num_failed_local(0);
    {
      vec32 vw0 = {};
      uint32_t addend = 0x0u;
      vec32 expected = {};  // the 1u is the carry into MSB
      vec32 result = add_word_to_vector_using_generator_and_reverse(vw0.begin(), vw0.end(), addend);
      if (result != expected)
      {
        ++num_failed_local;
        std::cout << "failed test " << test_name.c_str() << " part 0"
          << " result=" << result << " expected=" << expected << std::endl;
      }
    }

    if (num_failed_local == 0)
    {
      vec32 vw0 = {}; // 2 more till rollover
      uint32_t addend = 0xffffffffu;
      vec32 expected = { addend };  // the 1u is the carry into MSB
      vec32 result = add_word_to_vector_using_generator_and_reverse(vw0.begin(), vw0.end(), addend);
      if (result != expected)
      {
        ++num_failed_local;
        std::cout << "failed test " << test_name.c_str() << " part 1"
          << " result=" << result << " expected=" << expected << std::endl;
      }
    }
    if (num_failed_local == 0)
    {
      vec32 vw0 = { 0xffffffff }; // 2 more till rollover
      uint32_t addend = 0x3u;
      vec32 expected = { 0x2u, 0x1u };  // the 1u is the carry into MSB
      vec32 result = add_word_to_vector_using_generator_and_reverse(vw0.begin(), vw0.end(), addend);
      if (result != expected)
      {
        ++num_failed_local;
        std::cout << "failed test " << test_name.c_str() << " part 2"
          << " result=" << result << " expected=" << expected << std::endl;
      }
    }

    if (num_failed_local == 0)
    {
      vec32 vw1 = { 0xFFFFFFFEu }; // 2 more till rollover
      uint32_t addend = 0x33u;
      vec32 expected = { 0x31u, 0x1u };  // the 1u is the carry into MSB

      vec32 result = add_word_to_vector_using_generator_and_reverse(vw1.begin(), vw1.end(), addend);
      if (result != expected)
      {
        ++num_failed_local;
        std::cout << "failed test " << test_name.c_str() << " part 3"
          << " result=" << result << " expected=" << expected << std::endl;
      }
    }


    if (num_failed_local == 0)
    {
      vec32 vw1 = { 0xFFFFFFFFu, 0xFFFFFFFFu };
      uint32_t addend = 0x33u;
      vec32 expected = { 0x32u, 0x0u, 0x1u };  // the 1u is the carry into MSB

      vec32 result = add_word_to_vector_using_generator_and_reverse(vw1.begin(), vw1.end(), addend);
      if (result != expected)
      {
        ++num_failed_local;
        std::cout << "failed test " << test_name.c_str() << " part 4"
          << " result=" << result << " expected=" << expected << std::endl;
      }
    }

    if (num_failed_local == 0)
    {
      vec32 vw1 = { 0xffffffffu, 0x2u };
      uint32_t addend = 0x33u;
      vec32 expected = { 0x32u, 0x3u };  // the 1u is the carry into MSB

      vec32 result = add_word_to_vector_using_generator_and_reverse(vw1.begin(), vw1.end(), addend);
      if (result != expected)
      {
        ++num_failed_local;
        std::cout << "failed test " << test_name.c_str() << " part 5"
          << " result=" << result << " expected=" << expected << std::endl;
      }
    }

    if (num_failed_local == 0)
    {
      vec32 vw1 = { 0xfffffffe, 0xffffffffu, 0x2u };
      uint32_t addend = 0x33u;
      vec32 expected = { 0x31u, 0, 0x3u };  // the 1u is the carry into MSB

      vec32 result = add_word_to_vector_using_generator_and_reverse(vw1.begin(), vw1.end(), addend);
      if (result != expected)
      {
        ++num_failed;
        std::cout << "failed test " << test_name.c_str() << " part 6"
          << " result=" << result << " expected=" << expected << std::endl;
      }
    }

    if (num_failed_local == 0)
    {
      vec32 vw1 = { 0xFFFFFFFFu, 0xFFFFFFFFu };
      uint32_t addend = 0x1u;
      vec32 expected = { 0x0u, 0x0u, 0x1u };  // the 1u is the carry into MSB

      vec32 result = add_word_to_vector_using_generator_and_reverse(vw1.begin(), vw1.end(), addend);
      if (result != expected)
      {
        ++num_failed_local;
        std::cout << "failed test " << test_name.c_str() << " part 7"
          << " result=" << result << " expected=" << expected << std::endl;
      }
    }

    if (num_failed_local == 0)
    {
      ++num_passed;
      std::cout << "passed test " << test_name.c_str() << std::endl;
    }
    else
    {
      ++num_failed;
      std::cout << "failed test " << test_name.c_str() << std::endl;
    }

  }

  {
    const std::string test_name("mult_using_generator_test");
    // multiply psuedo-random numbers.
    // compare generator (lazy) with eager result.
    bool success(true);
    std::minstd_rand0 generator(seed1);

    for (unsigned i(0); i < 100; ++i)
    {
      vec32 a = make_random_vnat_of_size(255u, generator);
      vec32 b = make_random_vnat_of_size(255u, generator);

      vec32 result2 = Big_numbers::mul_vec32(a, b);

      // also test a generator that does convolutional multiply
      vec32 resultv = mul_using_generator(a.begin(), a.end(), b.begin(), b.end());
      if (result2 != resultv)
      {
        success = false;
        std::cout << "fail of generator in" << test_name.c_str()
          << " fuzz index=" << i
          << "  mul_old_fashioned != mul_cov,  result sizes (mul, gen)=" << result2.size() << ", " << resultv.size() << std::endl;
        std::cout << "\nmul result= " << result2 << std::endl;
        std::cout << "\ngen_result= " << resultv << std::endl;
        return -1;
      }
    }  // end for

    if (success)
    {
      ++num_passed;
      //BNat n(accumulator);
      //std::cout << "number=" << n << std::endl;
      std::cout << "passed test " << test_name.c_str() << std::endl;
    }
    else
    {
      ++num_failed;
      std::cout << "failed test " << test_name.c_str() << std::endl;
      return -1;
    }
  }


  std::cout << "passed " << num_passed << " tests" << std::endl;
  std::cout << "failed " << num_failed << " tests" << std::endl;
  return num_failed;
}
