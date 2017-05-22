#pragma once
#ifndef ARITHMETIC_ALGORITHMS
#define ARITHMETIC_ALGORITHMS

/*
generic arithmetic, using forward and bi-directional const_iterators for input arguments.
Output is either 1-word (in the case of generators) or directed by an output iterator
*/

#include <cstdint>
#include <ostream>
#include <utility>
#include <iso646.h>   // so "not" 'or" and "and" work, VSC++ bug
//#include <iostream> // for debug cout <<

namespace arithmetic_algorithm {

  //precondition: a >= b, as numbers.  Hence the "unsafe" part.
  template <class Fwd_itera, class Fwd_iterb, class Out_container>
  void subtract_unsafe(Fwd_itera aiter, Fwd_itera aend, Fwd_iterb biter, Fwd_iterb bend, Out_container& out)
  {
    // grade-school subtaction, with borrows
    bool borrow(false);
    while (aiter != aend)
    {
      Fwd_itera::value_type aval = *aiter;
      Fwd_iterb::value_type bval = (biter== bend) ? Fwd_iterb::value_type(0u) : *biter;
      Fwd_itera::value_type diff1 = aval - bval;
      Fwd_itera::value_type diff2 = diff1 - borrow;
      borrow = (diff1 > aval) || (diff2 > diff1);
      if (biter != bend)
      {
        ++biter;
      }
      out.push_back(diff2);
      ++aiter;
    }
  }


  // Sum_generator(abegin, aend, bbegin, bend, bool carry_in)
  // represents sum from adding the range bbegin..bend into abegin..aend
  //
  // The usual addition LSW to MSW with carrys
  // Internally, a fwd_iterator returns a word at a time, from LSW to MSW
  //
  template < class Forward_iterator >
  class Sum_generator
  {
  public:
    Sum_generator(
      const Forward_iterator& abegin, const Forward_iterator& aend,
      const Forward_iterator& bbegin, const Forward_iterator& bend, bool carry_in)
      : m_abegin(abegin)
      , m_aend(aend)
      , m_bbegin(bbegin)
      , m_bend(bend)
      , m_carry_in(carry_in)
    {}

    // This default constructor produces an instance with no meaning (until assigned).
    Sum_generator() {}

    bool strongly_equal(const Sum_generator& rhs) const  noexcept
    {
      return
        (m_abegin == rhs.m_abegin)
        && (m_aend == rhs.m_aend)
        && (m_bbegin == rhs.m_bbegin)
        && (m_bend == rhs.m_bend)
        && (m_carry_in == rhs.m_carry_in);
    }

    class const_iterator
    {
    public:
      struct End_tag_t {};

      // default constructor has no meaning until copy-assigned
      const_iterator()
        : m_accum(0)
        , m_value(0)
        , m_done(true)
      {}

      // make an end const_iterator
      const_iterator(const Sum_generator<Forward_iterator>& the_parent, End_tag_t)
        : parent(the_parent)
        , m_aiter(parent.m_aend)
        , m_biter(parent.m_bend)
        , m_accum(0)
        , m_value(0)
        , m_done(true)
      {}

      explicit const_iterator(const Sum_generator<Forward_iterator>& the_parent)
        : parent(the_parent)
        , m_aiter(parent.m_abegin)
        , m_biter(parent.m_bbegin)
        , m_accum(0)
        , m_value(0)
        , m_done(true)
      {
        if (m_aiter != parent.m_aend)
        {
          const uint32_t aval = *m_aiter;
          if (m_biter != parent.m_bend)
          {
            // compute the first value
            uint32_t val = *m_biter;
            bool has_carry(false);
            // add the carry_in
            if (parent.m_carry_in)
            {
              val += 1u;
              has_carry = (val == 0);
            }

            m_value = aval + val;
            has_carry |= (m_value < aval);
            m_accum = uint32_t(has_carry);
            ++m_aiter;
            ++m_biter;
            m_done = (m_aiter == parent.m_aend) && (m_biter == parent.m_bend) && (m_value == 0) && (m_accum == 0);
          }
          else
          {
            // b is 0, just add the carry to a
            m_value = aval + parent.m_carry_in;
            m_accum = uint32_t(m_value < aval);
            ++m_aiter;
            m_done = (m_aiter == parent.m_aend) && (m_value == 0) && (m_accum == 0);
          }
        }
        else
        {
          // a is 0
          if (m_biter != parent.m_bend)
          {
            const uint32_t bval = *m_biter;

            // a is zero, just add the carry to b
            m_value = bval + parent.m_carry_in;
            m_accum = uint32_t(m_value < bval);
            ++m_biter;
            m_done = (m_biter == parent.m_bend) && (m_value == 0) && (m_accum == 0);
          }
          else
          {
            // a and b are zero
            if (parent.m_carry_in)
            {
              m_value = 1u;
              m_done = false;   // one more value to read
            }
          }
        }
      } // end const_iterator constructor

      const_iterator(const const_iterator& rhs)  // copy constructor
        : parent(rhs.parent)
        , m_aiter(rhs.m_aiter)
        , m_biter(rhs.m_biter)
        , m_accum(rhs.m_accum)
        , m_value(rhs.m_value)
        , m_done(rhs.m_done)
      {
      }

      bool operator==(const const_iterator& rhs) const  noexcept
      {
        bool are_equal(false);
        if (m_done != rhs.m_done)
        {
          return are_equal;   // one is at end and other is not, unequal
        }
        are_equal = m_done ||    // both end iterators, so equal
          (
            parent.strongly_equal(rhs.parent)
            && (m_aiter == rhs.m_aiter)
            && (m_biter == rhs.m_biter)
            && (m_accum == rhs.m_accum)
            && (m_value == rhs.m_value));

        return are_equal;
      }

      bool operator!=(const const_iterator& rhs) const noexcept
      {
        return !(*this == rhs);
      }

      const uint32_t & operator*() noexcept
      {
        return m_value;
      }

      //pre-increment
      const_iterator& operator++() // this is noexcept if ++Forward_iterator is noexcept..  how to express?
      {
        if (!m_done)
        {
          uint32_t aval(0);
          uint32_t bval(0);
          if (m_aiter != parent.m_aend)
          {
            aval = (*m_aiter);
            ++m_aiter;
          }
          if (m_biter != parent.m_bend)
          {
            bval = (*m_biter);
            ++m_biter;
          }
          if (0 == m_accum)
          {
            m_value = aval + bval;
            m_accum = (m_value < aval);
          }
          else
          {
            uint32_t value = aval + 1u;
            bool has_carry = (value == 0u);
            value += bval;
            has_carry |= (value < bval);
            m_accum = has_carry;
            m_value = value;
          }
          m_done = (m_aiter == parent.m_aend) && (m_biter == parent.m_bend) && (m_value == 0) && (m_accum == 0);
        }
        return *this;
      }

      // post increment
      const_iterator operator++(int)
      {
        const_iterator tmp(*this);
        operator++();
        return tmp;
      }



      /* TODO an optimization-- construct a rev_iterator from a (non-end) iterator.
      The iterator could keep track of last carry and how many maxvalues
      in-a-row have been encountered during incrementing, and just
      hand this off during construction.  This would be useful in
      convolutional multiplies (a+b)*(c+d) could be done lazily with
      no additional memory allocations.
      */

      using difference_type = ptrdiff_t;
      using value_type = uint32_t;
      using pointer = const uint32_t*;
      using reference = const uint32_t&;   // so read-only
      using iterator_category = std::forward_iterator_tag;


    private:

      const Sum_generator& parent;
      // Forward_iterator m_iter;
      typename Forward_iterator m_aiter;
      typename Forward_iterator m_biter;
      uint32_t m_accum;
      uint32_t m_value;
      bool m_done;  // equivalent to (m_iter==m_end) && (m_accum==0) && (m_value==0)
    };  // end class iterator of Sum_generator


    const_iterator begin() const
    {
      return const_iterator(*this);
    }

    const_iterator end() const
    {
      return const_iterator(*this, const_iterator::End_tag_t());
    }



  private:
    const Forward_iterator& m_abegin;
    const Forward_iterator& m_aend;
    const Forward_iterator& m_bbegin;
    const Forward_iterator& m_bend;
    const bool m_carry_in;

  };  // end class Sum_generator


      // Product_generator(abegin, aend, bbegin, bend)
      // represents product from multiplying the range abegin..aend and bbegin..bend
      // The iterators abegin,aend only need be forward iterators.
      // The iterators bbegin..bend need to be bidirectional iterators.
      // The functions begin() and end() return a forward iterator to return
      // the result from LSW to MSW.  If the result is zero, there is no MSW
      // because begin()==end().  If the result is nonzero, the MSW is nonzero.
      //
  template < class Forward_iterator, class Bidir_iterator >
  class Product_generator
  {
  public:
    Product_generator(
      const Forward_iterator& abegin, const Forward_iterator& aend,
      const Bidir_iterator& bbegin, const Forward_iterator& bend)
      : m_abegin(abegin)
      , m_aend(aend)
      , m_bbegin(bbegin)
      , m_bend(bend)
    {}

    // This default constructor produces an instance with no meaning (until assigned).
    Product_generator() {}

    bool strongly_equal(const Product_generator& rhs) const  noexcept
    {
      return
        (m_abegin == rhs.m_abegin)
        && (m_aend == rhs.m_aend)
        && (m_bbegin == rhs.m_bbegin)
        && (m_bend == rhs.m_bend);
    }

    class iterator
    {
      using Uint128 = std::pair<uint64_t, uint64_t>;  // least and most significant 64 bits

    private:

      bool increment_internal_iterators() // returns true if done with convolutions
      {
        /* incrementing is tricky for convolution product.
        the k-convolution is sum of a[i]*b[j] where i+j==k.
        [astart,aend) [bstart,bend) are intervals for i and j, constrained to be the same length.

        When k needs to increment
        1) Try to increment the end of both intervals, preserving length equality (both lengths increase by 1).
        2) if step 1 fails because one interval, call it foo, failed, while the other, call it bar, succeeded. Increment the start of bar to restore length equality.
        3) If step 1 fails because both intervals could not expand, try to increment the start of both intervals (both lengths decrease by 1).
        4) If try of step 3 fails (astart == aend), the intervals are empty and no increment is possible.
        */
        bool done_with_convolutions(false);
        bool a_can_expand = (m_a_end_conv != parent.m_aend);
        bool b_can_expand = (m_b_end_conv != parent.m_bend);
        if (a_can_expand)
        {
          ++m_a_end_conv;   // lengths no longer equal, a-interval is one longer
          if (b_can_expand)
          {
            ++m_b_end_conv;  // lengths equal
          }
          else
          {
            ++m_a_start_conv;   // lengths equal after restoring length of a-interval
          }

        }
        else
        {
          // a-interval cannot expand
          if (b_can_expand)
          {
            ++m_b_end_conv;  // lengths unequal, b-interval one larger
            ++m_b_start_conv;      // lengths equal again
          }
          else
          {
            // neither can expand, try to shrink by incrementing the start
            if (m_a_start_conv != m_a_end_conv)  //if the lengths are nonzero
            {
              ++m_a_start_conv;    // a-interval one shorter
              ++m_b_start_conv;    // lengths equal again
            }
            else
            {
              done_with_convolutions = true;
            }
          }
        }
        return done_with_convolutions;
      }


    public:
      struct End_tag_t {};

      // default constructor has no meaning until copy-assigned
      iterator()
        : m_step(0u)
        , m_accum(0)
        , m_value(0)
        , m_done(true)
      {}

      // make an end iterator
      iterator(const Product_generator<Forward_iterator, Bidir_iterator>& the_parent, End_tag_t)
        : parent(the_parent)
        , m_a_start_conv(parent.m_aend)
        , m_a_end_conv(m_a_start_conv)
        , m_b_start_conv(parent.m_bend)
        , m_b_end_conv(m_b_start_conv)
        , m_step(1u)  // count the increments that increase the k in k-convolution
        , m_accum(0u, 0u)
        , m_value(0)
        , m_done(true)
      {
      }

      explicit iterator(const Product_generator<Forward_iterator, Bidir_iterator>& the_parent)
        : parent(the_parent)
        , m_a_start_conv(parent.m_abegin)
        , m_a_end_conv(m_a_start_conv)
        , m_b_start_conv(parent.m_bbegin)
        , m_b_end_conv(m_b_start_conv)
        , m_step(0u)
        , m_accum(0u, 0u)
        , m_value(0)
        , m_done(true)
      {
        // if either multiplicand is 0, return 0.
        if (m_a_start_conv == parent.m_aend)
        {
          return;
        }

        if (m_b_start_conv == parent.m_bend)
        {
          return;
        }
        ++m_a_end_conv;  // points 1 beyond m_a_start_conv
        ++m_b_end_conv;  // points 1 begond m_b_start_conv

                         // perform 1-convolutional product     
        const uint64_t sum = m_accum.first + (uint64_t(*m_a_start_conv) * uint64_t(*m_b_start_conv));
        m_accum.first = sum;

        const uint64_t LSW_MASK(0xffff'ffffu);
        m_value = uint32_t(sum & LSW_MASK);
        // shift m_accum right 32 bits (divide by 2**32)
        m_accum.first = (sum >> 32u);
        m_done = false;
        m_step += not increment_internal_iterators();  // keep track of the k in k-convolutions
      } // end iterator constructor

      iterator(const iterator& rhs)  // copy constructor
        : parent(rhs.parent)
        , m_a_start_conv(rhs.m_a_start_conv)
        , m_a_end_conv(rhs.m_a_end_conv)
        , m_b_start_conv(rhs.m_b_start_conv)
        , m_b_end_conv(rhs.m_b_end_conv)
        , m_step(0u)
        , m_accum(rhs.m_accum)
        , m_value(rhs.m_value)
        , m_done(rhs.m_done)
      {
      }

      bool operator==(const iterator& rhs) const  noexcept
      {
        bool are_equal(false);
        if (m_done != rhs.m_done)
        {
          return are_equal;   // one is at end and other is not, unequal
        }
        are_equal = m_done ||    // both end iterators, so equal
          (
            parent.strongly_equal(rhs.parent)
            && (m_a_start_conv == rhs.m_a_start_conv)
            && (m_a_end_conv == rhs.m_a_end_conv)
            && (m_b_start_conv == rhs.m_b_start_conv)
            && (m_b_end_conv == rhs.m_b_end_conv)
            && (m_accum == rhs.m_accum)
            && (m_value == rhs.m_value));

        return are_equal;
      }

      bool operator!=(const iterator& rhs) const noexcept
      {
        return !(*this == rhs);
      }

      const uint32_t & operator*() noexcept
      {
        return m_value;
      }



      //pre-increment
      iterator& operator++() // this is noexcept if ++Forward_iterator is noexcept..  how to express?
      {

        if (m_done)
        {
          m_value = 0u;  // keep returning 0 on overflow
          return *this; // ++ has no effect if done
        }

        // perform convolutional product     
        std::reverse_iterator<Bidir_iterator> bp(m_b_end_conv);
        Forward_iterator ap = m_a_start_conv;
        while (ap != m_a_end_conv)
        {
          // add prod and acc, careful to detect carries
          const uint64_t sum = m_accum.first + (uint64_t(*ap) * uint64_t(*bp));
          //std::cout << "convolve2c sum " << acc.first << " + "  << (*ap) << " * " << (*bp) << " = " << sum << std::endl;
          m_accum.second += (sum < m_accum.first);  // add 1 if overflow
          m_accum.first = sum;

          ++ap;
          ++bp;
        }
        const uint64_t LSW_MASK(0xffff'ffffu);
        m_value = uint32_t(m_accum.first & LSW_MASK);
        // shift m_accum right 32 bits (divide by 2**32)
        m_accum.first = (m_accum.first >> 32u) + ((m_accum.second & 0xFFFF'FFFFu) << 32u);
        m_accum.second = (m_accum.second >> 32u);

        const bool done_with_convolutions = increment_internal_iterators();
        if (done_with_convolutions)
        {
          m_done = (m_accum.first == 0u) && (m_accum.second == 0u) && (m_value == 0u);
        }
        else
        {
          ++m_step; // keep track of the k in k-convolutions
        }
        return *this;
      }

      // post increment
      iterator operator++(int)
      {
        iterator tmp(*this);
        operator++();
        return tmp;
      }

      using difference_type = ptrdiff_t;
      using value_type = uint32_t;
      using pointer = const uint32_t*;
      using reference = const uint32_t&;   // so read-only
      using iterator_category = std::forward_iterator_tag;


    private:

      const Product_generator& parent;
      // Forward_iterator m_iter;
      // can view m_a_start_conv as parent.m_abegin + ka, constrained by k = ka + kb,  where k in closed interval [0, asize + bsize -1]

      typename Forward_iterator m_a_start_conv; // where k-convolution starts, parent.m_abegin + ka, constrained by k = ka+kb 
      typename Forward_iterator m_a_end_conv; // where k-convolution ends, parent.m_abegin + k, constrained by k <= a_size 
      typename Bidir_iterator m_b_start_conv;
      typename Bidir_iterator m_b_end_conv; // where k-convolution ends, parent.m_bbegin + k, constrained by k <= b_size
      size_t m_step;     // 0..a_size+b_size-1 the k of a k-convolution
      Uint128 m_accum;
      uint32_t m_value;
      bool m_adone;
      bool m_end_adone;
      bool m_bdone;
      bool m_end_bdone;
      bool m_done;  // equivalent to (m_iter==m_end) && (m_accum==0) && (m_value==0)
    };  // end class iterator of Product_generator


    iterator begin() const
    {
      return iterator(*this);
    }

    iterator end() const
    {
      return iterator(*this, iterator::End_tag_t());
    }



  private:
    const Forward_iterator& m_abegin;
    const Forward_iterator& m_aend;
    const Bidir_iterator& m_bbegin;
    const Bidir_iterator& m_bend;

  };  // end class Product_generator


  // class Product_by_word_generator
  // Represents a product, and returns the results
  // via iterator, rereferencing returns LSW first and MSw last.
  // If the product is zero, the begin and end iterators will be equal.
  // 
  // The input argument iterators must be forward_iterators representing a range,
  // with a value_type of uint32_t.
  //
  // The internal iterator is a const forward_iterator
  // (it cannot be used to change this product, only to read words out).
  // Attempting to increment beyond the end is legal and does nothing.
  // Attempting to dereference an end iterator always produces 0.
  template < class Forward_iterator >
  class Product_by_word_generator
  {
  public:
    Product_by_word_generator(const Forward_iterator& abegin, const Forward_iterator& aend, const uint32_t b)
      : m_abegin(abegin)
      , m_aend(aend)
      , m_b(b)
    {}


    // This default constructor produces an instance with no meaning (until assigned).
    Product_by_word_generator() : m_b(0) {}

    bool operator==(const Product_by_word_generator& rhs) const  noexcept
    {
      return (m_abegin == rhs.m_abegin)
        && (m_aend == rhs.m_aend)
        && (m_b == rhs.m_b);
    }

    bool operator!=(const Product_by_word_generator& rhs) const noexcept
    {
      return !(*this == rhs);
    }

    class const_iterator
    {
    public:
      struct End_tag_t {};

      // default constructor has no meaning until copy-assigned
      const_iterator()
        : m_accum(0)
        , m_value(0)
        , m_done(true)
      {}

      // make an end const_iterator
      const_iterator(const Product_by_word_generator<Forward_iterator>& the_parent, End_tag_t)
        : parent(the_parent)
        , m_iter(parent.m_aend)
        , m_accum(0)
        , m_value(0)
        , m_done(true)
      {}

      explicit const_iterator(const Product_by_word_generator<Forward_iterator>& the_parent)
        : parent(the_parent)
        , m_iter(parent.m_abegin)
        , m_accum(0)
        , m_value(0)
        , m_done(true)
      {
        if ((m_iter == parent.m_aend) || (parent.m_b == 0))
        {
          return;  // product is 0
        }

        // compute the first value
        const uint32_t aword = (*m_iter);
        ++m_iter;
        const uint64_t prod = uint64_t(aword)*uint64_t(parent.m_b);
        m_accum = prod >> 32;
        m_value = prod & 0xffff'ffff;
        m_done = false;
      }

      const_iterator(const const_iterator& rhs)  // copy constructor
        : parent(rhs.parent)
        , m_iter(rhs.m_iter)
        , m_accum(rhs.m_accum)
        , m_value(rhs.m_value)
        , m_done(rhs.m_done)
      {
      }

      bool operator==(const const_iterator& rhs) const  noexcept
      {
        bool are_equal(false);
        if (m_done != rhs.m_done)
        {
          return are_equal;   // one is at end and other is not, unequal
        }
        are_equal = m_done ||    // both end iterators, so equal
          ((parent == rhs.parent)
            && (m_iter == rhs.m_iter)
            && (m_accum == rhs.m_accum)
            && (m_value == rhs.m_value));

        return are_equal;
      }

      bool operator!=(const const_iterator& rhs) const noexcept
      {
        return !(*this == rhs);
      }

      const uint32_t & operator*() noexcept
      {
        return m_value;
      }

      //pre-increment
      const_iterator& operator++() // this is noexcept if ++m_iter is noexcept..  how to express?
      {
        if (!m_done)
        {
          if (m_iter != parent.m_aend)
          {
            const uint32_t aword = (*m_iter);
            ++m_iter;
            const uint64_t prod = uint64_t(aword)*uint64_t(parent.m_b) + uint64_t(m_accum);
            m_accum = prod >> 32;
            m_value = prod & 0xffff'ffff;
          }
          else
          {
            m_value = m_accum;    // possibly an overflow word produced
            m_accum = 0;          // force this iterator to be an end iterator
            m_done |= (m_value == 0);    // once m_value is 0, done
          }
        }
        return *this;
      }

      // post increment
      const_iterator operator++(int)
      {
        const_iterator tmp(*this);
        operator++();
        return tmp;
      }

      using difference_type = ptrdiff_t;
      using value_type = uint32_t;
      using pointer = const uint32_t*;
      using reference = const uint32_t&;   // so read-only
      using iterator_category = std::forward_iterator_tag;


    private:

      const Product_by_word_generator& parent;
      // Forward_iterator m_iter;
      typename Forward_iterator m_iter;
      uint32_t m_accum;
      uint32_t m_value;
      bool m_done;  // equivalent to (m_iter==m_end) && (m_accum==0) && (m_value==0)
    };

    const_iterator begin() const
    {
      return const_iterator(*this);
    }

    const_iterator end() const
    {
      return const_iterator(*this, const_iterator::End_tag_t());
    }


  private:
    const Forward_iterator& m_abegin;
    const Forward_iterator& m_aend;
    const uint32_t m_b;

  };  // end class Product_by_word_generator


  // class Sum_by_word_generator
  // Represents a sum, and returns the results
  // via iterator.
  // A fwd interator returns LSW first and MSW last.
  // A rev_iterator returns MSW first and LSW last
  // If the results is zero, the begin and end iterators will be equal.
  // 
  // The input argument iterators must be forward_iterators representing a range,
  // with a value_type of uint32_t.
  //
  // The output iterator is a non-mutable forward_iterator.
  // Attempting to increment beyond the end is legal and does nothing.
  // Attempting to dereference an end iterator always produces 0.
  template < class Forward_iterator >
  class Sum_by_word_generator
  {
  public:
    Sum_by_word_generator(const Forward_iterator& abegin, const Forward_iterator& aend, const uint32_t b)
      : m_abegin(abegin)
      , m_aend(aend)
      , m_b(b)
    {}

    // This default constructor produces an instance with no meaning (until assigned).
    Sum_by_word_generator() : m_b(0) {}

    bool operator==(const Sum_by_word_generator& rhs) const  noexcept
    {
      return (m_abegin == rhs.m_abegin)
        && (m_aend == rhs.m_aend)
        && (m_b == rhs.m_b);
    }

    bool operator!=(const Sum_by_word_generator& rhs) const noexcept
    {
      return !(*this == rhs);
    }

    class iterator
    {
    public:
      struct End_tag_t {};

      // default constructor has no meaning until copy-assigned
      iterator()
        : m_accum(0)
        , m_value(0)
        , m_done(true)
      {}

      // make an end iterator
      iterator(const Sum_by_word_generator<Forward_iterator>& the_parent, End_tag_t)
        : parent(the_parent)
        , m_iter(parent.m_aend)
        , m_accum(0)
        , m_value(0)
        , m_done(true)
      {}

      explicit iterator(const Sum_by_word_generator<Forward_iterator>& the_parent)
        : parent(the_parent)
        , m_iter(parent.m_abegin)
        , m_accum(0)
        , m_value(0)
        , m_done(true)
      {
        if (m_iter != parent.m_aend)
        {
          // compute the first value
          m_value = *m_iter + parent.m_b;
          m_accum = uint32_t(m_value < parent.m_b);  // 0 or 1 for carry
          ++m_iter;
          m_done = (m_iter == parent.m_aend) && (m_value == 0) && (m_accum == 0);
        }
        else
        {
          // a is zero (the empty vector)
          m_value = parent.m_b;
          m_done = (parent.m_b == 0u);  // sum is zero, all done
        }
      }

      iterator(const iterator& rhs)  // copy constructor
        : parent(rhs.parent)
        , m_iter(rhs.m_iter)
        , m_accum(rhs.m_accum)
        , m_value(rhs.m_value)
        , m_done(rhs.m_done)
      {
      }

      bool operator==(const iterator& rhs) const  noexcept
      {
        bool are_equal(false);
        if (m_done != rhs.m_done)
        {
          return are_equal;   // one is at end and other is not, unequal
        }
        are_equal = m_done ||    // both end iterators, so equal
          ((parent == rhs.parent)
            && (m_iter == rhs.m_iter)
            && (m_accum == rhs.m_accum)
            && (m_value == rhs.m_value));

        return are_equal;
      }

      bool operator!=(const iterator& rhs) const noexcept
      {
        return !(*this == rhs);
      }

      const uint32_t & operator*() noexcept
      {
        return m_value;
      }

      //pre-increment
      iterator& operator++() // this is noexcept if ++m_iter is noexcept..  how to express?
      {
        if (!m_done)
        {
          if (m_iter != parent.m_aend)
          {
            const uint32_t aword = (*m_iter);
            ++m_iter;
            m_value = aword + m_accum;
            m_accum = uint32_t(m_value < aword);  // another carry to propagate?
          }
          else
          {
            m_value = m_accum;    // possibly an overflow word produced
            m_accum = 0;          // force this iterator to be an end iterator
            m_done |= (m_value == 0);    // once m_value is 0, done
          }
        }
        return *this;
      }

      // post increment
      iterator operator++(int)
      {
        iterator tmp(*this);
        operator++();
        return tmp;
      }

      using difference_type = ptrdiff_t;
      using value_type = uint32_t;
      using pointer = const uint32_t*;
      using reference = const uint32_t&;   // so read-only
      using iterator_category = std::forward_iterator_tag;


    private:

      const Sum_by_word_generator& parent;
      // Forward_iterator m_iter;
      typename Forward_iterator m_iter;
      uint32_t m_accum;
      uint32_t m_value;
      bool m_done;  // equivalent to (m_iter==m_end) && (m_accum==0) && (m_value==0)
    };  // end class iterator of Sum_by_word_generator


    iterator begin() const
    {
      return iterator(*this);
    }

    iterator end() const
    {
      return iterator(*this, iterator::End_tag_t());
    }


    //const_iterator begin() const;




    // The reverse iterator keeps track of how many MSW (at back of vector) are maxvalue.
    // and the value of the carry (0 or 1) from the before the run of maxvalues.
    // The sum at each of these places is either maxvalue or 0, depending on the carry.
    // By doing this once, the number of accesses to each word is limited to 1 pass, 
    // which is good for cache and means only n memory reads are necessary (amortized).

    // invariants: TODO reach clarity in the invariants.
    // m_iter is pointing to the place used to generate
    // the carry-in for m_value, if not m_done.
    // m_done true after construction if 0 is the result
    //             after increment has been called when (m_iter == a.parent.abegin) && (0 == m_run_of_maxval)  
    class rev_iterator
    {
    public:
      struct End_tag_t {};
      // default constructor has no meaning until copy-assigned
      // invariants do not hold for it. Only useful for array construction.
      rev_iterator()
        : parent(Sum_by_word_generator())
        , m_iter(parent.m_aend)
        , m_carry_before_run(0u)
        , m_run_of_maxval(0u)
        , m_sum(0u)
        , m_value(0u)
        , m_done(true)
      {}

      /* TODO an optimization-- construct a rev_iterator from an iterator.
       The iterator could keep track of last carry and how many maxvalues
       in-a-row have been encountered during incrementing, and just
       hand this off during construction.  This would be useful in
       convolutional multiplies (a+b)*(c+d) could be done lazily with
       no additional memory allocations.
      
       (a*b)*(c*d) tricky, assuming caller has set-aside
       memory sizeof(a)+sizeof(b)+sizeof(cd)+sizeof(d),
       store biggest sub-prod first (eg a*b), then smaller,
       but then not sure how to do multiply in-place. 
       example
         (90*90) * (30 * 70)
         allocate mem for 8 digits.
         xxxxxxxx
         00180012    (MSD's are last, opposite usual writing convention, so this is 8100 and 2100 for sub-products)
            0     3 -convolution v[0]*v[7] + v[1]*v[6] + v[2]*v[5] + v[3]*v[4]   = 0*2 + 0*1 + 1*0 + 8*0
             1    4 -convolution v[1]*v[7] + v[2]*v[6] + v[3]*v[5]               = 0*2 + 1*1 + 8*0 
           0      2 -convolution v[0]*v[6] + v[1]*v[5] + v[2]*v[4]               = 0*1 + 0*0 + 1*0
              01  5 -convolution v[2]*v[7] + v[3]*v[6]                           = 1*2 + 8*1
          0       1 -convolution v[0]*v[5] + v[1]*v[4]                           = 0*0 + 0*0
         0        0 -convolution v[0]*v[4]                                       = 0*0
               61 6 -convolution v[3]*v[7]                                       = 8*2
         ------------
         00001071  17,010,000

         Could do convolutions k=3,2,1 storing results in r[3], r[2], r[1]
                               k=0         v[0] *= v[4], v[1] = r[1]
                               k=4         v[4]
                               k=5         v[5],  v[2]=r[2]
                               k=6         v[6],v[7]  v[3]=r3
      */


      // make an end rev_iterator
      rev_iterator(const Sum_by_word_generator<Forward_iterator>& the_parent, End_tag_t)
        : parent(the_parent)
        , m_iter(parent.m_aend)
        , m_carry_before_run(0u)
        , m_run_of_maxval(0u)
        , m_sum(0u)
        , m_value(0u)
        , m_done(true)
      {}

      // make a begin rev_iterator
      explicit rev_iterator(const Sum_by_word_generator<Forward_iterator>& the_parent)
        : parent(the_parent)
        , m_iter(parent.m_aend)
        , m_carry_before_run(0)
        , m_run_of_maxval(0)
        , m_sum(0)
        , m_value(0)
        , m_done(false)
      {
        size_t runcount(0);
        if (parent.m_aend == parent.m_abegin)
        {
          m_value = parent.m_b;
          m_done = (0u == m_value);
          return;
        }

        const uint32_t maxval(0xffff'ffffu);
        // loop backwards, counting the run of maxval.
        --m_iter;  // points to the last element
        const uint32_t first_val = *m_iter;
        if (m_iter == parent.m_abegin)
        {
          uint32_t sum = first_val + parent.m_b;
          // sum is 1 if there was a carry, otherwise 0
          if (sum < first_val)
          {
            m_value = 1;  // just adding two words, but overflowed a new MSW
            m_sum = sum;  // will use this next increment
            m_carry_before_run = 1u; // and this too
          }
          else
          {
            m_value = sum;
            m_done = true;
          }
          return;
        }
        //uint32_t value_after_run = first_val;
        uint32_t sumval(0u);
        uint32_t run_count(0u);
        m_carry_before_run = 0;

        m_sum = 0;
        do
        {
          --m_iter;
          uint32_t aval = *m_iter;
          uint32_t val_to_add = (m_iter == parent.m_abegin) ? (parent.m_b) : uint32_t(0);
          uint32_t sumval(aval + val_to_add);
          if (sumval != maxval)
          {
            m_carry_before_run = (sumval < aval);  // detect the carry
            m_sum = sumval;
            break;
          }
          ++run_count;
        } while (m_iter != parent.m_abegin);
        // if m_iter != parent.m_abegin, broke out of loop because sumval!=maxval, the run ended
        // otherwise the run continues all the way first word sum, and m_carry_before_run = 0
        if ((first_val == maxval) && m_carry_before_run)
        {
          m_value = 1u;  // the carry overflowed into a new MSW
          ++run_count;   // ensures run_count zeros are output for the run of carries
        }
        else
        {
          m_value = first_val + m_carry_before_run;
        }
        m_run_of_maxval = run_count;
      }

      rev_iterator(const rev_iterator& rhs)  // copy constructor
        : parent(rhs.parent)
        , m_iter(rhs.m_iter)
        , m_carry_before_run(rhs.m_carry_before_run)
        , m_run_of_maxval(rhs.m_run_of_maxval)
        , m_sum(rhs.m_sum)
        , m_value(rhs.m_value)
        , m_done(rhs.m_done)
      {
      }

      bool operator==(const rev_iterator& rhs) const  noexcept
      {
        bool are_equal(false);
        if (m_done != rhs.m_done)
        {
          return are_equal;   // one is at end and other is not, unequal
        }
        are_equal = m_done ||    // both end iterators, so equal
          ((parent == rhs.parent)
            && (m_iter == rhs.m_iter)
            && (m_carry_before_run == rhs.m_carry_before_run)
            && (m_run_of_maxval == rhs.m_run_of_maxval)
            && (m_sum == rhs.m_sum)
            && (m_value == rhs.m_value));

        return are_equal;
      }

      bool operator!=(const rev_iterator& rhs) const noexcept
      {
        return !(*this == rhs);
      }

      const uint32_t & operator*() noexcept
      {
        return m_value;
      }

      //pre-increment
      rev_iterator& operator++() // this is noexcept if ++m_iter is noexcept..  how to express?
      {

        if (!m_done)
        {
          const uint32_t maxval(0xffff'ffffu);

          // if run_count!=0, return maxval or 0 based on the carry.
          if (0 != m_run_of_maxval)
          {
            m_value = maxval + m_carry_before_run;  // 0 if there was a carry, maxval otherwise
            --m_run_of_maxval;
          }
          else if (m_iter == parent.m_abegin)
          {
            // since 0==m_run_of_maxval, possibly done
            if (m_carry_before_run)
            {
              m_value = m_sum;
              m_carry_before_run = 0;
            }
            else
            {
              m_value = 0;
              m_done = true;
            }
          }
          else
          {
            //m_iter now points to position that was used to calculate m_sum and 
            // carry-in for m_value.
            // go back at least one, but maybe more until *m_iter is not maxval
            uint32_t run_count(0);
            const uint32_t carryless_value = m_sum;
            do
            {
              --m_iter;
              // 
              const uint32_t aval = *m_iter;
              uint32_t val_to_add = (m_iter == parent.m_abegin) ? (parent.m_b) : uint32_t(0);
              uint32_t sumval(aval + val_to_add);
              if (sumval != maxval)
              {
                m_carry_before_run = (sumval < aval);  // detect the carry
                m_sum = sumval;
                break;
              }
              ++run_count;
            } while (m_iter != parent.m_abegin);

            m_run_of_maxval = run_count;
            m_value = carryless_value + m_carry_before_run;
          }


        }
        return *this;
      }

      // post increment
      rev_iterator operator++(int)
      {
        rev_iterator tmp(*this);
        operator++();
        return tmp;
      }
      using difference_type = ptrdiff_t;
      using value_type = uint32_t;
      using pointer = const uint32_t*;
      using reference = const uint32_t&;   // so read-only
      using iterator_category = std::forward_iterator_tag;

    private:

      const Sum_by_word_generator& parent;
      typename Forward_iterator m_iter;
      uint32_t m_carry_before_run;
      size_t m_run_of_maxval;
      uint32_t m_sum;
      uint32_t m_value;
      bool m_done;  // equivalent to (m_iter== parent.abegin) 


    };  // end rev_iterator



    rev_iterator rbegin() const
    {
      return rev_iterator(*this);
    }

    rev_iterator rend() const
    {
      return rev_iterator(*this, rev_iterator::End_tag_t());
    }


  private:
    const Forward_iterator& m_abegin;
    const Forward_iterator& m_aend;
    const uint32_t m_b;

  };  // end class Sum_by_word_generator


      // TODO Symdiff_generator  has iterator for result and
      // can be used as a bool to determine if (a>=b)
      // NOTE: either requires
      // 1  sizes passed and bidirectional iterators (so if same sizes can scan from MSW down)
      // 2  have to scan entirety of a and b in constructor, to see which is bigger
      // 3  assume a>=b as a requires of the call

      // TODO pair(fwd_iter_a, fwd_iter_b) locate_signficant_diff(a_begin, a_end, b_begin, b_end)
      // returns the iterators of the MSW where a and b first differ.
      // if both are end, a==b
      // if fwd_iter_a is end, then b is longer and fwd_iter_b is word and index (a.end()-a.begin())
      // if fwd_iter_b is end, the a is longer and fwd_iter_a is word and index (b.end()-b.begin())
      // else neither is end, and a<b or b<a, can determine using *fwd_iter_a < *fwd_iter_b
      // this algorithm will have to scan all of a or all of b to determine the longest.






} // namespace


#endif // ARITHMETIC_ALGORITHMS