// test_big_numbers.cpp : Defines the entry point for the console application.
//

//#include "stdafx.h"
#include "..\integer\integer.h"
#include <iostream>
#include <cstring>
#include <array>
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
    std::cout << "starting test_big_numbers, implementation has max num of uint32_t =" << test.max_size() << std::endl;
  }





  {
    const std::string test_name("multiply_by_word_fuzz_test");
    std::cout << "running " << test_name.c_str() << std::endl;
    // multiply psuedo-random numbers.
    // compare fast-cache to convultion multiply for consistency.
    // later do same test to compare times.
    bool success(true);
    std::minstd_rand0 generator(seed1);

    for (unsigned i(0); i < 100; ++i)
    {
      uint32_t  b = (generator() & 0xFFFF'FFFFu);
      const vec32 a = make_random_vnat_of_size(255u, generator);

      const vec32 result1 = Big_numbers::mul_vec32_by_word(a, b);
      const size_t asize = a.size();
      const size_t bsize = 1u;
      size_t max_size = (asize==0u) || (bsize==0u) ? 0u : asize+bsize;
      size_t min_size = (max_size != 0u) ? max_size - 1u : 0u;
      if ( (result1.size() < min_size) || (result1.size() > max_size))
      {
        success = false;
        std::cerr << "fail of " << test_name.c_str() 
          << " fuzz index=" << i 
          << " , asize=" << asize 
          << " bsize=" << bsize 
          << " result size=" << result1.size()
          << std::endl;
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
    }
  }


  {
    const std::string test_name("mul_small_numbers_performance_fuzz_test");
    std::cout << "running " << test_name.c_str() << std::endl;
    // multiply psuedo-random numbers.
    // compare fast-cache to convolution multiply for consistency.
    // later do same test to compare times.
    bool success(true);
    uint32_t parity(0);

    std::minstd_rand0 generator(seed1);

  #ifdef _DEBUG
    const size_t max_size(15u);
    const unsigned int num_iters(10'000u);
  #else
    const size_t max_size(0x15u);
    const unsigned int num_iters(100'000u);
  #endif
    std::vector<vec32> a;
    a.reserve(num_iters);
    std::vector<vec32> b;
    b.reserve(num_iters);

    for (size_t i(0); i < num_iters; ++i)
    {
      a.push_back(make_random_vnat_of_size(max_size, generator));
      b.push_back(make_random_vnat_of_size(max_size, generator));
    }

    myclock::time_point start2 = myclock::now();

    for (unsigned i(0); i < num_iters; ++i)
    {
      //std::cout << test_name.c_str() << " test index " << i << " of " << num_iters << std::endl;
      vec32 result1 = Big_numbers::mul_vec32(a[i], b[i]);
    }
    myclock::time_point end2 = myclock::now();
    std::chrono::duration<double> time_span2 =
    std::chrono::duration_cast<std::chrono::duration<double>>(end2 - start2);
    ++num_passed;

    std::cout << "passed test " << test_name.c_str()
      << " elapsed seconds=" << time_span2.count()
      << std::endl;
  }


  {
    const std::string test_name("mul_large_numbers_performance_fuzz_test");
    std::cout << "running " << test_name.c_str() << std::endl;
    // multiply psuedo-random numbers.
    // compare fast-cache to convolution multiply for consistency.
    // later do same test to compare times.
    bool success(true);
    uint32_t parity(0);

    std::minstd_rand0 generator(seed1);

#ifdef _DEBUG
    const size_t max_size(4000u);
    const unsigned int num_iters(10u);
#else
    const size_t max_size(0x10'000u);
    const unsigned int num_iters(10u);
#endif
    std::vector<vec32> a;
    a.reserve(num_iters);
    std::vector<vec32> b;
    b.reserve(num_iters);

    for (size_t i(0); i < num_iters; ++i)
    {
      a.push_back(make_random_vnat_of_size(max_size, generator));
      b.push_back(make_random_vnat_of_size(max_size, generator));
    }

    myclock::time_point start2 = myclock::now();

    for (unsigned i(0); i < num_iters; ++i)
    {
//#ifndef _DEBUG
      std::cout << test_name.c_str() << " test index " << i << " of " << num_iters
        << " sizes=" << a[i].size() << " " << b[i].size()
        << std::endl;
//#endif
      vec32 result1 = Big_numbers::mul_vec32(a[i], b[i]);
    }
    myclock::time_point end2 = myclock::now();
    std::chrono::duration<double> time_span2 =
    std::chrono::duration_cast<std::chrono::duration<double>>(end2 - start2);

    std::cout << 
    ++num_passed;
    std::cout << "passed test " << test_name.c_str() 
      << " elapsed seconds=" << time_span2.count() << std::endl;
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

      vec32 q(make_random_vnat_of_size(sizem, generator));  // 7 is a mask for sizes to allow, between 1 and 7 words in the vector
      vec32 d(make_random_nonzero_vnat_of_size(sizem, generator));
      vec32 r(make_random_nonzero_vnat_le(d, generator));
      vec32 n = Big_numbers::add_vec32(r, Big_numbers:: mul_vec32(q, d));  //n = (q*d) + r;

      auto result = Big_numbers::div_vec32(n, d);

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





  std::cout << "passed " << num_passed << " tests" << std::endl;
  std::cout << "failed " << num_failed << " tests" << std::endl;
  return num_failed;
}
