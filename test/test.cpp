// test_big_numbers.cpp : Defines the entry point for the console application.
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
#include <windows.h>  // for Sleep

using BNat = Big_numbers::Nat;
//using BNat_mut = Big_numbers::Nat_mut;
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



static uint32_t get_MSW_mask(uint32_t b)
{
  uint32_t mask(1);

  // find the string of 1 bits that is >= b.
  for (uint32_t i(0); i < 32u; ++i)
  {
    if (mask >= b)
    {
      break;
    }
    mask |= (mask << 1);
  }
  return mask;
}



// mul_using_generator(), multiplies two vectors using a product generator.
// for testing Product_generator.
std::vector<uint32_t> mul_using_generator(
  const std::vector <uint32_t>::const_iterator abegin,
  const std::vector<uint32_t>::const_iterator aend,
  const std::vector <uint32_t>::const_iterator bbegin,
  const std::vector<uint32_t>::const_iterator bend)
{
  std::vector<uint32_t> result;
  arithmetic_algorithm::Product_generator<std::vector<uint32_t>::const_iterator, std::vector<uint32_t>::const_iterator> gen(abegin, aend, bbegin, bend);
  for (auto iter = gen.begin(); iter != gen.end(); ++iter)
  {
    const uint32_t value = *iter;
    result.push_back(value);
  }
  return result;
}

std::vector<uint32_t> mul_word_to_vector_using_generator(
  const std::vector <uint32_t>::const_iterator abegin,
  const std::vector<uint32_t>::const_iterator aend,
  const uint32_t b)
{
  std::vector<uint32_t> result;
  arithmetic_algorithm::Product_by_word_generator<std::vector<uint32_t>::const_iterator> gen(abegin, aend, b);
  for (auto iter = gen.begin(); iter != gen.end(); ++iter)
  {
    result.push_back(*iter);
  }
  return result;
}

std::vector<uint32_t> add_word_to_vector_using_generator(const std::vector <uint32_t>::const_iterator abegin,
  const std::vector<uint32_t>::const_iterator aend,
  const uint32_t b)
{
  std::vector<uint32_t> result;

  arithmetic_algorithm::Sum_by_word_generator<std::vector<uint32_t>::const_iterator> gen(abegin, aend, b);
  for (auto iter = gen.begin(); iter != gen.end(); ++iter)
  {
    result.push_back(*iter);
  }
  return result;
}

std::vector<uint32_t> add_word_to_vector_using_generator_and_reverse(const std::vector <uint32_t>::const_iterator abegin,
  const std::vector<uint32_t>::const_iterator aend,
  const uint32_t b)
{
  std::vector<uint32_t> result;
  arithmetic_algorithm::Sum_by_word_generator<std::vector<uint32_t>::const_iterator> gen(abegin, aend, b);
  auto riter = gen.rbegin();
  auto rend = gen.rend();
  for (; riter != rend; ++riter)
  {
    result.push_back(*riter);
  }
  std::reverse(result.begin(), result.end());
  return result;
}

std::vector<uint32_t> add_using_generator(
  const std::vector <uint32_t>::const_iterator abegin,
  const std::vector<uint32_t>::const_iterator aend,
  const std::vector<uint32_t>::const_iterator bbegin,
  const std::vector<uint32_t>::const_iterator bend,
  bool carry_in = false)
{
  std::vector<uint32_t> result;
  // TODO there is a bug, this loop is going on and on, expect iter==gen.end, what is happening? 
  arithmetic_algorithm::Sum_generator<std::vector<uint32_t>::const_iterator> gen(abegin, aend, bbegin, bend, carry_in);
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
      std::vector<uint32_t> result = add_word_to_vector_using_generator(vw0.begin(), vw0.end(), addend);
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
      std::vector<uint32_t> result = add_word_to_vector_using_generator(vw0.begin(), vw0.end(), addend);
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
      std::vector<uint32_t> result = add_word_to_vector_using_generator(vw0.begin(), vw0.end(), addend);
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

      std::vector<uint32_t> result = add_word_to_vector_using_generator(vw1.begin(), vw1.end(), addend);
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

      std::vector<uint32_t> result = add_word_to_vector_using_generator(vw1.begin(), vw1.end(), addend);
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

      std::vector<uint32_t> result = add_word_to_vector_using_generator(vw1.begin(), vw1.end(), addend);
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

      std::vector<uint32_t> result = add_word_to_vector_using_generator(vw1.begin(), vw1.end(), addend);
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

      std::vector<uint32_t> result = add_word_to_vector_using_generator(vw1.begin(), vw1.end(), addend);
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
      std::vector<uint32_t> result = add_word_to_vector_using_generator_and_reverse(vw0.begin(), vw0.end(), addend);
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
      std::vector<uint32_t> result = add_word_to_vector_using_generator_and_reverse(vw0.begin(), vw0.end(), addend);
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
      std::vector<uint32_t> result = add_word_to_vector_using_generator_and_reverse(vw0.begin(), vw0.end(), addend);
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

      std::vector<uint32_t> result = add_word_to_vector_using_generator_and_reverse(vw1.begin(), vw1.end(), addend);
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

      std::vector<uint32_t> result = add_word_to_vector_using_generator_and_reverse(vw1.begin(), vw1.end(), addend);
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

      std::vector<uint32_t> result = add_word_to_vector_using_generator_and_reverse(vw1.begin(), vw1.end(), addend);
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

      std::vector<uint32_t> result = add_word_to_vector_using_generator_and_reverse(vw1.begin(), vw1.end(), addend);
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

      std::vector<uint32_t> result = add_word_to_vector_using_generator_and_reverse(vw1.begin(), vw1.end(), addend);
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
    const std::string test_name("add_no_carry");

    vec32 bnv32 = { 0x32u };
    vec32 bnv33 = { 0x33u };
    vec32 bnv1 = { 0x1u };
    BNat thirty_two(bnv32);
    BNat thirty_three(bnv33);
    BNat one(bnv1);

    BNat sum = add(thirty_two, one);

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
    const std::string test_name("add_with_carry");
    vec32 vw1 = { 0xFFFFFFFEu }; // 2 more till rollover
    vec32 vw2 = { 0x33u };
    vec32 vexpect = { 0x31u, 0x1u };  // the 1u is the carry into MSB
    BNat big1(vw1);
    BNat big2(vw2);
    BNat expected(vexpect);
    BNat sum = add(big1, big2);
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


  {
    const std::string test_name("test_order_operators");
    vec32 vw0 = { 0x30 }; // last word is most significant
    vec32 vw1 = { 0xFFFFFFFEu, 0x30 }; // last word is most significant
    vec32 vw2 = { 0xFFFFFFFFu, 0x30 };
    vec32 vw3 = { 0xFFFFFFFFu, 0x30 };
    BNat big0(vw0);
    BNat big1(vw1);
    BNat big2(vw2);
    BNat big3(vw3);

    if ((big1 < big2) && not (big2 < big1) && (big3 == big2) && (big0 < big1))
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
    const std::string test_name("add_fuzz_test"); //mtj
    // add psuedo-random numbers.
    // also use Sum_generator and compare results for consistency
    bool success(true);
    std::minstd_rand0 generator(seed1);
#ifdef _DEBUG
    const size_t num_iters(10'000u);
#else
    const size_t num_iters(1'000'000u);
#endif
    // TODO adjust num_iters based on debug or releasef
    for (size_t i(0); i < num_iters; ++i)
    {
      vec32 a = make_random_vnat_of_size(5u, generator);
      vec32 b = make_random_vnat_of_size(5u, generator);

      vec32 result1 = add_using_generator(a.begin(), a.end(), b.begin(), b.end());  // TODO mtj
      vec32 result2 = Big_numbers::add_vec32(a, b);

      if (result1 != result2)
      {
        success = false;
        std::cout << "fail of " << test_name.c_str() << " fuzz index=" << i << " , add_using_generator != add_vec32,  result sizes=" << result1.size() << ", " << result2.size() << std::endl;
        std::cout << "\n arg1= " << a << std::endl;
        std::cout << "\n arg2 = " << b << std::endl;
        std::cout << "\nresult1= " << result1 << std::endl;
        std::cout << "\nresult2= " << result2 << std::endl;
        return -1;
      }


    }
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

  {
    const std::string test_name("conv_mul");

    vec32 vop1 = { 0x1u, 0x3u }; // 0x3'0000'0001
    vec32 vop2 = { 0x2u, 0x4u }; // 0x4'0000'0002
    vec32 vres = { 2u, 0xAu, 0xCu };
    BNat bigop1(vop1);
    BNat bigop2(vop2);
    BNat bigres(vres);

    BNat result = Big_numbers::mul(bigop1, bigop2);
    if (result != bigres)
    {
      ++num_failed;
      std::cout << "failed test " << test_name.c_str() << std::endl;
      std::cout << " size of result=" << result.num.d.size() << std::endl;
      std::cout << " result= " << result << std::endl;
      std::cout << " expected= " << bigres << std::endl;
      std::cout << "operand1= " << bigop1 << std::endl;
      std::cout << "operand2= " << bigop2 << std::endl;
      return -1;
    }

    // also test a generator that does convolutional multiply
    vec32 resultv = mul_using_generator(vop1.begin(), vop1.end(),
                                        vop2.begin(), vop2.end());
    if (resultv == vres)
    {
      ++num_passed;
      std::cout << "passed test " << test_name.c_str() << std::endl;
    }
    else
    {
      ++num_failed;
      std::cout << "failed generator test " << test_name.c_str() << std::endl;
      std::cout << " size of result=" << resultv.size() << std::endl;
      std::cout << " result= " << resultv << std::endl;
      std::cout << " expected= " << vres << std::endl;
      std::cout << "operand1= " << vop1 << std::endl;
      std::cout << "operand2= " << vop2 << std::endl;
      return -1;
    }

  }

  {
    const std::string test_name("div by word");

    vec32 vw20 = { 0x20u }; // last word is most significant
    vec32 vw30 = { 0x7u, 0x30u };
    vec32 prod_plus_10 = { 0xEAu, 0x600u };
    // prod_plus_10 is 10 more than vw20*vw30, so dividing by vw20 should give quot=vw30 and remainder 10  
    BNat big20(vw20);
    BNat big30(vw30);
    BNat big_prod_plus_10(prod_plus_10);
    //BNat product = Big_numbers::conv_mul(big20, big30);
    std::pair<BNat, uint32_t> temp = div(big_prod_plus_10, 0x20u);
    if ((temp.second == 10) && (temp.first == big30))
    {
      ++num_passed;
      std::cout << "passed test " << test_name.c_str() << std::endl;
    }
    else
    {
      ++num_failed;
      std::cout << "failed test " << test_name.c_str() << std::endl;
      std::cout << " result_quotient= " << temp.second << " result_remainder=" << temp.second << std::endl;
      std::cout << " expected_quotient=" << vw30 << " expected_remainder=" << vec32(0x20u) << std::endl;
      return -1;
    }
  }

  {
    const std::string test_name("div");

    BNat divsr1(vec32{ 0x20u });
    BNat num1(vec32{ 0xEEAD, 0x600u });//last is most significant

    BNat expect1_rem(vec32{ 0xDu });
    BNat expect1_quot(vec32{ 0x775, 0x30 });

    std::pair<BNat, BNat> temp1 = div(num1, divsr1);
    bool passed1 = ((temp1.second == expect1_rem) && (temp1.first == expect1_quot));
    if (not passed1)
    {
      std::cout << "failed subtest div1 of " << test_name.c_str() << std::endl;
      std::cout << " result_quotient= " << temp1.first << " result_remainder=" << temp1.second << std::endl;
      std::cout << " expected_quotient=" << expect1_quot << " expected_remainder=" << expect1_rem << std::endl;
      return -1;
    }


    BNat num2(vec32{ 0xBEEF, 0x60Fu });
    BNat divsr2(vec32{ 0x0,0x20u });    // should divide about 0x30 times
    BNat expect2_quot(vec32{ 0x30 });
    BNat expect2_rem(vec32{ 0xBEEFu, 0xFu });


    std::pair<BNat, BNat> temp2 = div(num2, divsr2);
    bool passed2 = ((temp2.second == expect2_rem) && (temp2.first == expect2_quot));
    if (not passed2)
    {
      std::cout << "failed subtest div2 of " << test_name.c_str() << std::endl;
      std::cout << " result_quotient= " << temp2.first << " result_remainder=" << temp2.second << std::endl;
      std::cout << " expected_quotient=" << expect2_quot << " expected_remainder=" << expect2_rem << std::endl;
      return -1;
    }

    if (passed1 && passed2)
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
    const std::string test_name("conv_mul_forced_carry");

    BNat big2(vec32{ 0xFFFF'FFFFu, 0xFFFF'FFFF });
    BNat big3(vec32{ 0xFFFF'FFFFu, 0xFFFF'FFFFu, 0xFFFF'FFFFu });
    BNat expected(vec32{ 1u, 0u, 0xFFFF'FFFFu, 0xFFFF'FFFEu, 0xFFFF'FFFFu });

    BNat result = Big_numbers::mul(big2, big3);
    if (result == expected)
    {
      ++num_passed;
      std::cout << "passed test " << test_name.c_str() << std::endl;
    }
    else
    {
      ++num_failed;
      std::cout << "failed test " << test_name.c_str() << std::endl;
      std::cout << " size of result=" << result.num.d.size() << std::endl;
      std::cout << " result= " << result << std::endl;
      std::cout << " expected= " << expected << std::endl;
      return -1;
    }
  }

  {
    const std::string test_name("old-fashioned mul");

    vec32 vw2 = { 0xFFFF'FFFFu, 0xFFFF'FFFF }; // last word is most significant
    vec32 vw3 = { 0xFFFF'FFFFu, 0xFFFF'FFFFu, 0xFFFF'FFFFu };
    vec32 vw6 = { 1u, 0u, 0xFFFF'FFFFu, 0xFFFF'FFFEu, 0xFFFF'FFFFu };

    vec32 result = Big_numbers::mul_old_fashioned(vw2, vw3);
    if (result == vw6)
    {
      ++num_passed;
      std::cout << "passed test " << test_name.c_str() << std::endl;
    }
    else
    {
      ++num_failed;
      std::cout << "failed test " << test_name.c_str() << std::endl;
      std::cout << " size of result=" << result.size() << std::endl;
      std::cout << " result= " << result << std::endl;
      //std::cout << " expected= " << big6 << std::endl;
      return -1;
    }
  }

  {
    const std::string test_name("multiply_by_word_stress_test");
    time_t start_time;
    time(&start_time);

    vec32 accumulator;
    accumulator.push_back(1u);

    const size_t iterations(4'000u);
    for (size_t j(0u); j < iterations; ++j)
    {
      uint32_t word(0xFFFF'FFFEu);
      Big_numbers::scale_by_word(accumulator, word);  // keep accumulating in place
    }

    time_t end_time;
    time(&end_time);

    if (accumulator.size() == iterations)
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
      std::cout << "expected size=" << iterations << " actual size=" << accumulator.size() << std::endl;
      return -1;
    }
    double seconds = difftime(end_time, start_time);
    std::cout << "elapsed seconds:" << seconds << std::endl;
  }

  {
    const std::string test_name("decrement_at_index");

    vec32 accumulator = { 0x12341234, 0x4567 };  // MSW last
    const vec32 delta1 = { 0x1111 };
    size_t index = 1u;  // index of the MSW
    Big_numbers::decrement_at_index(accumulator, index, delta1);

    vec32 expected1 = { 0x12341234, 0x3456 };  // MSW last
    bool passed1 = (expected1 == accumulator);

    if (passed1)
    {
      ++num_passed;
      std::cout << "passed test " << test_name.c_str() << std::endl;
    }
    else
    {
      ++num_failed;
      std::cout << "failed test " << test_name.c_str() << std::endl;
      std::cout << "expected=" << expected1 << " actual=" << accumulator << std::endl;
      return -1;
    }
  }

#if 0
  {
    const std::string test_name("mutate by multiply");
    time_t start_time;
    time(&start_time);

    BNat_mut accumulator(1u);
    BNat_mut scale(0xFFFF'FFFEu);

    const size_t iterations(4'000u);
    for (size_t j(0u); j < iterations; ++j)
    {
      accumulator.scale_by(scale);
    }

    time_t end_time;
    time(&end_time);

    if (accumulator.num.d.size() == iterations)
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
      std::cout << "expected size=" << iterations << " actual size=" << accumulator.num.d.size() << std::endl;
      return -1;
    }
    double seconds;

    seconds = difftime(end_time, start_time);
    std::cout << "elapsed seconds:" << seconds << std::endl;
  }
#endif

  {
    const std::string test_name("multiply_fuzz_test");
    // multiply psuedo-random numbers.
    // compare fast-cache to convultion multiply for consistency.
    // later do same test to compare times.
    bool success(true);
    std::minstd_rand0 generator(seed1);
    //generator.seed(seed1);

    for (unsigned i(0); i < 100; ++i)
    {
      vec32 a = make_random_vnat_of_size(255u, generator);
      vec32 b = make_random_vnat_of_size(255u, generator);

      vec32 result1 = Big_numbers::mul_old_fashioned(a, b);
      vec32 result2 = Big_numbers::mul_vec32(a, b);

      if (result1 != result2)
      {
        success = false;
        std::cout << "fail of " << test_name.c_str() << " fuzz index=" << i << " , mul_old_fashioned != mul_cov,  result sizes=" << result1.size() << ", " << result2.size() << std::endl;
        std::cout << "\nmul_old_fashioned result= " << result1 << std::endl;
        std::cout << "\nmul result= " << result2 << std::endl;
        return -1;
      }


      // also test a generator that does convolutional multiply
      vec32 resultv = mul_using_generator(a.begin(), a.end(),
        b.begin(), b.end());
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


  {
    const std::string test_name("div_multiply_fuzz_test");
    std::cout << "running " << test_name.c_str() << std::endl;

    // generate random number d!=0, random r<d, random q,  and compute
    // n = q*d + r.
    // Then verify that 
    //   (q,r) = div(n,d)   (floor(n/d), n remainder d))

    bool success(true);

    std::minstd_rand0 generator(seed1);
    const size_t sizem(0x7Fu);
#ifdef _DEBUG
    const size_t num_iters(20u);
#else
    const size_t num_iters(1'000'000u);
#endif
    size_t iter_percent(0);
    myclock::time_point start2 = myclock::now();
    for (size_t i(0); i < num_iters; ++i)
    {
      size_t percent_done = (i * 100u) / num_iters;
      if (percent_done > iter_percent)
      {
        iter_percent = percent_done;
        if (0 == (iter_percent % 10))
        {
          std::cout << test_name.c_str() << " " << iter_percent << "% done" << std::endl;
        }
      }

      BNat q(make_random_vnat_of_size(sizem, generator));  // 7 is a mask for sizes to allow, between 1 and 7 words in the vector
      BNat d(make_random_nonzero_vnat_of_size(sizem, generator));
      BNat r(make_random_nonzero_vnat_le(d.num.d, generator));
      BNat n = (q*d) + r;

      auto result = div(n, d);

      if (result.second != r)
      {
        std::cout << "expected remainder=" << r << " actual remainder=" << result.second << std::endl;
        success = false;
        return -1;
      }

      if (result.first != q)
      {
        std::cout << "expected quotient=" << q << " actual quotient=" << result.first << std::endl;
        success = false;
        return -1;
        //break;
      }
    } // forloop

    if (success)
    {
      myclock::time_point end2 = myclock::now();
      std::chrono::duration<double> time_span2 =
        std::chrono::duration_cast<std::chrono::duration<double>>(end2 - start2);

      ++num_passed;
      std::cout << "passed test " << test_name.c_str() << " with " << num_iters << " iterations, elapsed seconds=" << time_span2.count() << std::endl;
    }
    else
    {
      ++num_failed;
      std::cout << "failed test " << test_name.c_str() << std::endl;
    }
  }




  {
    const std::string test_name("multiply_performance_fuzz_test");
    std::minstd_rand0 generator(seed1);
    //generator.seed(seed1);


    // multiply psuedo-random numbers.
    // compare fast-cache to convolution multiply for consistency.
    // later do same test to compare times.
    bool success(true);
    uint32_t parity(0);
#ifdef _DEBUG
    size_t max_size = 15u;
#else
    size_t max_size = 4096u;
#endif
    myclock::time_point start2 = myclock::now();
    for (unsigned i(0); i < 100u; ++i)
    {
      vec32 av = make_random_vnat_of_size(max_size, generator);
      vec32 bv = make_random_vnat_of_size(max_size, generator);

      BNat a(std::move(av));
      BNat b(std::move(bv));
      BNat result(a * b);      // This allocates memory for the result.  Timing will reflect that
      if (result.num_word32() != 0u)
      {
        parity = parity ^ result.ls_word();
      }
    }

    myclock::time_point end2 = myclock::now();
    std::chrono::duration<double> time_span2 =
      std::chrono::duration_cast<std::chrono::duration<double>>(end2 - start2);

    std::cout << "parity=" << std::hex << parity << std::dec;
    std::cout << " elapsed seconds for multiply=" << time_span2.count() << std::endl;
    ++num_passed;
    std::cout << "passed test " << test_name.c_str() << std::endl;
  }


  std::cout << "passed " << num_passed << " tests" << std::endl;
  std::cout << "failed " << num_failed << " tests" << std::endl;
  Sleep(5 * 1000);
  return num_failed;
}




