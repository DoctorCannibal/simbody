#ifndef SimTK_SimTKCOMMON_ARRAY_H_
#define SimTK_SimTKCOMMON_ARRAY_H_

/* -------------------------------------------------------------------------- *
 *                      SimTK Core: SimTKcommon                               *
 * -------------------------------------------------------------------------- *
 * This is part of the SimTK Core biosimulation toolkit originating from      *
 * Simbios, the NIH National Center for Physics-Based Simulation of           *
 * Biological Structures at Stanford, funded under the NIH Roadmap for        *
 * Medical Research, grant U54 GM072970. See https://simtk.org.               *
 *                                                                            *
 * Portions copyright (c) 2010 Stanford University and the Authors.           *
 * Authors: Michael Sherman                                                   *
 * Contributors:                                                              *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS, CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,    *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR      *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE  *
 * USE OR OTHER DEALINGS IN THE SOFTWARE.                                     *
 * -------------------------------------------------------------------------- */

/** @file
 * This file defines the SimTK::Array_<T> class and related support classes.
 */

#include "SimTKcommon/internal/common.h"

#include <algorithm>
#include <iterator>
#include <vector>
#include <ostream>

namespace SimTK {

// We want the index_type and size_type for ordinary integral types to
// be either both signed or both unsigned so that an index value can
// be compared against a container's size() method without a warning.
// Also, there has to be a signed difference_type that can hold the
// difference between any two valid indices. This means we can't use
// the full range of unsigned types.

template <class X> struct IndexTraits {
    typedef X                           index_type;
    typedef typename X::size_type       size_type;
    typedef typename X::difference_type difference_type;
    // We require that max_index()+1 fit in size_type and that
    // -max_index() and max_index() fit in difference type.
    static const size_type max_size = X::max_size;
    static const char* index_name() {return X::index_name();}
};

// If max_size is m, then indices range from 0..m-1, so index differences
// range from 1-m to m-1. If the signed difference type has the same number 
// of bits as the unsigned index, we have to limit m so that m-1 is 
// representable in the signed difference type.
template <> struct IndexTraits<unsigned> {
    typedef unsigned    index_type;
    typedef unsigned    size_type;
    typedef int         difference_type;
    static const size_type max_size = 0x7fffffffU;
    static const char* index_name() {return "unsigned";}
};

template <> struct IndexTraits<int> {
    typedef int             index_type;
    typedef int             size_type;
    typedef int             difference_type;
    static const size_type  max_size = 0x7fffffff;
    static const char* index_name() {return "int";}
};

template <> struct IndexTraits<unsigned short> {
    typedef unsigned short  index_type;
    typedef unsigned short  size_type;
    typedef short           difference_type;
    static const size_type  max_size = 0x7fffU;
    static const char* index_name() {return "unsigned short";}
};

template <> struct IndexTraits<short> {
    typedef short     index_type;
    typedef short     size_type;
    typedef short     difference_type;
    static const size_type max_size = 0x7fff;
    static const char* index_name() {return "short";}
}; 

template <> struct IndexTraits<unsigned long long> {
    typedef unsigned long long  index_type;
    typedef unsigned long long  size_type;
    typedef long long           difference_type;
    static const size_type max_size = 0x7fffffffffffffffULL;
    static const char* index_name() {return "unsigned long long";}
};

template <> struct IndexTraits<long long> {
    typedef long long   index_type;
    typedef long long   size_type;
    typedef long long   difference_type;
    static const size_type max_size = 0x7fffffffffffffffLL;
    static const char* index_name() {return "long long";}
};

// A container using unsigned char as an index should use unsigned char
// as its size, meaning the max size is 255 and the max index must be
// 254. Then the difference type must hold -254..254 which takes a short.
template <> struct IndexTraits<unsigned char> {
    typedef unsigned char index_type;
    typedef unsigned char size_type;
    typedef short         difference_type;
    static const size_type max_size = 255;
    static const char* index_name() {return "unsigned char";}
};

// A container using signed char as an index should used signed char as
// its size also, so the max size is 127 meaning the max index is 126
// and the difference range is -126..126 which fits in a signed char.
template <> struct IndexTraits<signed char> {
    typedef signed char index_type;
    typedef signed char size_type;
    typedef signed char difference_type;
    static const size_type max_size = 127;
    static const char* index_name() {return "signed char";}
};

// We won't use the top bit of a char index so sizes are 0 to 127
// and index differences -126..126 which fits in a signed char.
template <> struct IndexTraits<char> {
    typedef char        index_type;
    typedef char        size_type;
    typedef signed char difference_type;
    static const size_type max_size = 127;
    static const char* index_name() {return "char";}
};

/**
The Array_<T> container class is a plug-compatible replacement for the STL
std::vector<T> class, but with a number of advantages in binary compatibility, 
performance, and functionality. Basically this is just an std::vector without 
the nanny-state coddling that sometimes gets in the way when you're in a hurry.

@par Compatibility:
Included here are binary compatibility issues and compatibility with the C++
standard STL objects.

- Most important, it is safe to pass an Array_<T> through an API to a binary 
  library without worrying about compiler version or Release/Debug compatibility
  issues. For a given compiler (e.g. gcc or Microsoft cl) and word size (64 bit
  vs. 32 bit), Array_<T> has an extremely stable memory layout that is preserved 
  across compiler versions, and between Release and Debug builds. This allows us
  to use Array_<T> in the SimTK API where use of std::vector<T> would be 
  desirable but problematic.
- It supports all standard types, methods, iterators, and operators of the C++
  standard std::vector<T> so it works smoothly with all STL containers and
  algorithms.
- It is convertible to and from std::vector<T>, although that requires
  copying of the elements.

@par Performance:
There are several performance problems with the C++ standard STL design in
general, and with Microsoft's implementation in particular, that are addressed
here. Microsoft in its wisdom decided that STL containers should still range 
check in Release builds for safety, but that makes them too slow for many 
purposes (and also breaks the promise of generic programming but that's 
another rant). Attempting to disable these checks breaks binary compatibility. 
Message to realtime and game programmers: this container was designed by 
people as fanatic as you are about performance.

- No heap allocation occurs when an empty Array_<T> object is declared; in
  that case both begin() and end() are null.
- Array_<T> methods are extremely fast in Release builds with zero overhead, 
  inline, unchecked methods. The implementations of inline methods are kept
  small to ensure that they are actually inlined in practice; and generated 
  assembly code was examined to make sure.
- There are some dangerous extensions provided that permit the expert user
  to construct objects directly into the array without having to copy them,
  a big win for complicated objects and even bigger for those that don't
  have copy constructors!
- There is a constant-time eraseFast() method you can use if you don't mind the
  array being reordered after the erase. This avoids the extremely expensive
  "compress" activity required by the standard erase() method.
- The default size_type for an Array_<T> is a 32-bit integer rather than a
  size_t. On a 64-bit machine that keeps the overhead down substantially since
  the structure is then one 64-bit pointer and two 32-bit integers, fitting
  nicely into a cleanly alignable 16 bytes.
- The optional index-type template parameter can be used to reduce the memory
  footprint to as little as 8 bytes on a 32 bit machine (e.g., a 32 bit 
  pointer and two shorts.)

@par Functionality:
For the most part Array_<T> is a plug-compatible replacement for std::vector<T>,
and everything that both classes can do is done the same way. However, there 
are a few additions that we have found useful.

- You can specify an optional index type which can be used to provide
  type-safe indexing (i.e. the array can only be indexed by indices of
  a particular type, like MobilizedBodyIndex). This has zero performance cost
  if the index is an integral type or class consisting of only an integral
  value as produced by the SimTK_DEFINE_UNIQUE_INDEX_TYPE macro.
- You can create uninitialized slots in the array and construct directly into
  them rather than having to construct a temporary object which must then be
  copied into the array.

**/
template <class T, class X=int> class Array_ {
//------------------------------------------------------------------------------
public:
//------------------------------------------------------------------------------

typedef T           value_type;
typedef T*          pointer;
typedef const T*    const_pointer;
typedef T&          reference;
typedef const T&    const_reference;
typedef T*          iterator;
typedef const T*    const_iterator;

typedef std::reverse_iterator<iterator>         reverse_iterator;
typedef std::reverse_iterator<const_iterator>   const_reverse_iterator;

typedef X                                       index_type;
typedef IndexTraits<X>                          index_traits;
typedef typename index_traits::size_type        size_type;
typedef typename index_traits::difference_type  difference_type;

/** Return the maximum allowable size for this container. **/
size_type   max_size()   const {return index_traits::max_size;}
const char* index_name() const {return index_traits::index_name();}

//TODO
// insert, assign
// check standard
// more raw methods


/** @name           Construction and destruction

A variety of constructors are provided for this class, including all those
required by the C++ standard for std::vector implementations, plus additional
ones providing smooth conversions between Array_<T> and std::vector<T> objects.
**/
/*{*/

/** Default constructor allocates no heap space and is very fast. **/
Array_() : data(0), nUsed(0), nAllocated(0) {}

/** Construct an array containing n default-constructed elements. T's default 
constructor is called exactly n times. If n is zero no heap space will be 
allocated; although in that case it is preferable to use the default 
constructor if you can since that will be somewhat faster. **/
explicit Array_(size_type n) {
    SimTK_SIZECHECK(n, max_size(), "Array_<T>::ctor(n)");
    allocateNoConstruct(n);
    defaultConstruct(data, data+n);
    nUsed = n;
}

/** Construct an array containing n elements each set to a copy of the given 
initial value. T's copy constructor will be called exactly n times. If n is 
zero no space will be allocated. **/
Array_(size_type n, const T& initVal) {
    SimTK_SIZECHECK(n, max_size(), "Array_<T>::ctor(n,T)");
    allocateNoConstruct(n);
    fillConstruct(data, data+n, initVal);
    nUsed = n;
}

/** Construct an array from a range of values identified by a begin and an end
iterator. This is templatized so can be used with any source type T2 that is 
assignment-compatible with this array's element type T. **/
template <class T2>
Array_(const T2* first, const T2* last1) {
    const char* methodName = "Array_<T>::ctor(first,last1)";
    SimTK_ERRCHK((first&&last1)||(first==last1), methodName, 
        "Pointers must be non-null unless they are both null.");
    SimTK_ERRCHK3(isSizeOK(last1-first), methodName,
        "Source has %llu elements but this array is limited to %llu"
        " elements by its index type %s.",
        (unsigned long long)(last1-first), ullMaxSize(), index_name());
    nUsed = size_type(last1-first);
    allocateNoConstruct(nUsed);
    copyConstruct(data, data+nUsed, first);
}

/** Copy constructor allocates exactly as much memory as is in use in the 
source (not its capacity) and copy constructs the elements so that T's copy 
constructor will be called exactly src.size() times. If the source is empty, 
no heap space will be allocated. **/
Array_(const Array_& src) {
    nUsed = src.nUsed;
    allocateNoConstruct(nUsed);
    copyConstruct(data, data+nUsed, src.data);
}

/** Construct this Array_<T,X> as a copy of another Array_<T2,X2> where T2!=T
or X2!=X. This will work as long as the source is not larger than will fit 
here, and as long as the source element type T2 is assignment compatible with 
this array's element type T. One of T's constructors will be called exactly 
src.size() times; the particular constructor is whichever one best matches 
T(T2). **/
template <class T2, class X2>
Array_(const Array_<T2,X2>& src) {
    new (this) Array_(src.begin(), src.end()); // see above
}

/** It type T has a destructor, then the Array_<T> destructor calls T's 
destructor once for each element it contains before freeing the memory it is 
using. Built-in types like int do not have destructors so destruction is a
constant-time operation for them. **/
~Array_() {
    clear(); // each element is destructed; nUsed=0
    deallocateNoDestruct(); // free data; nAllocated=0 
}
/*@}              End of constructors and destructor */



/** @name           Assignment methods and operators

Assignment methods always begin by erasing all the elements currently in this 
array, then <em>copy constructing</em>, not <em>assigning</em> from the source.
This is therefore not exactly equivalent to elementwise assignment; it does not 
use the element type's copy assignment operator. We may reuse the existing heap 
allocation if it is sufficient and not \e too big; otherwise we'll reallocate 
before copying. **/
/*@{*/

/** Fill this array with n copies of the supplied fill value. Note that this
serves to allow fill from an object whose type T2 is different from T, as
long as there is a constructor T(T2) that works since that can be invoked
(implicitly or explicitly) to convert the T2 object to type T prior to the
call. **/ 
Array_& assign(size_type n, const T& fillValue) {
    SimTK_ERRCHK3_ALWAYS(isSizeOK(n), "Array_<T>::assign(n,T)",
        "Requested size %llu is too many for this which Array is limited"
        " to %llu elements by its index type %s.",
        (unsigned long long)(n), ullMaxSize(), index_name());

    clear(); // all elements destructed; allocation unchanged
    reallocateIfAdvisable(n); // change size if too small or too big
    fillConstruct(data, data+n, fillValue);
    nUsed = n;
    return *this;
}


/** Assign this array from a range [b,e) given by non-pointer iterators. If we
can determine how many elements n that represents in advance, we'll do only a 
single allocation here and call one of T's constructors exactly n times with no
destructor calls except to erase the original data. If these aren't random 
access iterators then we'll just have to add elements as we find them using 
push_back() meaning we may need to reallocate log(n) times. **/
template <class InputIterator>
Array_& assign(InputIterator first, InputIterator last1) {
    assignImpl(first, last1, 
               std::iterator_traits<InputIterator>::iterator_category());
    return *this;
}


/** Assign to this array to to make it a copy of the elements in range 
[first,last1) given by ordinary pointers. It is not allowed for this range to 
include any of the elements currently in the array. The source elements can be 
of a T2 that may be the same or different than this array's element type T as 
long as there is a constructor T(T2) that works. Note that although the source 
arguments are pointers, those may be iterators for some container depending on 
implementation details of the container. Specifically, any Array_<T2>::iterator 
is an ordinary pointer.

@par Complexity:
Say the array initially has size n and capacity c, and the source provides
m new elements. If type T has a destructor, it will be called exactly n times. 
Reallocation will then occur if c < m and may occur if c >> m. Then the
constructor T(T2) will be called exactly m times. **/
template <class T2>
Array_& assign(const T2* first, const T2* last1) {
    const char* methodName = "Array_<T>::assign(b,e)";
    SimTK_ERRCHK((first&&last1)||(first==last1), methodName, 
        "Pointers must be non-null unless they are both null.");
    SimTK_ERRCHK(last1<=begin() || end()<=first, methodName, 
        "Source pointers can't be within the destination Array.");
    // Pointers are random access iterators.
    assignImpl(first,last1,std::random_access_iterator_tag());
    return *this;
}

/** Copy assignment operator destructs the current contents of this array and 
then makes it a copy of the source array by repeated calls to the element 
type's copy constructor. At most one reallocation of heap space occurs that 
may result in this array having a larger or smaller capacity, although of 
course it will be at least as large as the source. **/
Array_& operator=(const Array_& src) {
    if (this != &src)
        assignImpl(src.begin(), src.end(), std::random_access_iterator_tag());
    return *this;
}

/** This is assignment from a source array whose element type T2 and/or index 
type X2 are different from this array's T and X. This will work as long as 
this array can accommodate all the elements in the source and T2 is assignment
compatible with T. See discussion for the copy assignment operator for more 
information. */
template <class T2, class X2>
Array_& operator=(const Array_<T2,X2>& src) {
    assignImpl(src.begin(), src.end(), std::random_access_iterator_tag());
    return *this;
}


/** This is assignment from a source std::vector<T2>. This will work as long as 
this array can accommodate all the elements in the source and T2 is assignment
compatible with T. See discussion for the copy assignment operator for more 
information. */
template <class T2, class A>
Array_& operator=(const std::vector<T2,A>& src) {
    assignImpl(src.begin(), src.end(), std::random_access_iterator_tag());
    return *this;
}
/*@}*/


/** This is a specialized algorithm providing constant time exchange of data 
with another array that has identical element and index types. This is \e much 
faster than using the std::swap() algorithm on the arrays since that would
involve O(n) copying operations. This method makes no calls to any constructors
or destructors. **/
void swap(Array_& other) {
    std::swap(data,other.data);
    std::swap(nUsed,other.nUsed);
    std::swap(nAllocated,other.nAllocated);
}

/** @name                   Size and capacity 

These methods examine and alter the number of elements or the amount of 
allocated heap space or both. **/
/*@{*/

/** The current number of elements stored in this array. **/
size_type size() const {return nUsed;}
/** The amount of heap space currently allocated to this array measured by the
number of elements it can hold without reallocation. The value returned by 
capacity() is always greater than or equal to size(). **/
size_type capacity() const {return nAllocated;}
/** Return true if there are no elements currently stored in this Array. This
is equivalent to the tests begin()==end() or size()==0. **/
bool empty() const {return nUsed==0;}

/** Change the size of this Array, preserving all the elements that will still 
fit, and default constructing any new elements that are added. **/
void resize(size_type sz) {
    if (sz == nUsed) return;
    if (sz == 0) {clear(); return;}
    if (sz < nUsed) {
        erase(&data[sz], end());
        return;
    }
    // sz > nUsed
    reserve(sz);
    defaultConstruct(data+nUsed, data+sz);
    nUsed = sz;
}

/** Change the size of this Array, preserving all the elements that will still 
fit, and initializing any new elements that are added by repeatedly copy-
constructing from the supplied value. **/
void resize(size_type sz, const T& initVal) {
    if (sz == nUsed) return;
    if (sz == 0) {clear(); return;}
    if (sz < nUsed) {
        erase(&data[sz], end());
        return;
    }
    // sz > nUsed
    reserve(sz);
    fillConstruct(data+nUsed, data+sz, initVal);
    nUsed = sz;
}

/** Ensure that this Array has enough allocated capacity to hold the indicated 
number of elements. No heap reallocation will occur after this until the Array
is grown beyond this capacity, meaning that adding elements will not invalidate
any iterators or element addresses until that point. This method will never 
reduce the capacity of the Array. **/
void reserve(size_type newCapacity) {
    if (nAllocated >= newCapacity)
        return;
    assert(newCapacity > nUsed);
    T* newdata = allocN(newCapacity); // no construction yet
    copyConstructThenDestructSource(newdata, newdata+nUsed, data);
    freeN(data);
    data = newdata;
    nAllocated = newCapacity;
}
/*@}*/

/** @name                       Iterators

These methods deal in iterators, which are STL generalized pointers. For this
class, iterators are just ordinary pointers to T, and you may depend on that.
By necessity, reverse iterators can't be just pointers; however, they contain 
an ordinary iterator (i.e. a pointer) that can be obtained by calling the 
reverse iterator's base() method. **/
/*@{*/

/** Return a const pointer to the first element of this array if any, otherwise
end(), which may be null (0) in that case but does not have to be. **/
const_iterator begin() const {return data;}
/** Return a const pointer to what would be the element just after the last one
in the array; this may be null (0) if there are no elements but doesn't have to
be. **/
const_iterator end() const {return data + nUsed;} // one past end
iterator begin() {return data;}
iterator end() {return &data[nUsed];} // one past end

const_reverse_iterator rbegin() const 
{   return const_reverse_iterator(end()); }
const_reverse_iterator rend() const 
{   return const_reverse_iterator(begin()); }
reverse_iterator rbegin()
{   return reverse_iterator(end()); }
reverse_iterator rend()
{   return reverse_iterator(begin()); }
/*@}*/

/** @name                     Element access

These methods provide access to individual elements that are currently present
in the array. **/
/*@{*/

/** Select an element by its index, returning a const reference. Note that only 
a value of the Array's templatized index type is allowed (default is int). This 
will be range-checked in a Debug build but not in Release.
@pre 0 <= \a i < size()
@par Complexity:
    Constant time. **/
const T& operator[](index_type i) const {
    SimTK_INDEXCHECK(i,nUsed,"Array_<T>::operator[]() const");
    return data[i];
}
/** Select an element by its index, returning a writable (lvalue) reference. 
Note that only a value of the Array's templatized index type is allowed 
(default is int). This will be range-checked in a Debug build but not 
in Release. 
@pre 0 <= \a i < size()
@par Complexity:
    Constant time. **/
T& operator[](index_type i) {
    SimTK_INDEXCHECK(i,nUsed,"Array_<T>::operator[]()");
    return data[i];
}
/** Same as operator[] but always range-checked, even in a Release build.  
@pre 0 <= \a i < size()
@par Complexity:
    Constant time. **/
const T& at(index_type i) const {
    SimTK_INDEXCHECK_ALWAYS(i,nUsed,"Array_<T>::at() const");
    return data[i];
}
/** Same as operator[] but always range-checked, even in a Release build.  
@pre 0 <= \a i < size()
@par Complexity:
    Constant time. **/
T& at(index_type i) {
    SimTK_INDEXCHECK_ALWAYS(i,nUsed,"Array_<T>::at()");
    return data[i];
}
/** Return a const reference to the first element in this array, which must
not be empty.
@pre The array is not empty.
@par Complexity:
    Constant time. **/
const T& front() const 
{   SimTK_ERRCHK(!empty(), "Array_<T>::front()", "Array was empty.");
    return data[0]; }
/** Return a writable reference to the first element in this array, which must
not be empty.
@pre The array is not empty.
@par Complexity:
    Constant time. **/
T& front() 
{   SimTK_ERRCHK(!empty(), "Array_<T>::front()", "Array was empty.");
    return data[0]; }
/** Return a const reference to the last element in this array, which must
not be empty.
@pre The array is not empty.
@par Complexity:
    Constant time. **/
const T& back() const 
{   SimTK_ERRCHK(!empty(), "Array_<T>::back()", "Array was empty.");
    return data[nUsed-1]; }
/** Return a writable reference to the last element in this array, which must
not be empty.
@pre The array is not empty.
@par Complexity:
    Constant time. **/
T& back() 
{   SimTK_ERRCHK(!empty(), "Array_<T>::back()", "Array was empty.");
    return data[nUsed-1]; }
/*@}*/

/** Erase all the elements currently in this Array without changing the capacity;
equivalent to erase(begin(),end()) but a little faster. Size is zero after this 
call. T's destructor is called exactly once for each element in the Array.

@par Complexity:
    O(n) if T has a destructor; constant time otherwise. **/
void clear() {
    destruct(begin(), end());
    nUsed = 0;
}

/**@name                Element insertion and removal

These are methods that change the number of elements in the array by insertion
or erasure. **/
/*@{*/

/** Erase elements in range [first,last1), packing in any later elements into 
the newly-available space and reducing the array's size by the number of 
elements erased. Capacity is unchanged. If the range is empty nothing happens.

@pre begin() <= \a first <= \a last1 <= end()
@param      first
    Points to the first element that will be erased.
@param      last1
    Points one element past the last element to be erased.
@return
    An iterator pointing to the new location of the element immediately
    following the erased ones, or end() if there are none. Either way, this is 
    the same memory address as the passed-in \a first argument since there can
    be no reallocation here.
@par Complexity:
    Calls T's destructor once for each erased element and calls T's copy 
    constructor and destructor once for each element that has to be moved. **/
T* erase(T* first, T* last1) {
    SimTK_ERRCHK(begin() <= first && first <= last1 && last1 <= end(),
    "Array<T>::erase(first,last1)", "Iterators out of range or out of order.");

    const size_type nErased = size_type(last1-first);
    if (nErased) {
        destruct(first, last1); // Destruct the elements we're erasing.
        moveElementsDown(last1, nErased); // Compress followers into the gap.
        nUsed -= nErased;
    }
    return first;
}

/** Erase just one element, moving all subsequent elements down one slot and
reducing the array's size by one. This is equivalent to erase(p,p+1) but faster;
that means \a p cannot be end() because end()+1 is not defined. Capacity is 
unchanged.

@note If you don't mind the elements being reordered, you can erase an element
in constant time using the non-standard extension eraseFast().

@pre begin() <= \a p < end()
@param      p
    Points to the element that will be erased; \a p cannot be end().
@return
    A pointer to the element that replaced the one at \a p, or end() if \a p 
    was the last element. Either way, this is the same memory address as the 
    erased element had since there can be no reallocation here.
@par Complexity:
    Calls T's destructor once for the erased element and calls T's copy 
    constructor and destructor once for each element that has to be moved. 
@see eraseFast() **/
T* erase(T* p) {
    SimTK_ERRCHK(begin() <= p && p < end(),
        "Array<T>::erase(p)", "Iterator must point to a valid element.");

    destruct(p);              // Destruct the element we're erasing.
    moveElementsDown(p+1, 1); // Compress followers into the gap.
    --nUsed;
    return p;
}


/** Be careful with this non-standard extension; it erases one element and 
then moves the last one in its place which changes the element order
from what it was before (unlike the standard erase() method). This avoids
having to compress the elements so this runs in constant time:
the element is destructed; then if it wasn't the last element the
copy constructor is used to copy the last element into the vacated
space, and the destructor is called to clear the last element. The
size is reduced by 1 but the capacity does not change. 

@pre begin() <= \a p < end()
@param      p
    Points to the element that will be erased; \a p cannot be end().
@return
    A pointer to the element that replaced the one at \a p, or end() if \a p 
    was the last element. Either way, this is the same memory address as the 
    erased element had since there can be no reallocation here.
@par Complexity:
    Calls T's destructor once for the erased element and calls T's copy 
    constructor and destructor once for each element that has to be moved.
@see erase() **/
T* eraseFast(T* p) {
    assert(begin() <= p && p < end());
    destruct(p);
    if (p+1 != end()) 
        moveOneElement(p, &back());
    --nUsed;
    return p;
}

/** Insert \a n copies of a given value at a particular location within this 
array, moving all following elements up by n positions.

@param[in]      p        
    Where to insert the new elements. This must be an iterator (pointer) that 
    is valid for this array, that is, begin() <= \a p <= end().
@param[in]      n
    How many copies of the given \a value to insert. Nothing happens if 
    \a n is zero.
@param[in]      value    
    A value of the element type that is copied into the newly-created elements
    using T's copy constructor.
@return         
    A pointer to the first of the newly-created elements in the array. This 
    will be different from \a p if reallocation occurred, otherwise it is the
    same as \a p was on entry.

@pre begin() <= \a p <= end()
@par Complexity:
    If size()+n > capacity() then the array must be reallocated, resulting in 
    size() copy constructor/destructor call pairs to move the old data to the
    new location. Otherwise, the m=(end()-\a p) elements above the insertion 
    point must be moved up n positions resulting in m copy/destruct pairs. Then
    there are n additional copy constructor calls to construct the new elements
    from the given value. 
**/
T* insert(T* p, size_type n, const T& value) {
    T* const gap = insertGapAt(p, n, "Array<T>::insert(p,n,value)");
    // Copy construct into the inserted elements and note the size change.
    fillConstruct(gap, gap+n, value);
    nUsed += n;
    return gap;
}

/** Insert a new element at a given location within this array, initializing 
it to a copy of a given value and moving all following elements up one 
position. This is identical to insert(\a p,1,\a value) but slightly faster; see
that method for full documentation. **/
T* insert(T* p, const T& value)  {
    T* const gap = insertGapAt(p, 1, "Array<T>::insert(p,value)");
    // Copy construct into the inserted element and note the size change.
    copyConstruct(gap, value);
    ++nUsed;
    return gap;
}

/** Insert elements in a range [first,last1) into this array at a given position
\a p, moving all following elements up by n=(last1-first) positions. This 
variant of insert() takes iterators which are ordinary pointers, although the
source elements do not have to be of type T as long as there is a constructor
T(T2) that works.

@param[in]      p        
    Where to insert the new elements. This must be an iterator (pointer) that 
    is valid for this array, that is, begin() <= \a p <= end().
@param[in]      first
    This is a pointer to the first element of the source to be copied.
@param[in]      last1    
    This points one element past the last element of the source to be copied.
@return         
    A pointer to the first of the newly-created elements in the array. This 
    will be different from \a p if reallocation occurred, otherwise it is the
    same as \a p was on entry.

@pre begin() <= \a p <= end()
@pre first <= last1
@pre The range [first,last1) does not include any of the current contents 
of this array.
@par Complexity:
    If capacity() < size()+n then the array must be reallocated, resulting in 
    size() calls to T's copy constructor and destructor (if any)to move the old
    data to the new location. Otherwise, the m=(end()-\a p) elements above the 
    insertion point must be moved up n positions resulting in m copy/destruct 
    pairs. Then there are n additional copy constructor calls to construct the 
    new elements from the given value. **/
template <class T2>
T* insert(T* p, T2* first, T2* last1) {
    const char* methodName = "Array_<T>::insert(p,first,last1)";
    SimTK_ERRCHK((first&&last1) || (first==last1), methodName, 
        "One of first or last1 was null; either both or neither must be null.");
    SimTK_ERRCHK(last1<=begin() || end()<=first, methodName, 
        "Source pointers can't be within the destination array.");
    // Pointers are random access iterators.
    return insertImpl(p,first,last1,std::random_access_iterator_tag());
}

/** This method increases the size of the Array by one element at the end and 
initializes that element by copy constructing it from the given value, just like
the std::vector::push_back() method. If capacity() > size(), that's all that 
will happen. If capacity()==size(), there is no room for another element so 
we'll allocate more space and move all the elements there. We return an
iterator pointing to the new element, which is also the element whose reference
would be returned by back() after this call.
@param[in]      value
    An object of type T from which the new element is copy-constructed.
@return 
    An iterator pointing to the newly added element; i.e., &back(). This is 
    non-standard; the standard push_back() is declared as a void function.

@notes
  - If you are appending a default-constructed object of type T, consider using
    the alternate non-standard but safe push_back() method rather than 
    push_back(T()). The non-standard method default-constructs the new element 
    internally. That saves a call to the copy constructor which can be expensive
    for some objects, and nonexistent for others.
  - If you are constructing the source object with a non-default constructor,
    and the object is expensive or impossible to default-construct and/or 
    copy-construct, consider using the non-standard and dangerous method 
    raw_push_back() which enables you to construct the new element in place. 

@par Complexity:
    Constant time if no reallocation is required; otherwise the current 
    contents of the array must be copied to new space, costing one call to T's
    copy constructor and destructor (if any) for each element currently in the
    array. Either way there is one call to T's copy constructor to construct 
    the new element from the supplied value. **/
T* push_back(const T& value) {
    if (nAllocated == nUsed)
        growAtEnd(1,"Array_<T>::push_back(elt)");
    T* const p = data + nUsed++;
    copyConstruct(p, value);
    return p;
}

/** This is a non-standard version of push_back() that increases the size of the
array by one default-constructed element at the end. This avoids having to 
default-construct the argument to the standard push_back() method which then has
to copy-construct it into the array. By carefully avoiding reallocation and
using this form of push_back() you can use the Array_<T> class to hold objects
of type T even if T has no copy constructor, which is prohibited by the 
std::vector<T> definition. 
@return 
    An iterator pointing to the newly added element; i.e., &back().
@par Complexity:
    Same as the standard push_back(value) method except without the final
    call to T's copy constructor.
@see push_back(value) 
**/
T* push_back() {
    if (nAllocated == nUsed)
        growAtEnd(1,"Array_<T>::push_back(elt)");
    T* const p = data + nUsed++;
    defaultConstruct(p);
    return p;
}


/** This dangerous method increases the Array's size by one element at the end 
but doesn't perform any construction so the memory is filled with garbage. You 
must immediately construct into this space, using code like:
@code       
    new(a.raw_push_back()) MyConstructor(...args...);       
@endcode
This is a substantial performance improvement when the element type is something
complicated since the constructor is called once and not copied; it can also be
used for objects that have neither default nor copy constructors.
@return 
    An iterator pointing at the unconstructed element. 
@par Complexity:
    Same as ordinary push_back().
@see push_back(value), push_back() 
**/
T* raw_push_back() {
    if (nAllocated == nUsed)
        growAtEnd(1,"Array_<T>::raw_push_back()");
    return data + nUsed++;
}

/** Remove the last element from this array, which must not be empty. The 
element is destructed, not returned. The array's size() is reduced by one. **/
void pop_back() {
    SimTK_ERRCHK(!empty(), "Array_<T>::pop_back()", "Array was empty.");
    destruct(data + --nUsed);
}
/*@}*/


//------------------------------------------------------------------------------
private:
//------------------------------------------------------------------------------

// This method is used when we have already decided we need to make room for 
// some new elements by reallocation, by creating a gap somewhere within the
// existing data. We'll issue an error message if this would violate the  
// max_size restriction (we can afford to do that even in a Release build since
// we don't expect to grow very often). Otherwise we'll allocate some more space
// and copy construct the existing elements into the new space, leaving a gap 
// where indicated. Note that this method does not change the current size but
// does change the capacity.
//
// The gapPos must point within the existing data with null OK if the array
// itself is null, and end() being OK any time although you should use the
// more specialized growAtEnd() method if you know that's what's happening.
//
// Don't call this with a gapSz of zero.
T* growWithGap(T* gapPos, size_type gapSz, const char* methodName) {
    assert(gapSz > 0); // <= 0 is a bug, not a user error

    // Note that gapPos may be null if begin() and end() are also.
    SimTK_ERRCHK(begin() <= gapPos && gapPos <= end(), methodName, 
        "Given insertion point is not valid for this array.");

    // Get some new space of a reasonable size.
    nAllocated   = calcNewCapacityForGrowthBy(gapSz, methodName);
    T* newData   = allocN(nAllocated);

    // How many elements will be before the gap?
    const size_type nBefore = gapPos-begin();

    // Locate the gap in the new space allocation.
    T* newGap    = newData + nBefore;
    T* newGapEnd = newGap  + gapSz; // one past the last element in the gap

    // Copy elements before insertion point; destruct source as we go.
    copyConstructThenDestructSource(newData,   newGap,        data);
    // Copy/destruct elements at and after insertion pt; leave gapSz gap.
    copyConstructThenDestructSource(newGapEnd, newData+nUsed, gapPos);

    // Throw away the old data and switch to the new.
    freeN(data); data = newData;
    return newGap;
}

// Same as growWithGap(end(), n, methodName); see above.
void growAtEnd(size_type n, const char* methodName) {
    assert(n > 0); // <= 0 is a bug, not a user error
    // Get some new space of a reasonable size.
    nAllocated   = calcNewCapacityForGrowthBy(n, methodName);
    T* newData   = allocN(nAllocated);
    // Copy all the elements; destruct source as we go.
    copyConstructThenDestructSource(newData, newData+nUsed, data);
    // Throw away the old data and switch to the new.
    freeN(data); data = newData;
}

// This method determines how much we should increase the array's capacity
// when asked to insert n elements, due to an insertion or push_back. We will
// generally allocate more new space than requested, in anticipation of
// further insertions. This has to be based on the current size so that only
// log(n) reallocations are performed to insert n elements one at a time. Our
// policy here is to at least double the capacity unless that would exceed 
// max_size(). There is also a minimum amount of allocation we'll do if the 
// current size is zero or very small. 
size_type calcNewCapacityForGrowthBy(size_type n, const char* methodName) const {
    SimTK_ERRCHK3_ALWAYS(isGrowthOK(n), methodName,
        "Can't grow this Array by %llu element(s) because it would"
        " then exceed the max_size of %llu set by its index type %s.",
        (unsigned long long)n, ullMaxSize(), index_name());

    // At this point we know that capacity()+n <= max_size().
    const size_type mustHave = capacity() + n;

    // Be careful not to overflow size_type as you could if you 
    // double capacity() rather than halving max_size().
    const size_type wantToHave = capacity() <= max_size()/2 
                                    ? 2*capacity() 
                                    : max_size();

    const size_type newCapacity = std::max(std::max(mustHave,wantToHave),
                                           minAlloc());
    return newCapacity;
}

// Insert an unconstructed gap of size n beginning at position p. The return
// value is a pointer to the first element in the gap. It will be p if no
// reallocation occurred, otherwise it will be pointing into the new data.
// On return nUsed will be unchanged although nAllocated may be bigger.
T* insertGapAt(T* p, size_type n, const char* methodName) {
    // Note that p may be null if begin() and end() are also.
    SimTK_ERRCHK(begin() <= p && p <= end(), methodName, 
        "Given insertion point is not valid for this Array.");

    if (n==0) return p; // nothing to do

    // Determine the number of elements before the insertion point and
    // the number at or afterwards (those must be moved up by one slot).
    const size_type before = p-begin(), after = end()-p;

    // Grow the container if necessary. Note that if we have to grow we
    // can create the gap at the same time we copy the old elements over
    // to the new space.
    if (capacity() >= size()+n) {
        moveElementsUp(p, n); // leave a gap at p
    } else { // need to grow
        nAllocated = calcNewCapacityForGrowthBy(n, methodName);
        T* newdata = allocN(nAllocated);
        // Copy the elements before the insertion point, and destroy source.
        copyConstructThenDestructSource(newdata, newdata+before, data);
        // Copy the elements at and after the insertion point, leaving a gap
        // of n elements.
        copyConstructThenDestructSource(newdata+before+n,
                                        newdata+before+n+after,
                                        p); // i.e., data+before
        p = newdata + before; // points into newdata now
        freeN(data);
        data = newdata;
    }

    return p;
}

// This is the slow generic insert() implementation for any input iterator that
// can't do random access (input, forward, bidirectional).
template <class InputIterator>
T* insertImpl(T* p, InputIterator first, InputIterator last1, 
              std::input_iterator_tag) 
{
    size_type nInserted = 0;
    while (first != last1) {
        p = insert(p, *first++);  // p may now point to reallocated memory
        ++p; ++nInserted;
    }
    // p now points just after the last inserted element; subtract the
    // number inserted to get a pointer to the first inserted element.
    return p-nInserted;
}

// This is the fast insert() implementation that works for random access
// iterators including ordinary pointers.
template <class RandomAccessIterator>
T* insertImpl(T* p, RandomAccessIterator first, RandomAccessIterator last1,
              std::random_access_iterator_tag) 
{
    const char* methodName = "Array_<T>::insert(p,first,last1)";
    SimTK_ERRCHK(first <= last1, methodName, "Iterators were out of order.");
    SimTK_ERRCHK3(isGrowthOK(last1-first), methodName,
        "Source has %llu elements which would make this Array exceeed the %llu"
        " elements allowed by its index type %s.",
        ull(last1-first), ullMaxSize(), index_name());

    const size_type n = size_type(last1-first);
    p = insertGapAt(p, n, methodName);
    copyConstruct(p, p+n, first);
    return p;
}

// This is the slow generic implementation for any input iterator that
// can't do random access (input, forward, bidirectional).
template <class InputIterator>
void assignImpl(InputIterator first, InputIterator last1, 
                   std::input_iterator_tag) 
{
    clear(); // TODO: change space allocation here?
    while (first != last1)
        push_back(*first++);
}

// This is the fast implementation that works for random access
// iterators including ordinary pointers. We can check here that the 
// iterators are in the right order, and that the source is not too big to
// fit in this array. Null pointer checks should be done prior to calling,
// however, since iterators in general aren't pointers.
template <class RandomAccessIterator>
void assignImpl(RandomAccessIterator first, RandomAccessIterator last1,
               std::random_access_iterator_tag) 
{
    const char* methodName = "Array_<T>::assign(first,last1)";
    SimTK_ERRCHK(first <= last1, methodName, "Iterators were out of order.");
    SimTK_ERRCHK3(isSizeOK(last1-first), methodName,
        "Source has %llu elements but this Array is limited to %llu"
        " elements by its index type %s.",
        ull(last1-first), ullMaxSize(), index_name());

    clear(); // all elements destructed; allocation unchanged
    nUsed = size_type(last1-first);
    reallocateIfAdvisable(nUsed); // change size if too small or too big
    copyConstruct(data, data+nUsed, first);
}

// We are going to put a total of n elements into the Array (probably
// because of an assignment or resize) and we want the space allocation
// to be reasonable. That means of course that the allocation must be 
// *at least* n, but we also don't want it to be too big. Our policy
// here is that if it is currently less than twice what we need we
// won't reallocate, otherwise we'll shrink the space. When changing
// the size to zero or something very small we'll treat the Array as
// though its current size is minAlloc, meaning we won't reallocate
// if the existing space is less than twice minAlloc.
// nAllocated will be set appropriately; nUsed is not touched here.
// No constructors or destructors are called.
void reallocateIfAdvisable(size_type n) {
    if (nAllocated < n || nAllocated/2 > std::max(minAlloc(), n)) 
        reallocateNoDestructOrConstruct(n);
}


void allocateNoConstruct(size_type n) 
{   data = allocN(n); nAllocated=n; }    // nUsed left unchanged
void deallocateNoDestruct() 
{   freeN(data); data=0; nAllocated=0; } // nUsed left unchanged
void reallocateNoDestructOrConstruct(size_type n)
{   deallocateNoDestruct(); allocateNoConstruct(n); }

// This sets the smallest allocation we'll do when growing.
size_type minAlloc() const 
{   return std::min(max_size(), size_type(4)); }

// Allocate without construction. Returns a null pointer if asked
// to allocate 0 elements. In Debug mode we'll fill memory with 
// all 1's as a bug catcher.
static T* allocN(size_type n) {
    if (n==0) return 0;
    unsigned char* newdata = new unsigned char[n * sizeof(T)];
    #ifndef NDEBUG
        unsigned char* b=newdata; 
        const unsigned char* e=newdata+(n*sizeof(T));
        while (b != e) *b++ = 0xff;
    #endif
    return reinterpret_cast<T*>(newdata);
}

// Free memory without calling destructors. Nothing happens if passed
// a null pointer.
static void freeN(T* p) {
    delete[] reinterpret_cast<char*>(p);
}

// default construct one element
static void defaultConstruct(T* p) {new(p) T();}
// default construct range [b,e)
static void defaultConstruct(T* b, T* e) 
{   while (b!=e) new(b++) T(); }

// copy construct range [b,e) with repeats of a given value
static void fillConstruct(T* b, const T* e, const T& v)
{   while(b!=e) new(b++) T(v); }

// copy construct one element from a given value
static void copyConstruct(T* p, const T& v) {new(p) T(v);}
// copy construct range [b,e) from sequence of source values
static void copyConstruct(T* b, const T* e, const T* src)
{   while(b!=e) new(b++) T(*src++); }
// Templatized copy construct will work if the source elements are
// assignment compatible with the destination elements.
template <class ForwardIterator>
static void copyConstruct(T* b, const T* e, ForwardIterator src)
{   while(b!=e) new(b++) T(*src++); }

// Copy construct range [b,e] from sequence of source values and
// destruct the source after it is copied. It's better to alternate
// copying and destructing than to do this in two passes since we
// will already have touched the memory.
static void copyConstructThenDestructSource
   (T* b, const T* const e, T* src)
{   while(b!=e) {new(b++) T(*src); src++->~T();} }

// We have an element at from that we would like to move into the currently-
// unconstructed slot at to. Both from and to are expected to be pointing to
// elements within the currently allocated space. From's slot will be left
// unconstructed.
void moveOneElement(T* to, T* from) {
    assert(data <= to   && to   < data+nAllocated);
    assert(data <= from && from < data+nAllocated);
    copyConstruct(to, *from); 
    destruct(from);
}


// Move elements from p to end() down by n places to fill an unconstructed gap
// beginning at p-n. Any leftover space at the end will be unconstructed.
void moveElementsDown(T* p, size_type n) {
    assert(n > 0);
    for (; p != end(); ++p)
        moveOneElement(p-n,p);
}

// Move elements from p to end() up by n places to make an unconstructed gap
// at [p,p+n). Note that this has to be done backwards so that we don't
// write on any elements until after they've been copied.
void moveElementsUp(T* p, size_type n) {
    assert(n > 0);
    T* src = end(); // points one past source element (begin()-1 not allowed)
    while (src != p) {
        --src; // now points to source
        moveOneElement(src+n, src);;
    }
}

// destruct one element
static void destruct(T* p) {p->~T();}
// destruct range [b,e)
static void destruct(T* b, const T* e)
{   while(b!=e) b++->~T(); }

// Check that a source object's size will fit in the Array being
// careful to avoid overflow and warnings in the comparison.
template <class S> 
bool isSizeOK(S srcSz) const
{   return ull(srcSz) <= ullMaxSize(); }

template <class S>
bool isGrowthOK(S n) const
{   return isSizeOK(ullCapacity() + ull(n)); }

// Cast an integral type to maximal-width unsigned long long to avoid accidental
// overflows that might otherwise occur due to wraparound that can happen 
// with small index types.
template <class S>
static unsigned long long ull(S sz)
{   return (unsigned long long)sz; }

// Return size(), capacity(), and max_size() cast to unsigned long long.
unsigned long long ullSize()     const {return ull(size());}
unsigned long long ullCapacity() const {return ull(capacity());}
unsigned long long ullMaxSize()  const {return ull(max_size());}


//------------------------------------------------------------------------------
private: //                     DATA MEMBERS
//------------------------------------------------------------------------------
// These are the only data members and this layout is guaranteed not to change
// from release to release. If data is null, then nUsed==nAllocated==0.
T*                  data;       // pointer to the first element, or null
size_type           nUsed;      // number of elements currently present (size)
size_type           nAllocated; // current element capacity, >= nUsed
};



//------------------------------------------------------------------------------
//                          RELATED GLOBAL OPERATORS
//------------------------------------------------------------------------------
// These are logically part of the Array_<T,X> class but are not actually 
// class members; that is, they are in the SimTK namespace.

/** Output a human readable representation of an Array to an std::ostream
(like std::cout). The format is '{' \e elements '}' where \e elements is a 
space-separated list of the Array's contents output by invoking the "<<" 
operator on the elements. This function will not compile if the element type 
does not support the "<<" operator. No newline is issued before or after the 
output.
@relates Array_ **/
template <class T, class X> inline std::ostream&
operator<<(std::ostream& o, const Array_<T,X>& a) {
    o << '{';
    if (!a.empty()) {
        o << a.front();
        for (const T* p = a.begin()+1; p != a.end(); ++p)
            o << ' ' << *p;
    }
    return o << '}';
} 


} // namespace SimTK
  
#endif // SimTK_SimTKCOMMON_ARRAY_H_
