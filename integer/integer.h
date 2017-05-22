#pragma once
#ifndef BIG_INTEGERS_H
#define BIG_INTEGERS_H

#include "arithmetic_algorithm.h"

#include <vector>
#include <cstdint>
#include <ostream>
#include <utility>
#include <iso646.h>   // so "not" 'or" and "and" work, VSC++ bug
//#include <iostream> // for debug cout <<

namespace Big_numbers {

  // utility functions assuming natural numbers represented as std::vector<uint32>
  // convention that empty vector is zero, and noempty vectors have last word the nonzero MSW 
  bool test_nonzero(const std::vector<uint32_t>& n) noexcept;
  bool test_zero(const std::vector<uint32_t>& n) noexcept;
  bool less_than(const std::vector<uint32_t>& lhs, const std::vector<uint32_t>& rhs) noexcept;
  bool greater_than(const std::vector<uint32_t>& lhs, const std::vector<uint32_t>& rhs) noexcept;

  inline bool less_than_or_equal(const std::vector<uint32_t>& lhs, const std::vector<uint32_t>& rhs) noexcept
  {
    return not greater_than(lhs, rhs);
  }

  std::vector<uint32_t> add_vec32(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b);
  std::vector<uint32_t> add_vec32_and_word(const std::vector<uint32_t>& n, const uint32_t delta);
  void increment_by_word(std::vector<uint32_t>& n, const uint32_t delta);

  // symmetric difference of two naturals (in vec32 format).
  // The second value of the result has the value less_than_or_equal(a,b)
  std::pair< std::vector<uint32_t>, bool> symdiff_vec32(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b);

  std::vector<uint32_t> mul_vec32(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b);

  // div takes a numerator n and a divisor d, returns a pair (quotient, remainder).
  std::pair< std::vector<uint32_t>, std::vector<uint32_t> > div_vec32(const std::vector<uint32_t>& n, const std::vector<uint32_t>& d);


  std::vector<uint32_t> mul_vec32_by_word(const std::vector<uint32_t>& a, const uint32_t b);

  struct To_infinity {};  // phantom type for constructing numbers with +-infinity




  // Base type for integral number types
  // By having one structure for many types, it makes it efficient to 
  // upcast, eg. interpret n:Nat as an n:Int, or a n:Nat as a N:Nat_inf
  struct Integral_number {
    /* invariant1:
    * d.size() == 0 iff the number represents 0.
    *  That is to say, zero number has an internal vector of size 0.
    *  d.size() == 0 means d.back() will have undefined behavior.
    *
    * invariant2:
    * d.back() is undefined for zero, and nonzero for other numbers
    *  because back() is the most significant word.
    *  Note that back() is d[d.size()-1], so this also
    *  says that the size is minimal (no 0 padding at end).
    *  There can be some padding of the form size < capacity.
    *
    * Semantics:
    *  For Nat type without infinities (infinite=false),
    *  the abs(n) = Sum(i=0..(size-1))  d[i] * 2**i
    *          n  = -abs(n)  if n.negative
    *          n  = abs(n)   if not n.negative
    *  Semantics of infinite not covered here,
    *    but should roughly follow IEEE 754 conventions (messy!)
    *
    *
    *
    * The choice is to put the MSB at the end of the vector.
    * This ordering causes  << and >> to be O(n) operations, vs. O(1)
    * A Nat_algebraic that uses the other ordering and uses singly-linked
    * lists would be a better choice if you need shifts to be O(1).
    * The cache-unfriendliness of that will have a performance cost, however.
    *
    * The chosen ordering favors algorithms that work from
    * LSB to MSB, which allows for easy propogation of carries.
    *
    * 32u*d.size() gives a good estimate of the number of bits, meaning
    *  32u*(dsize()-1) < numbits <= 32u*(dsize()) for nonzero numbers.
    *
    */

    std::vector < std::uint32_t > d;   // [0] is LSBs, last is MSBs. Empty for zero
    const bool negative = { false };
    const bool infinite = { false };
    const bool NaN = { false };


    // special values:  +inf,-inf, -0, NaN
    // +inf = some number larger than all others
    // -inf = some number smaller than all others
    // -0 weird, dont want -0=0 because then  inf = 1/0 = 1/-0 = -inf, so observably 0 != -0.  
    //    more questions: -0 < 0?  0 + -0 = ??? 0*-0 = -0? 0 not idempotent under multiply?
    //    why not just 1/inf and 1/-inf to 0? Loose the 1-1 correspondence of inverting nonzeros
    // NaN = not-a-number, can think of it as being a value, but not known, so incomparable
    // can define "compare" --> LT, EQ, GT, NC, so compare(NaN, x) = NC for all x.
    // could have this:
    // try
    // {
    //   if( lt(a,b) )   <--  lt throws "range_error" if a and b are not comparable
    //   {
    //     tstuff;
    //   }else{
    //     fstuff;
    //   }
    // }
    // catch(exception& e)
    // {
    //   // handle incomparables here, like NaN
    // }
    //    predicates:   lt, le, gt, ge, eq, nc, neq, cmp


    // Note: NaN = (+inf -  +inf) or (+inf + -inf) or 0/0  (and its signed variations)
    // Note: +inf = n/0 for n>0, -inf = m/0 for m<0
    // Note: +inf + +inf = +inf, -inf + -inf = -inf


    Integral_number() {}       // makes a nan

    Integral_number(const Integral_number& rhs)
      : d(rhs.d)
      , negative(rhs.negative)
      , infinite(rhs.infinite)
      , NaN(rhs.NaN)
    {}

    Integral_number(Integral_number&& rhs)
      : d(std::move(rhs.d))
      , negative(rhs.negative)
      , infinite(rhs.infinite)
      , NaN(rhs.NaN)
    {
    }

    explicit Integral_number(const std::vector<uint32_t>& value)
      : d(value)
      , negative(false)
      , infinite(false)
      , NaN(false)
    {}

    explicit Integral_number(const std::vector<uint32_t>&& value)
      : d(std::move(value))


    {
      //std::cout << "Integral_number move constructor called, size=" << d.size() << std::endl;
    }

    Integral_number(const std::vector<uint32_t>& value, const bool negate)
      : d(value)
      , negative(negate)
      ,infinite(false)
      ,NaN(false)
    {}

    Integral_number(const std::vector<uint32_t>&& value, const bool negate)
      : d(value), negative(negate), infinite(false) {}


    explicit Integral_number(const uint32_t w)
    {
      if (w != 0)
      {
        d = std::vector<uint32_t>(1u, w);
      }
    }

    explicit Integral_number(uint32_t first, size_t capacity)
    {
      d.reserve(capacity);
      d.push_back(first);
    }

    // constructor for +- infinity
    Integral_number(const bool negate, const To_infinity)
      : negative(negate), infinite(true) {}


  };


  // There are many ways to define a natural number
  // Here I give an interface that is reasonable
  // for an in-memory Nat values on a computer. 
  // (Not really infinite, you run out of memory eventually)
  //
  //   There is a default constructor that produces 0
  //   There is a constructor that takes a uint32
  //   There are equality and comparison predicates
  //   The binary functions + and * are defined (outside of the class)
  //   There is a div(const Nat& n, uint32_t d) function that returns (quotient, uint32_t remainder),
  //     but returns (0,0) if d==0
  //   There is a div(const Nat& n, const Nat& d) function that returns (quotient, remainder),
  //     but returns (0,0) if d==0
  //   size_t num_word32() function that returns the number of words (digits) in the number
  //   There is a function to get word[i], i in half-open interval [0, n.num_word32() )

  // in general, any operation that creates a Nat can throw a std::bad_alloc or std::length_error exception
  // If exception handling is disabled by compiler, the program will terminate due to the exception
  // This behavior is likely what you want when running out of memory.
  struct Nat {

    Nat() : num(uint32_t(0)) {}

    explicit Nat(uint32_t first, size_t capacity) : num(first, capacity) {}

    Nat(const Nat& n) : num(n.num)
    { }
    Nat(const Nat&& n) : num(std::move(n.num))
    { }

    // make a natural number from a list from lsb to msb (msb pushed last).
    explicit Nat(const std::vector<uint32_t>& value)
      : num(value) {}

    // make a natural number from a list from lsb to msb (msb pushed last).
    Nat(const std::vector<uint32_t>&& value)
      : num(value)
    {
      //std::cout << "Nat move constructor called, size=" << num.d.size() << std::endl;
    }

    explicit Nat(const std::uint32_t w) : num(w) {}

    Nat operator+(const Nat& b) const
    {
      return Nat(add_vec32(num.d, b.num.d));
    }

    Nat operator*(const Nat&b) const
    {
      return Nat(mul_vec32(num.d, b.num.d));
    }


    bool operator == (const Nat& rhs) const
    {
      return (rhs.num.d == num.d);
    }

    bool operator != (const Nat& rhs) const
    {
      return (rhs.num.d != num.d);
    }

    bool Nat::operator < (const Nat& rhs) const
    {
      return less_than(num.d, rhs.num.d);
    }
    bool operator > (const Nat& rhs) const
    {
      return (rhs < *this);
    }
    bool operator <= (const Nat& rhs) const
    {
      return (*this < rhs) || (*this == rhs);
    }
    bool operator >= (const Nat& rhs) const
    {
      return (*this > rhs) || (*this == rhs);
    }

    const Integral_number num;  // use this if you want to go low_level.

    size_t num_word32() const noexcept { return num.d.size(); }

    //WARNING these next 3 calls can have undefined behavior
    uint32_t get_word(size_t i) const noexcept { return i < num_word32() ? num.d[i] : 0; }
    uint32_t ls_word() const noexcept { return num.d[0]; }
    uint32_t ms_word() const noexcept { return num.d[num_word32() - 1u]; }

    bool is_nonzero() const noexcept { return (num.d.size() != 0u); }
    bool is_zero() const noexcept { return (num.d.size() == 0u); }

  };  //end Nat


  Nat add(const Nat& a, const Nat& b);
  //Nat operator+(const Nat& a, const Nat& b);  // for some reason, unable to inline this

  Nat mul(const Nat& a, const Nat& b);


  // euclidean division, not defined if d=0, will return (0,0) for quotient and remainder
  std::pair<Nat, uint32_t> div(const Nat& n, uint32_t d);
  std::pair<Nat, Nat> div(const Nat& n, const Nat& d);


  // an Int is your classical negate(nonzeroNat) | zero | nonzeroNat
  // no NaN or +inf or -inf
  struct Int {

    Int() : num(uint32_t(0)) {}

    explicit Int(int32_t first, size_t capacity) : num(first, capacity) {}

    Int(const Int& n) : num(n.num)
    { }
    Int(const Int&& n) : num(n.num)
    { }

    Int(const Nat&& nat) : num(std::move(nat.num)) {};

    // make a natural number from a list from lsb to msb (msb pushed last).
    explicit Int(const std::vector<uint32_t>& value, bool isNegative)
      : num(value, isNegative)
    {
    }

    // make Int from a list from lsb to msb (msb pushed last).
    Int(const std::vector<uint32_t>&& value, bool isNegative)
      : num(value, isNegative)
    {
      //std::cout << "Int move constructor called, size=" << num.d.size() << std::endl;
    }

    explicit Int(const std::int32_t w) : num(uint32_t(w < 0 ? w : -w), w < 0) {}

    bool operator == (const Int& rhs) const
    {
      return (rhs.num.negative == num.negative) && (rhs.num.d == num.d);
    }

    bool operator != (const Int& rhs) const
    {
      return (rhs.num.negative != num.negative) || (rhs.num.d != num.d);
    }

    bool operator < (const Int& rhs) const
    {
      bool result(true);
      if (num.negative)
      {
        result = rhs.num.negative ? less_than(rhs.num.d, num.d) : true;
      }
      else
      {
        result = rhs.num.negative ? false : less_than(num.d, rhs.num.d);
      }
      return result;
    }

    bool operator > (const Int& rhs) const
    {
      return (rhs < *this);
    }
    bool operator <= (const Int& rhs) const
    {
      return (*this < rhs) || (*this == rhs);
    }
    bool operator >= (const Int& rhs) const
    {
      return (*this > rhs) || (*this == rhs);
    }

    const Integral_number num;  // use this if you want to go low_level.

    size_t num_word32() const noexcept { return num.d.size(); }
    uint32_t get_word(size_t i) const noexcept { return i < num_word32() ? num.d[i] : 0; }
    uint32_t ls_word() const noexcept { return num.d[0]; }
    uint32_t ms_word() const noexcept { return num.d[num_word32() - 1u]; }

    bool is_nonzero() const noexcept { return (num.d.back() != 0u); }
    bool is_zero() const noexcept { return (num.d.back() == 0u); }

  };  //end Int

    // test for a < b, where a and b are vector<uint32_t>
  bool less_than(const std::vector<uint32_t>& lhs, const std::vector<uint32_t>& rhs) noexcept;
  // multiply a vector of words by a word, in-place
  void scale_by_word(std::vector<uint32_t>& v, const uint32_t word);
  // multiply a vector of words by a word, pure function
  std::vector<uint32_t> mul_vec_by_word(const std::vector<uint32_t>& a, const uint32_t b);
  // increment a vector of words by a word
  void increment_by_word(std::vector<uint32_t>& v, const uint32_t word);
  // decrement v at index i by delta
  void decrement_at_index(std::vector<uint32_t>& v, size_t index,
    const std::vector<uint32_t>&delta);

  std::vector<uint32_t> add_vec32(const std::vector<uint32_t>& a, const std::vector<uint32_t>&b);
  std::vector<uint32_t> mul_old_fashioned(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b);
  std::vector<uint32_t> mul_conv(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b);

  // pre-condition: operands almost the same size,  a.size <= b.size <= a.size+1
  // recursively performs divide and conquer, until a.size <= KARATSUBA_THESHOLD,
  // then just calls a nonrecursive multiply.
  // because halves the size, will recurse at most 64 times, but actually less 
  // because of the threshold transitions to non-recursive.
  // TODO develop on vec32, then change to be a templated (over the iterator class)
  // return false if the invariant was violated.
  // out_iter probably should have reserved (asize + bsize) words.
  const size_t KARATSUBA_THRESHOLD = 40u;
  bool mul_Karatsuba(std::vector<uint32_t>::const_iterator abegin, std::vector<uint32_t>::const_iterator aend,
                                      std::vector<uint32_t>::const_iterator bbegin, std::vector<uint32_t>::const_iterator bend,
                                      std::vector<uint32_t> out_iter);

  // A lazy version of mul_Karatsuba, with same pre-condition
  // Need to pass abegin(), aend(), bbegin(), bend(), each call returns one word of result.
  // TODO implement using the below signature, then 
  class Prod_Karatsuba_generator
  {
    Prod_Karatsuba_generator(std::vector<uint32_t>::const_iterator abegin, std::vector<uint32_t>::const_iterator aend,
                             std::vector<uint32_t>::const_iterator bbegin, std::vector<uint32_t>::const_iterator bend);
    // TODO 
    // class iterator
    //{
    //
    //}

    // iterator::begin();
    // iterator::end();
  };



#if 0 // TODO remove
  class Prod_by_word_generator
  {
  public:
    // constructor for object to perform a*b, where a is a vector and b is a word.
    Prod_by_word_generator(const std::vector<uint32_t>& a, const uint32_t b)
      :m_a(a),
      m_b(b),
      // a*b has size 0 if either is 0, otherwise result size is a.size() or a.size()+1
      m_max_size((a.size() == 0) || (b == 0) ? 0 : a.size() + 1u),
      m_step(0),
      m_accum(0)
    {
    }
    uint32_t get_next_word() noexcept;
    void reset() noexcept;

    const size_t get_result_max_size() const noexcept
    {
      return m_max_size;
    }

    // don't use this, it is slow compared to mul_conv()
    // here as an aid to testing and as an example of
    // how to use this generator.
    // requires get_next_word() has not been called
    std::vector<uint32_t> mul()
    {
      std::vector<uint32_t> result;
      const size_t max_size = get_result_max_size();
      if (max_size == 0)
      {
        return result;
      }

      result.reserve(max_size);

      size_t i(0u);
      uint32_t word(0);
      for (; i < max_size - 1u; ++i)
      {
        word = get_next_word();
        result.push_back(word);
      }

      // now calculate last word of the result vector. May be zero.  Example 3*3=09 but 4*4=16
      word = get_next_word();
      if (word != 0)   // most-significant-word must be nonzero
      {
        result.push_back(word);
      }
      return result;
    }

  private:
    const std::vector<uint32_t>& m_a;
    const uint32_t m_b;
    const size_t m_max_size;
    // next two variables hold the state that changes over time
    size_t m_step;
    uint32_t m_accum;
  };
#endif

#if 0
  // lazily generate the results of a multiply, 
  // starting with the least significant word, using get_next().
  class Prod_generator
  {
  public:
    // constructor for object to perform a*b, where a is a vector and b is a word.
    Prod_generator(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b)
      :m_a(a.size() <= b.size() ? a : b),
      m_b(a.size() <= b.size() ? b : a),
      // a*b has size 0 if either is 0, otherwise result size is a.size() or a.size()+1
      m_max_size((a.size() == 0) || (b.size() == 0) ? 0 : a.size() + b.size()),
      m_step(0),
      m_accum(0u, 0u)
    {

    }
    uint32_t get_next_word();
    void reset();

    const size_t get_result_max_size() const
    {
      return m_max_size;
    }

    // don't use this, it is slow compared to mul_conv()
    // here as an aid to testing and as an example of
    // how to use this generator.
    // requires get_next_word() has not been called
    std::vector<uint32_t> mul()
    {
      std::vector<uint32_t> result;
      const size_t max_size = get_result_max_size();
      if (max_size == 0)
      {
        return result;
      }

      result.reserve(max_size);

      size_t i(0u);
      uint32_t word(0);
      for (; i < max_size - 1u; ++i)
      {
        word = get_next_word();
        result.push_back(word);
      }

      // now calculate last word of the result vector. May be zero.  Example 3*3=09 but 4*4=16
      word = get_next_word();
      if (word != 0)   // most-significant-word must be nonzero
      {
        result.push_back(word);
      }
      return result;
    }

  private:
    const std::vector<uint32_t>& m_a;
    const std::vector<uint32_t>& m_b;
    const size_t m_max_size;
    // next two variables hold the state that changes over time
    size_t m_step;
    std::pair<uint64_t, uint64_t> m_accum;
  };
#endif


} // namespace Big_numbers



std::ostream& operator <<(std::ostream& os, const std::vector<uint32_t>& n);

std::ostream& operator <<(std::ostream& os, const Big_numbers::Nat& n);

#endif // BIG_NUMBERS_H