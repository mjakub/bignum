#include "pch.h"
#include "integer.h"
#include <iostream>
#include <limits>
#include <iso646.h>   // so "not" 'or" and "and" work, VSC++ bug
#include <assert.h>
#include <functional>  // std::plus
#include <numeric>     // inner_product

using vec32 = std::vector < uint32_t >;
using Uint128 = std::pair<uint64_t, uint64_t>;  // least and most significant 64 bits

namespace Big_numbers {
  // helper functions
#if 0
  void shift_right_32(Uint128& accum)
  {
    accum.first = (accum.first >> 32u) + ((accum.second & 0xFFFF'FFFFu) << 32u);
    accum.second >>= 32u;
  }
#endif

  constexpr Uint128 shr_32(const Uint128 accum) noexcept
  {
    return Uint128(
      (accum.first >> 32u) + ((accum.second & 0xFFFF'FFFFu) << 32u),
      (accum.second >> 32u)
    );
  }

  bool test_nonzero(const std::vector<uint32_t>& n) noexcept
  {
    return (n.size() != 0u);
  }

  bool test_zero(const std::vector<uint32_t>& n) noexcept
  {
    return (n.size() == 0u);
  }

  bool less_than(const std::vector<uint32_t>& lhs, const std::vector<uint32_t>& rhs) noexcept
  {
    bool is_less(false);
    const size_t lsize = lhs.size();
    const size_t rsize = rhs.size();
    if (lsize > rsize)
    {
      return is_less; // false
    }
    if (lsize < rsize)
    {
      is_less = true;
      return is_less;
    }

    // OK, same size,  do a reverse-iteration
    // so that MSB are considered first.
    auto riter = std::crbegin(rhs);
    auto rend = std::crend(rhs);
    auto iter = std::crbegin(lhs);
    while (riter != rend)
    {
      if (*riter == *iter)
      {
        ++iter;
        ++riter;
        continue;  // advance to next words
      }

      is_less = (*iter < *riter);
      break;
    }
    return is_less;
  }

  vec32 add_word(const vec32& n, const uint32_t delta)
  {
    vec32 result;
    if (delta == 0)
    {
      result = n;
      return result;
    }
    const size_t n_size = n.size();
    result.reserve(n_size + 1);

    uint32_t carry(delta);
    for (size_t i(0); i < n_size; ++i)
    {
      uint32_t sum = n[i] + carry;
      carry = (sum < n[i]);  // because true converted to 1 or 0
      result.push_back(sum);
    }
    if (carry != 0)
    {
      result.push_back(carry);
    }

    return result;
  }

  void increment_by_word(std::vector<uint32_t>& n, const uint32_t delta)
  {
    uint32_t carry(delta);
    size_t i(0u);
    const size_t nsize = n.size();
    while (carry != 0u)
    {
      if (i < nsize)
      {
        uint32_t prev = n[i];
        n[i] += carry;
        carry = (n[i] < prev);  // because true converted to 1 or 0
        ++i;
      }
      else
      {
        n.push_back(carry);  // carry goes into a new word
      }
    }
  }

  void increment_at_index_by_word(std::vector<uint32_t>& n, const size_t index, const uint32_t delta)
  {
    uint32_t carry(delta);
    size_t i(index);
    const size_t nsize = n.size();
    while (carry != 0u)
    {
      if (i < nsize)
      {
        uint32_t prev = n[i];
        n[i] += carry;
        carry = (n[i] < prev);  // because true converted to 1 or 0
        ++i;
      }
      else
      {
        n.push_back(carry);  // carry goes into a new word
      }
    }
  }


  void increment_at_index_by_dword(std::vector<uint32_t>& n, const size_t index, const uint64_t delta)
  {
    // add delta in
    const uint32_t LSW = 0xffff'ffffu;

    uint64_t carry(delta);
    //uint32_t carryLSW = delta & LSW;
    //uint32_t carryMSW = delta >> 32u;
    size_t i(index);
    const size_t nsize = n.size();

    for (; (i < nsize) && (carry != 0); ++i)
    {
      uint32_t carryLSW = carry & LSW;
      uint32_t carryMSW = (carry >> 32u);
      uint64_t sum(uint64_t(n[i]) + uint64_t(carryLSW));
      n[i] = sum & LSW;
      carry = (sum >> 32u) + carryMSW;
    }

    if ((i == nsize) && (carry != 0))
    {
      const uint32_t carryLSW(carry & LSW);
      n.push_back(carryLSW);  // carry goes into a new word
      if (carryLSW == 0)   // since carry != 0 and carryLSW==0, carryMSW is nonzero.
      {
        n.push_back(carry >> 32u); // push the nonzero MSW
      }
    }

  }



  static void subtract_dword_from_MSDW(vec32& n, const uint64_t delta) noexcept
  {
    const size_t index = n.size() - 2u;
    const uint32_t LSW = 0xffff'ffffu;
    uint32_t prev = n[index];
    n[index] -= uint32_t(delta & LSW);  // subtract
    uint32_t borrow = n[index] > prev;   // rollover detection
    uint32_t upper_word = uint32_t(delta >> 32u);
    n[index + 1] -= (upper_word + borrow);

    // remove zero MSW
    while ((n.size() != 0u) && (n.back() == 0))
    {
      n.pop_back();
    }
  }



  // precondition: a.size() <= b.size()
  vec32 add_ordered(const vec32& a, const vec32& b)
  {
    vec32 result;
    uint32_t carry(0u);
    size_t i(0u);
    for (; i < a.size(); ++i)
    {
      uint32_t s(carry + a[i] + b[i]); // this could wrap, test on next line for wrap
      carry = (s < a[i]) || (s < b[i]); // detect rollover
      result.push_back(s);
    }

    // now add the carry into the remaining bits of b
    for (; i < b.size(); ++i)
    {
      uint32_t s(carry + b[i]);
      carry = (s < b[i]);  // rollover
      result.push_back(s);
    }
    if (0 != carry)
    {
      result.push_back(carry);
    }
    return result;
  }


  std::vector<uint32_t> add_vec32(const std::vector<uint32_t>& a, const std::vector<uint32_t>&b)
  {
    return less_than(a, b) ? add_ordered(a, b) : add_ordered(b, a);
  }

  void scale_by_word(vec32& v, const uint32_t word)
  {
    if (0u == word)
    {
      v.clear();
      return;
    }

    if (1u == word)
    {
      return;
    }

    const size_t vsize(v.size());
    const uint64_t w(word);
    const uint32_t LSW_mask(0xffff'ffffu);
    uint64_t carry(0u);
    for (uint32_t i(0u); i < vsize; ++i)
    {
      const uint64_t a = w * uint64_t(v[i]) + carry;
      carry = a >> 32u;
      v[i] = (a & LSW_mask);
    }
    if (carry != 0)
    {
      v.push_back(uint32_t(carry));
    }
  }




  bool less_than_at_index(const vec32& lhs, size_t index, const vec32& rhs) noexcept
  {
    bool is_less(false);

    // first, see if size of the vectors gives an answer
    const size_t lsize = lhs.size();
    const size_t rsize = rhs.size();
    if (lsize > (rsize + index))
    {
      return is_less; // false
    }
    if (lsize < (rsize + index))
    {
      is_less = true;
      return is_less;
    }

    // OK, same size,  do a reverse-iteration
    // so that MSB are considered first.
    auto riter = std::crbegin(rhs);
    auto rend = std::crend(rhs);
    auto iter = std::crbegin(lhs);

    while (riter != rend)
    {
      if (*riter == *iter)
      {
        ++riter;
        ++iter;
        continue;
      }

      is_less = (*iter < *riter);
      break;
    }
    return is_less;
  }

  bool greater_than_at_index(const vec32& lhs, size_t index, const vec32& rhs) noexcept
  {
    bool is_greater(false);
    const size_t lsize = lhs.size();
    const size_t rsize = rhs.size();
    if (lsize < (rsize + index))
    {
      return is_greater; // false
    }
    if (lsize > (rsize + index))
    {
      is_greater = true;
      return is_greater;
    }

    // OK, same size,  do a reverse-iteration
    // so that MSW are considered first.
    auto riter = std::crbegin(rhs);
    auto rend = std::crend(rhs);
    auto iter = std::crbegin(lhs);

    while (riter != rend)
    {
      if (*riter == *iter)
      {
        ++riter;
        ++iter;
        continue;
      }
      is_greater = (*iter > *riter);
      break;
    }
    return is_greater;
  }

  bool greater_than(const vec32& lhs, const vec32& rhs) noexcept
  {
    bool is_greater(false);
    const size_t lsize = lhs.size();
    const size_t rsize = rhs.size();
    if (lsize > rsize)
    {
      is_greater = true;
      return is_greater;
    }
    if (lsize < rsize)
    {
      return is_greater; // false
    }

    // OK, same size,  do a reverse-iteration
    // so that MSW are considered first.
    auto riter = std::crbegin(rhs);
    auto rend = std::crend(rhs);
    auto iter = std::crbegin(lhs);

    while (riter != rend)
    {
      if (*riter == *iter)
      {
        ++riter;
        ++iter;
        continue;
      }

      is_greater = (*iter > *riter);
      break;
    }
    return is_greater;
  }

#if 0
  bool twice_greater(const vec32& lhs, const vec32& rhs)  //  returns 2*lhs > rhs
  {
    bool is_greater(false); // RVO

    const size_t lsize = lhs.size();
    const size_t rsize = rhs.size();

    // lsize > rsize => lhs > rhs  => 2*lhs > rhs
    if (lsize > rsize)
    {
      is_greater = true;
      return is_greater;
    }
    // lsize < (rsize - 1u) => lhs < 2**32 * rhs => lhs < rhs
    if (lsize < (rsize - 1u))
    {
      return is_greater; // false
    }
    if ((lsize == (rsize - 1)) && (rhs.back() > 1u))
    {
      return is_greater; // false
    }


    // remaining possibilities: 
    // lsize = rsize  (both vectors same length)
    // lsize = rsize-1 and rhs MSW is 1

    // in first case, the MSW of 2*lhs = shift left each word of lhs, shifting in MSbit from the next least-significant word
    // in second case, shift right each word of lhs, shifting in LSbit from the next most-significant word.
    auto riter = std::crbegin(rhs);
    auto rend = std::crend(rhs);
    auto liter = std::crbegin(lhs);
    auto lend = std::crend(lhs);
    if (lsize == rsize)
    {
      if ((lhs.back() & 0x8000'0000) != 0u)  // twice lhs will have a size > rsize, so 2*lhs > rhs.
      {
        is_greater = true;
        return is_greater;
      }
    }
    else
    {
      // lsize = rsize-1 and rhs MSW is 1
      if (0 == (lhs.back() & 0x8000'0000))
      {
        return is_greater;  // 2*lhs has size less than rsize, so 2*lhs < rhs   
      }
      // can start scan at next word of rhs, since 2*lhs and rhs both have MSW=1.
      ++riter;
    }
    while (riter != rend)
    {
      uint32_t previous_word(0);
      if ((liter + 1u) != lend)
      {
        previous_word = *(liter + 1u);
      }
      uint32_t current_word = (*liter) & 0x7fff'ffff;
      uint32_t twicelhs_word = (current_word << 1u) + (previous_word >> 31u);
      if (*riter == twicelhs_word)
      {
        ++riter;
        ++liter;
      }
      else
      {
        is_greater = (twicelhs_word > *riter);
        break;
      }
    }

    return is_greater;
  }
#endif

#if 0
  // precondition:   not less_than_at_index(v, index, delta)
  // also, does not normalize v after subtracting
  static void decrement_at_index_by_word(vec32& v, size_t index, uint32_t delta)
  {
    // subtract a word at a time, checking for borrow
    const size_t vsize = v.size();

    for (size_t i(index); (i < vsize) && (delta != 0); ++i)
    {
      const uint32_t prev = v[i];
      const uint32_t newv = prev - delta;
      v[i] = newv;
      delta = (newv > prev);  // check for underflow
    }
  }
#endif


  static void decrement_by_word(vec32& v, uint32_t delta)
  {
    // subtract a word at a time, checking for borrow
    const size_t vsize = v.size();

    for (size_t i(0); (i < vsize) && (delta != 0); ++i)
    {
      const uint32_t prev = v[i];
      const uint32_t newv = prev - delta;
      v[i] = newv;
      delta = (newv > prev);  // check for underflow, delta now used as a borrow
    }
    // remove MSW zeros
    while ((v.size() != 0) && (v.back() == 0))
    {
      v.pop_back();
    }
  }

  void decrement_at_index(vec32& v, size_t index, const vec32& delta)
  {
    // subtract a word at a time, checking for borrow
    const size_t vsize = v.size();
    const size_t dsize = delta.size();
    if (index >= vsize)
    {
      return;  // can not subtract here, nothing to see
    }

    bool borrow(false);
    size_t di(0);
    uint32_t delt(0);
    for (size_t i(index); i < vsize; ++i, ++di)
    {
      if (di < dsize)
      {
        delt = delta[di];
      }
      else
      {
        if (not borrow)
        {
          break;  // all done adding in delta and no borrow either
        }
        delt = 0;
      }
      if ((delt != 0) || borrow)
      {
        const uint32_t prev = v[i];
        const uint32_t newv = prev - (delt + borrow);
        borrow = (newv >= prev);  // check for underflow
        v[i] = newv;
      }
    }

    // may have left zeros at back of vector
    // erase MS zeros
    while ((v.size() != 0) && (v.back() == 0))
    {
      v.pop_back();
    }

  }


  void decrement_by(vec32& v, const vec32& delta)
  {
    // subtract a word at a time, checking for borrow
    bool borrow(false);
    const size_t vsize = v.size();
    const size_t dsize = delta.size();
    if (dsize == 0)
    {
      return;
    }

    if (dsize > vsize)
    {
#ifdef _DEBUG
      assert(false);
#endif
      return;
    }

    if (dsize == 1u)
    {
      decrement_by_word(v, delta[0]);
      return;
    }

    uint32_t delt(0);
    for (size_t i(0); i < vsize; ++i)
    {
      if (i < dsize)
      {
        delt = delta[i];
      }
      else
      {
        if (not borrow)
        {
          break;  // all done subtracting words of delta, and no borrow either
        }
        delt = 0;
      }
      if (delt != 0 || borrow)
      {
        const uint32_t prev = v[i];
        const uint32_t newv = prev - (delt + borrow);
        // special case: delta+borrow could sum to 0 if delt=0xffff'ffff, borrow=true
        // to handle this case, the test below is ">=" instead of ">".
        borrow = (newv >= prev);  // check for underflow
        v[i] = newv;
      }
    }

    // erase MS zeros
    while ((v.size() != 0) && (v.back() == 0))
    {
      v.pop_back();
    }
  }

  Nat add(const Nat& a, const Nat& b)
  {
    const size_t a_size = a.num.d.size();
    const size_t b_size = b.num.d.size();

    vec32 result = (a_size < b_size) ? add_ordered(a.num.d, b.num.d) : add_ordered(b.num.d, a.num.d);
    return Nat(result);

  }  // end add()



  vec32 mul_vec_by_word(const std::vector<uint32_t>& a, const uint32_t b)
  {
    vec32 result;

    if (b == 0u)
    {
      return result;
    }

    if (b == 1u)
    {
      result = a;
      return result;
    }

    const uint64_t LSW = 0xffff'ffffu;  // least significant word of a double word

    size_t asize = a.size();
    result.reserve(asize + 1u); // reserve extra word in case of overflow
    uint32_t accum(0);
    const uint64_t bb(b);

    for (size_t i(0); i < asize; ++i)
    {
      // each pass, calculate into the accumulator and write out one word from accum0
      const uint64_t prod = uint64_t(a[i]) * bb;
      const uint32_t prev = accum;
      accum += (prod & LSW);
      result.push_back(accum);
      accum = (accum < prev) + uint32_t(prod >> 32u);  // add in carry and product terms
    }
    // add in the remaining accumulator words, if overflow
    if (accum != 0)
    {
      result.push_back(accum);
    }

    return result;
  }





  // calculate r -= word_shift(d*q, index_of_work);
  // precondition: r >= word_shift(d*q, index_of_work);
  // this is similar to a "madd" multiply-and-accumulate, but does a subtraction
  void sub_product_at_index(vec32& r, const size_t index_of_work, const vec32& d, const uint32_t q)
  {
#ifdef _DEBUG
    vec32 test_rem(r);
    const vec32 product = mul_vec_by_word(d, q);
    decrement_at_index(test_rem, index_of_work, product);  // decrement remainder by product at index
#endif
                                 // combine the functionality of mul_vec_by_word and  decrement_at_index.
                                 // as each word is computed by mult_by_word, take that word and subtract it at the index.
    if (0 == q)
    {
      return;  // nothing to do
    }
    if (1 == q)
    {
      decrement_at_index(r, index_of_work, d);  // product to subtract is just d
      return;
    }
    const uint64_t LSW = 0xffff'ffffu;  // least significant word of a double word

    const size_t dsize = d.size();
    const size_t rsize = r.size();
    uint32_t accum(0);
    const uint64_t qq(q);

    bool borrow(false);
    uint32_t delt(0);
    size_t index(index_of_work);
    for (size_t i(0); i < dsize; ++i, ++index)
    {
      // each pass, calculate a word of product into accum
      const uint64_t prod = uint64_t(d[i]) * qq;
      const uint32_t prev = accum;
      accum += (prod & LSW);

      // accum has word to subtract from index=(index_of_work + i)
      //size_t index = index_of_work + i;
      const uint32_t adjust = accum + borrow;
      //if (adjust)
      {
        const uint32_t prev = r[index];
        const uint32_t newv = prev - adjust;
        borrow = (newv >= prev);  // check for underflow
        r[index] = newv;
      }

      accum = (accum < prev) + uint32_t(prod >> 32u);  // add in carry and product terms
    }

    // if overflow, there might be one more accum word to subtract
    const uint32_t adjust = accum + borrow;
    if (adjust)
    {
      const uint32_t prev = r[index];
      const uint32_t newv = prev - adjust;
      borrow = (newv >= prev);  // check for underflow
      r[index] = newv;
      ++index;
    }

    // keep subtracting borrows till no more borrows.  Worst case: {0,0,0,0,0,1} - 1 
    while (borrow)
    {
#ifdef _DEBUG
      if (index >= rsize)
      {
        assert(false);
        break;
      }
#endif
      // need to subtract 1
      // only way a borrow is needed is if value is 0
      borrow = (r[index] == 0);
      --r[index];
      ++index;
    }

    // erase MS zeros
    while ((r.size() != 0) && (r.back() == 0))
    {
      r.pop_back();
    }


#ifdef _DEBUG
    assert(test_rem == r);
#endif
  }






  // precondition:  a.size() <= b.size()
  vec32 mul_ordered_old_fashioned(const vec32& a, const vec32& b)
  {
    vec32 result;
    if (test_zero(a) || test_zero(b))
    {
      return result;
    }

    if (1u == a.size())
    {
      result = mul_vec_by_word(b, a[0]);
      return result;
    }

    const size_t num_words(a.size() + b.size());
    result = vec32(num_words, 0u);
    const uint32_t LSW_mask(0xffff'ffffu);

    // standard elementary-school multiply, but inner loop traverses shorter number in the hopes it fits into cache.
    for (size_t j(0); j < b.size(); ++j)
    {
      uint64_t carry(0);
      const uint64_t bj(b[j]);
      for (size_t i(0); i < a.size(); ++i)
      {
        const uint64_t ai(a[i]);
        const uint64_t prod = bj*ai;

        size_t k = i + j;
        uint64_t tk(result[k]);
        uint64_t sum = tk + prod + carry;
        carry = (sum < tk) || (sum < prod);
        result[k] = uint32_t(sum & LSW_mask);
        carry += (sum >> 32u);
      }
      if (0 != carry)
      {
        result[a.size() + j] = uint32_t(carry & LSW_mask);
      }
    } // outer loop

      // pop off the last result if zero, want the TOS to be nonzero (MSW)
    if (0 == result[num_words - 1u])
    {
      result.pop_back();
    }
    return result;
  }

  vec32 mul_old_fashioned(const vec32& a, const vec32& b)
  {
    return (a.size() <= b.size())
      ? mul_ordered_old_fashioned(a, b)
      : mul_ordered_old_fashioned(b, a);
  }

#if 0
  // Takes dot product [a[start]*b[k-start] + a[start+1]*b[k-start-1] + ... a[end-1]*b[k-start-(end-1)]
  // and stores it into a 128-bit accumulator.
  // So ap can be a pointer to a[start], bp can be a pointer to b[k-start], n=end-start, and dot with ++a --b for end-start iterations
  Uint128 convolve2c(vec32::const_iterator ap, vec32::const_iterator aend, vec32::const_iterator bp, const Uint128 accum) noexcept
  {
    Uint128 acc = accum;
    while (ap != aend)
    {
      // add prod and acc, careful to detect carries
      const uint64_t sum = acc.first + (uint64_t(*ap) * uint64_t(*bp));
      //std::cout << "convolve2c sum " << acc.first << " + "  << (*ap) << " * " << (*bp) << " = " << sum << std::endl;
      acc.second += (sum < acc.first);  // add 1 if overflow
      acc.first = sum;
      ++ap;
      --bp;
    }
    return acc;
  }
#endif


  auto sum_for_conv = [](const Uint128 accum, const uint64_t right) noexcept -> Uint128
  {
    const uint64_t sum = accum.first + right;
    return Uint128(sum, accum.second + (sum < right));  //adds in 1 if a carry happened
  };

  auto mult_to_64 = [](const uint32_t left, const uint32_t right)  noexcept -> uint64_t
  {
    return uint64_t(left) * uint64_t(right);
  };

  Uint128 convolve(vec32::const_iterator start, vec32::const_iterator end, std::reverse_iterator< vec32::const_iterator> back, const Uint128 accum) noexcept
  {
    return std::inner_product(start, end, back, accum, sum_for_conv, mult_to_64);
  }

  // precondition: a.size() <= b.size()
  vec32 mul_ordered(const vec32& a, const vec32& b)
  {
    vec32 result = {};

    //std::cout << "mul_ordered a=" << a << std::endl;
    //std::cout << "mul_ordered b=" << b << std::endl;

    const size_t a_size(a.size());
    const size_t b_size(b.size());

    if ((0 == a_size) || (0 == b_size))
    {
      return result;
    }

#ifdef _DEBUG
    assert(a_size <= b_size);
#endif
    const size_t b_size_m1(b_size - 1u);
    const size_t num_words(a_size + b_size);
    const size_t num_words_m1(num_words - 1u);

    if (1u == a.size())
    {
      result = mul_vec_by_word(b, a[0]);
      //std::cout << "mul_ordered result of mul_vec_by_word=" << result << std::endl;

#ifdef _DEBUG
      assert((0 == result.size()) || (result.back() != 0));  // unless n is zero, the MSW is nonzero 
#endif
      return result;
    }

    result.reserve(num_words);

    Uint128 accum(0, 0);
#ifdef _DEBUG
    assert(accum.first == 0);
    assert(accum.second == 0);
#endif
    const uint64_t LSW_mask(0xffff'ffffu);
    const vec32::const_iterator abegin = a.cbegin();
    const vec32::const_iterator aend = a.cend();
    const vec32::const_iterator bbegin = b.cbegin();
    const vec32::const_iterator bend = b.cend();
    const std::reverse_iterator<vec32::const_iterator> rev_a(aend);  // will start at a.back() 
    const std::reverse_iterator<vec32::const_iterator> rev_b(bend);  // will start at b.back()

                                     // These three cases have to do with whether the k-convolution 
                                     // falls in ranges
                                     // 0 <= k < a_size
                                     // a_size <= k < b_size
                                     // b_size <= k < (asize + b-1)

    std::reverse_iterator<vec32::const_iterator> b_iter(bbegin + 1); // starts at bbegin, goes backwards
    size_t k(0);
    for (auto aiter(abegin); k < a_size; ++aiter, ++k)
    {
      std::reverse_iterator<vec32::const_iterator> b_iter(bbegin + 1 + k); // points to b[0], b[1],...b[a_size]
      //const Uint128 temp = std::inner_product(abegin, abegin + k + 1, b_iter, accum, sum_for_conv, mult_to_64);
      const Uint128 temp = convolve(abegin, abegin + k + 1, b_iter, accum);
      result.push_back(temp.first & LSW_mask);
      accum = shr_32(temp);
    }

    // for k in the range a_size <= k < b_size,   range length is b_size-asize
    //std::reverse_iterator<vec32::const_iterator> biter(bbegin + (a_size + 1));  // will start pointing to b[a_size-1], 
    // Runs through b[bsize-1], b[bsize1-1],... b[bsize - (bsize-asize)] = b[asize]
    //const std::reverse_iterator<vec32::const_iterator> biter_end(bbegin + a_size);  // pointer to b[asize-1]
    for (; k < b_size; ++k)
    {
      std::reverse_iterator<vec32::const_iterator> b_iter(bbegin + (1 + k)); // points to b[a_size], b[a_size+1],...b[b_size-1]
      //const Uint128 temp = std::inner_product(abegin, aend, b_iter, accum, sum_for_conv, mult_to_64);
      const Uint128 temp = convolve(abegin, aend, b_iter, accum);
      result.push_back(temp.first & LSW_mask);
      accum = shr_32(temp);
    }

    // for k in the range b_size <= k < (num_words-1), range size = num_words-1-b_size = a_size-1
    for (auto aiter(abegin + 1); aiter != aend; ++aiter)
    {
      const Uint128 temp = convolve(aiter, aend, rev_b, accum);
      //const Uint128 temp = std::inner_product(aiter, aend, rev_b, accum, sum_for_conv, mult_to_64);
      result.push_back(temp.first & LSW_mask);
      accum = shr_32(temp);
    }

#ifdef _DEBUG
    assert((0 == result.size()) || (result.back() != 0));  // unless n is zero, the MSW is nonzero 
#endif

                                 // the position of the most signifcant word isn't known until now
                                 // for example 11*11 = 0121, don't want to push that 0, just 3 digit result
                                 // but         99*99 = 9801, do want to push that last digit, a 4 digit result
    while ( (accum.first != 0) || (accum.second != 0) )
    {
      result.push_back(accum.first & LSW_mask);
      accum = shr_32(accum);
    }

#ifdef _DEBUG
      assert((result.back() != 0) || (0 == result.size()));  // unless n is zero, the MSW is nonzero 
#endif
    return result;
  }  // end mul_ordered

  vec32 mul_vec32(const vec32& a, const vec32& b)
  {
    return a.size() < b.size()
      ? mul_ordered(a, b)
      : mul_ordered(b, a);
  }

#if 0
  vec32 mul_Karatsuba(const vec32& a, const vec32& b)
  {
    // TODO make a version that uses vec32::const_iterator and vec32::const_iterator
    // When this works, turn into a template
#ifdef _DEBUG
    assert(a.size() <= b.size());
    assert(b.size() <= a.size() + 1u);
#endif

    // TBD write Karatsuba algorithm
    // A = a0  + a1 << N    (this is a word shift)
    // B = b0  + b1 << M
    // A*B = a0*b0                             // t0
    //     + (a0 * b1)<<M + (a1*b0)<<N       // t1
    //     + a1*b1 << (N+M)                  // t2
    //
    // the plan is to choose N and M to be equal, 
    // because in that case
    // t1= (a0*b1 + a1*b0) << N
    //   = t0 + t2 - (a0-a1)*(b0-b1)<<N which is just one multiply
    //
    // if A.size and B.size are both odd or both even,
    // then A.size == B.size
    // let N=M=floor(A.size/2)
    //
    // if A.size is even, and B.size odd = a.size+1
    // let N=M=floor(A.size/2);
    //
    // if A.size is odd and B.size is even
    // let N=M=ceil(A.size/2);
    // 
    //

    // to do in place:
    //
    // 1.  allocate space for result (A.size + B.size) = 2N or 2N+1 if A.size != B.size. 
    // 2.  figure the signs of a0-a1 and b0-b1
    //     store abs(a0-a1) in left N words of result, 
    //     store abs(b0-b1) in right N words of result (position 3N of result).
    // 3.  store abs(a0-a1)*(b0-b1) starting at Nth word of result
    //     use a recursive call to do the multiply
    // 4.  calcuate t0 lazily, for each word store in the Least significant N words
    //     (overwriting abs(a0-a1)). Since there are 2N words in t0, the last N words
    //     are accumulated into the existing words of abs(t1)
    // 5.  calculate t2 lazily, starting at the position 2N of the result and accumlating
    //     into upper Nwords of abs(t1), then assigning into the MSW N words of result,
    //     writing over the abs(b0-b1) that currently reside there.
    // 
    //     This algorithm assumes the existence of a Karatsuba generator.
    //     this means the multiply in step 3 is done by a generator,
    //     which is used in steps 4 and 5 for the accumulation.
    //     So, basically you have to be able to do a Karatsuba generator 
    //     in order to do an in-place non-generator multiply.
    return mul_ordered(a, b);
  }
#endif

  bool loop_invariant(const vec32& numerator, const uint32_t divisor, const vec32& quotient, const uint32_t remainder)
  {
    // predicate to assert numerator = quotient*divisor + remainder
    // Allows quotient to be de-normalized (can have MSW zero).

    // first, make a normalized version of the quotient.
    vec32 q = quotient;
    // remove any 0 MSWs left in the quotient
    while ((q.size() != 0) && (q.back() == 0))
    {
      q.pop_back();
    }
    vec32 prod = mul_vec_by_word(q, divisor);

    vec32 sum = add_word(prod, remainder);
    const bool return_val(sum == numerator);
    return return_val;
  }


  bool loop_invariant(const vec32& numerator, const vec32& divisor, const vec32& quotient, const vec32& remainder)
  {
    // predicate to assert numerator = quotient*divisor + remainder
    // Allows quotient to be de-normalized (can have MSW zero).

    // first, make a normalized version of the quotient.
    vec32 q = quotient;
    // remove any 0 MSWs left in the quotient
    while ((q.size() != 0) && (q.back() == 0))
    {
      q.pop_back();
    }
    vec32 prod = mul_vec32(q, divisor);

    const size_t r_size = remainder.size();
    const size_t p_size = prod.size();

    vec32 sum = add_vec32(prod, remainder);
    const bool return_val(sum == numerator);
    return return_val;
  }

  // div, divide n by d, returning a quotient and a remainder
  // satisfies n = quot * d + rem,   where rem < d, unless d==0, in which case rem=d=0
  std::pair<vec32, uint32_t> div(const vec32& n, const uint32_t d)
  {
    // allocate memory for the returned quotient, zero filling it.
    // It might be 1 word too large and may need a pop_back to preserve the invariant (quotient.back() != 0u)
    std::pair<vec32, uint32_t> quot_rem;

    vec32& ret_quotient = quot_rem.first;
    uint32_t& ret_remainder = quot_rem.second;

    ret_remainder = 0u;

    if (test_zero(n))
    {
#ifdef _DEBUG
      assert(loop_invariant(n, d, quot_rem.first, quot_rem.second));
#endif
      return quot_rem;
    }

    if (0u != d)
    {
      ret_quotient = vec32(n.size(), 0u);  // allocate enough space for the quotient, 0 initialize.
      vec32 remainder = n;   // copy the numerator

                   // at this point, have established the invariant
                   // n = quotient*d + remainder; 
                   // underestimate the quotient and keep subtracting until remainder < d
      size_t rsize = remainder.size();
      while (rsize > 1)
      {
        // whittle down the size of the remainder while preserving invariant
        // divide the high two words of the invariant by the divisor word
        // This will result in 1 or 2 words of quotient.
        const size_t rsizem1(rsize - 1u);
        const size_t rsizem2 = (rsizem1 - 1u);
        const uint64_t high_words = (uint64_t(remainder[rsizem1]) << 32) + uint64_t(remainder[rsizem2]);

        const uint64_t high_quot = high_words / d;
        const uint64_t high_prod = high_quot * d;
        // high_words = high_quot*d + r, where r<=d
        increment_at_index_by_dword(ret_quotient, rsizem2, high_quot);
        subtract_dword_from_MSDW(remainder, high_prod);      // decrease the remainder by same amount
        rsize = remainder.size();

#ifdef _DEBUG
        vec32 denom;
        denom.push_back(d);
        assert(loop_invariant(n, denom, ret_quotient, remainder));
#endif
      }

      // have reduced remainder to have size 1. Do 1 more divide if possible
      if (rsize == 0)
      {
        ret_remainder = 0;
      }
      else
      {
        if (remainder[0] >= d)
        {
          uint32_t quotient = remainder[0] / d;
          remainder[0] -= (d*quotient);
          increment_by_word(ret_quotient, quotient);
        }
        ret_remainder = remainder[0];
      }
    }

    // remove any 0 MSWs from the result
    while ((ret_quotient.size() != 0) && (ret_quotient.back() == 0))
    {
      ret_quotient.pop_back();
    }
#ifdef _DEBUG
    assert(loop_invariant(n, d, quot_rem.first, quot_rem.second));
#endif
    return quot_rem;
  }




  // div, divide n by d, returning a quotient and a remainder
  // satisfies n = quot * d + rem,   where rem < d, unless d==0, in which case rem=d=0
  std::pair< vec32, vec32 > div(const vec32& n, const vec32& d)
  {
    std::pair<vec32, vec32> result;
    const bool is_zero_quotient = less_than(n, d);
    const size_t nsize = n.size();
    const size_t dsize = d.size();

    if (dsize == 1)
    {
      // special case of 1-word divisor. Also handles divide-by-zero case.
      std::pair<vec32, uint32_t> temp = div(n, d[0]);
      result.first = temp.first;
      result.second = vec32(1, temp.second);
      //std::pair<vec32, vec32> result(temp.first, vec32(1, temp.second));
#ifdef _DEBUG
      assert(loop_invariant(n, d, result.first, result.second));
#endif
      return result;
    }

    const size_t quotient_num_words = is_zero_quotient ? size_t(1u) : (nsize - dsize + 1u);
    const size_t remainder_num_words = test_zero(d) ? 1u : (is_zero_quotient ? nsize : dsize);

    // allocate memory for the returned quotient, zero filling it.
    // It might be 1 word too large and may need a pop_back to preserve the invariant (quotient.back() != 0u)
    result.first = vec32(quotient_num_words, 0u);
    result.second = vec32(remainder_num_words, 0u);

    if (test_nonzero(d))
    {
      // some more meaningful names
      vec32& ret_quotient = result.first;
      vec32& ret_remainder = result.second;

      ret_remainder = n;   // copy the numerator

                 // at this point, have established the invariant
                 // n = quotient*d + remainder; 
                 // underestimate the quotient and keep subtracting until remainder < d
      size_t rsize = ret_remainder.size();
      const uint32_t d_MSW = d.back();  // get the MSW of the divisor
      const uint64_t d_MSDW = (uint64_t(d_MSW) << 32u) + d[dsize - 2];
#ifdef _DEBUG
      assert(loop_invariant(n, d, ret_quotient, ret_remainder));
#endif
      //size_t loopcount(0);
      while (not less_than(ret_remainder, d))   // ensures the (remainder < d) postcondition, same as while r>=d
      {
        //++loopcount;
#ifdef _DEBUG
        assert(rsize >= dsize);  // r >=d implies rsize >= dsize >= 2
#endif
                     // since the smallest rsize can be is two, that is the base case
                     // for exiting the loop.  Otherwise keep reducing remainder.
        if (ret_remainder.size() == 2u)   // A base case for exiting loop
        {
          // can do this last step exactly.
          const uint64_t rem_dword = (uint64_t(ret_remainder[1u]) << 32u) + ret_remainder[0];
          const uint64_t quot_dword = rem_dword / d_MSDW;
          // since d_MSDW is nonzero in MSW, this quotient really fits in 32 bits.
          const uint32_t quot_word = uint32_t(quot_dword);
          const uint64_t prod_dword = quot_dword * d_MSDW;
          increment_by_word(ret_quotient, quot_word);  // increase the quotient
          subtract_dword_from_MSDW(ret_remainder, prod_dword);  // decrease the remainder
#ifdef _DEBUG
          assert(loop_invariant(n, d, ret_quotient, ret_remainder));
#endif
          break;  //out of loop
        }

        const uint64_t rem_MSWs = (uint64_t(ret_remainder[rsize - 1u]) << 32) + uint64_t(ret_remainder[rsize - 2u]);

        if (d_MSDW < rem_MSWs)
        {
          // can use d_MSDW for trial divide, it will give at least a quotient 1 with nonzero remainder
          // because the MSW of div_word is nonzero, this results in a single-word quotient
          const uint64_t high_quotd = rem_MSWs / d_MSDW;
#ifdef _DEBUG
          assert(high_quotd < 0x1'0000'0000ull);
#endif
          const uint32_t high_quotw = uint32_t(high_quotd);
          const size_t index_of_work(rsize - dsize);  // note rsize>=dsize since remainder >=d
          sub_product_at_index(ret_remainder, index_of_work, d, high_quotw);
          increment_at_index_by_word(ret_quotient, index_of_work, high_quotw);  // increase the (quotient*d) value
#ifdef _DEBUG
          assert(loop_invariant(n, d, ret_quotient, ret_remainder));
#endif
        }
        else if (d_MSDW == rem_MSWs)
        {
          if (rsize == dsize)
          {
            // r is nearly equal to d, just subtract d from r and exit
            decrement_by(ret_remainder, d);
            const uint32_t one(1u);
            increment_by_word(ret_quotient, one);  // increase the (quotient*d) value
#ifdef _DEBUG
            assert(loop_invariant(n, d, ret_quotient, ret_remainder));
            assert(less_than(ret_remainder, d));
#endif
            break;
          }

          // quotient is darn nearly 1. use 0xffffffff as an estimated quotient, with a word shift (see index_of_work)
#ifdef _DEBUG
          assert(rsize > dsize);  // Since already handled rsize == dsize, it must be rsize > dsize.
#endif
          const size_t index_of_work = rsize - dsize - 1u;  // ok since rsize > dsize
          const uint32_t quot_w = 0xffff'ffffu;
          // proof that quotient is not too big:
          // Need to verify that r >= q*d
          // Since r and d have equal MSWs, this inequality holds:
          // r + 2**32(rs-2) > d * 2**32(rs-ds)  where rs=rsize, ds = dsize.
          // rearranging gives
          // r > d*(2**32(rs-ds)-k) + (k*d - 2**32(rs-2))   where choose k=2**32(rs-ds-1)
          // the last term is >=0 because d>=2**32(ds-1)  (the MSW of d is >=1)
          // dropping the (non-negative) last term gives
          // r > d*(2**32(rs-ds)-2**32(rs-ds-1))
          // r > d*q  where q = 0xffffffff * 2**32(rs-ds-1)
          // QED


          // ret_remainder -= word_shift( d*high_quotw, index_of_work)
          sub_product_at_index(ret_remainder, index_of_work, d, quot_w);
          //const vec32 product = mul_vec_by_word(d, quot_w);
          //decrement_at_index(ret_remainder, index_of_work, product);  // decrement remainder by product at index

          increment_at_index_by_word(ret_quotient, index_of_work, quot_w);  // increase the (quotient*d) value
#ifdef _DEBUG
          assert(loop_invariant(n, d, ret_quotient, ret_remainder));
#endif
        }
        else
        {
          // can use (d_MSW+1) for trial divide
          // proof that the quotient obtained fits in 32 bits:
          // since here,  d_MSDW > rem_MSWs
          // so           d_MSW*2**32 >= rem_MSWs
          //              (d_MSW+1)*2**32 > rem_MSWs
          // or           2**32 > rem_MSWs/(d_MSW+1)
          // QED
          const uint64_t high_quotd = rem_MSWs / (uint64_t(d_MSW) + 1u);
#ifdef _DEBUG
          assert(high_quotd < 0x1'0000'0000ull);
#endif
          const uint32_t high_quotw = uint32_t(high_quotd);
#ifdef _DEBUG
          assert(high_quotw != 0);
#endif
          const size_t index_of_work = rsize - dsize - 1u;

          // ret_remainder -= word_shift( d*high_quotw, index_of_work)
          sub_product_at_index(ret_remainder, index_of_work, d, high_quotw);
          //const vec32 product = mul_vec_by_word(d, high_quotw);
          //decrement_at_index(ret_remainder, index_of_work, product);  // decrement remainder by product at index
          increment_at_index_by_word(ret_quotient, index_of_work, high_quotw);  // increase the (quotient*d) value
#ifdef _DEBUG
          assert(loop_invariant(n, d, ret_quotient, ret_remainder));
#endif
        }
        rsize = ret_remainder.size();

      } // while

        // remove any 0 MSWs left in the quotient
      while ((ret_quotient.size() != 0) && (ret_quotient.back() == 0))
      {
        ret_quotient.pop_back();
      }
#ifdef _DEBUG
      assert(loop_invariant(n, d, ret_quotient, ret_remainder));
#endif
      //std::cout << " loopcount=" << loopcount << " sizeof n=" << nsize << " sizeof d=" << dsize << std::endl;


    }  // endif nonzero divisor
    return result;
  }



  // precondition: b.size() >= a.size()
  // postcondition:  b is modified to (a*b)
  void mul_ordered_in_place(const vec32& a, vec32& b)
  {
    if (1u == a.size())
    {
      scale_by_word(b, a[0]);
    }
    else
    {
      vec32 result = mul_ordered(a, b);
      b = result;  // replace contents!
    }
  }


  Nat mul(const Nat& a, const Nat& b)
  {
    // calculate convolution sum a[i]*b[j] for i+j = pos
    // the value goes in the "pos" position.
    // pos runs from 0 to a.size()+b.size()-2.
    // example ab*de = be at pos=0, ad+be at pos=1, ad at pos=2

    vec32 result;

    const size_t a_size = a.num.d.size();
    const size_t b_size = b.num.d.size();
    result = (a_size <= b_size) ? mul_ordered(a.num.d, b.num.d) : mul_ordered(b.num.d, a.num.d);
    return Nat(result);
  }


#if 0
  Nat operator+(const Nat& a, const Nat& b)
  {
    return add(a, b);
  }
#endif
  Nat operator*(const Nat& a, const Nat&b)
  {
    return mul(a, b);
  }


  std::pair<Nat, uint32_t> div(const Nat& n, uint32_t d)
  {
    const std::pair< vec32, uint32_t > temp = div(n.num.d, d);
    return std::pair <Nat, uint32_t>(std::move(temp.first), temp.second);
  }

  std::pair<Nat, Nat> div(const Nat& n, const Nat& d)
  {
    if (d.num.d.size() == 1u)
    {
      const std::pair< vec32, uint32_t > temp = div(n.num.d, d.num.d[0]);
      return std::pair <Nat, Nat>(std::move(temp.first), temp.second);
    }
    const std::pair< vec32, vec32 > temp = div(n.num.d, d.num.d);
    return std::pair<Nat, Nat>(std::move(temp.first), std::move(temp.second));
  }



  // increment (in-place) by a value
  void Nat_mut::increment_by(const Nat_mut& rhs)
  {
    vec32 result =
      (num.d.size() <= rhs.num.d.size())
      ? add_ordered(num.d, rhs.num.d)
      : add_ordered(rhs.num.d, num.d);
    num.d = result;
  }

  // multiply (in-place) by a value
  void Nat_mut::scale_by(const Nat_mut& rhs)
  {
    if (num.d.size() >= rhs.num.d.size())
    {
      mul_ordered_in_place(rhs.num.d, num.d);
    }
    else
    {
      vec32 result =
        (num.d.size() <= rhs.num.d.size())
        ? mul_ordered(num.d, rhs.num.d)
        : mul_ordered(rhs.num.d, num.d);
      num.d = result;
    }
  }



} // end namespace Big_numbers

std::ostream& operator <<(std::ostream& os, const vec32& n)
{
  auto end = n.crend();

  for (auto riter = n.crbegin(); riter != end; )
  {
    os << std::hex << *riter;
    ++riter;
    if (riter != end)
    {
      os << "'";
    }
  }

  return os;
}

std::ostream& operator <<(std::ostream& os, const Big_numbers::Nat& n)
{
  os << n.num.d;
  return os;
}



