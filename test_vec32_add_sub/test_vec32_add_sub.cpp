// test_vec32_add_sub.cpp : Defines the entry point for the console application.
//
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

// precondition v.size() != 0
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

int main()
{
  typedef std::chrono::high_resolution_clock myclock;

  uint32_t num_failed(0);
  uint32_t num_passed(0);
  const unsigned seed1(0xa134f102);

  {
    vec32 test;
    std::cout << "test_vec32_add_sub, implementation has max num of uint32_t =" << test.max_size() << std::endl;
  }



  {
    const std::string test_name("basic_predicates");
    vec32 zero;
    vec32 vw0 = { 0x30 }; // last word is most significant
    vec32 vw1 = { 0xFFFFFFFEu, 0x30 }; // last word is most significant
    vec32 vw2 = { 0xFFFFFFFFu, 0x30 };
    vec32 vw3 = { 0xFFFFFFFFu, 0x30 };

    if (Big_numbers::less_than(vw1, vw2) &&
      (not Big_numbers::less_than(vw2, vw1)) &&
      (vw3 == vw2) &&
      Big_numbers::less_than(vw0, vw1) &&
      Big_numbers::test_zero(zero) &&
      (not Big_numbers::test_zero(vw0)))
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
    const std::string test_name("add_no_carry");

    vec32 bnv32 = { 0x32u };
    vec32 bnv33 = { 0x33u };
    vec32 bnv1 = { 0x1u };
    BNat thirty_two(bnv32);
    BNat thirty_three(bnv33);
    BNat one(bnv1);

    BNat sum = Big_numbers::add(thirty_two, one);

    if (sum == thirty_three)
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
    const std::string test_name = "add word";

    uint32_t num_failed_local(0);
    {
      vec32 vw0 = {};
      uint32_t addend = 0x0u;
      vec32 expected = {};  // the 1u is the carry into MSB
      std::vector<uint32_t> result = Big_numbers::add_word(vw0, addend);
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
      std::vector<uint32_t> result = Big_numbers::add_word(vw0, addend);
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
      std::vector<uint32_t> result = Big_numbers::add_word(vw0, addend);
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

      std::vector<uint32_t> result = Big_numbers::add_word(vw1, addend);
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

      std::vector<uint32_t> result = Big_numbers::add_word(vw1, addend);
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

      std::vector<uint32_t> result = Big_numbers::add_word(vw1, addend);
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

      std::vector<uint32_t> result = Big_numbers::add_word(vw1, addend);
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

      std::vector<uint32_t> result = Big_numbers::add_word(vw1, addend);
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
    const std::string test_name("add_with_carry");
    vec32 vw1 = { 0xFFFFFFFEu }; // 2 more till rollover
    vec32 vw2 = { 0x33u };
    vec32 expected = { 0x31u, 0x1u };  // the 1u is the carry into MSB
    vec32 sum = Big_numbers::add_vec32(vw1, vw2);
    if (sum == expected)
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


  std::cout << "passed " << num_passed << " tests" << std::endl;
  std::cout << "failed " << num_failed << " tests" << std::endl;
  return num_failed;
}

//TODO test_add_word

// TODO test_add  
//std::vector<uint32_t> add_vec32(const std::vector<uint32_t>& a, const std::vector<uint32_t>&b);