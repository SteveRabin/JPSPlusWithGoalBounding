/*
 * GenericHeap.h
 *
 * Copyright (c) 2007, Nathan Sturtevant
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University of Alberta nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY NATHAN STURTEVANT ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <copyright holder> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */ 

#ifdef _MSC_VER
#include "stdafx.h"
#include <hash_map>
#else
#include <ext/hash_map>
#endif

#ifndef HEAP2_H
#define HEAP2_H

#include <cassert>
#include <vector>
#include <stdint.h>

/**
 * Generic Heap
 *
* A relatively simple heap class. Uses a second hash table so that
 * keys can be looked up and re-heaped in constant time instead of
 * linear time.
 *
 * The EqKey and CmpKey functions were required for the gnu hash_map, but
 * aren't compatible with the windows hash_map. So, there is some
 * unnecessary code here which I just haven't re-written.
 */
template<typename Obj, class HashKey, class EqKey, class CmpKey>
class GenericHeap {
public:
    GenericHeap();
    ~GenericHeap();
    void reset();
    void add(Obj val);
    void decreaseKey(Obj val);
    bool isIn(Obj val);
    Obj remove();
    void pop() { remove(); }
    Obj top() { return _elts[0]; }
    Obj find(Obj val);
    bool empty();
    uint32_t size() { return (uint32_t)_elts.size(); }
	void reserve(unsigned int c) { _elts.reserve(c); }
private:
    std::vector<Obj> _elts;
    void heapifyUp(unsigned int index);
    void heapifyDown(unsigned int index);
#ifdef _MSC_VER
    typedef stdext::hash_map<size_t, unsigned int > IndexTable;
#else
//    // this was my original definition, but to maintain compatibility with
//    // microsoft's hash_map, we now have to require a perfect hash function
//    typedef __gnu_cxx::hash_map<Obj, unsigned int, HashKey, EqKey > IndexTable;
    typedef __gnu_cxx::hash_map<size_t, unsigned int > IndexTable;
#endif
    IndexTable table;
};


template<typename Obj, class HashKey, class EqKey, class CmpKey>
GenericHeap<Obj, HashKey, EqKey, CmpKey>::GenericHeap()
{
}

template<typename Obj, class HashKey, class EqKey, class CmpKey>
GenericHeap<Obj, HashKey, EqKey, CmpKey>::~GenericHeap()
{
}

/**
 * GenericHeap::reset()
 *
 * \brief Remove all objects from queue.
 *
 * \return none
 */
template<typename Obj, class HashKey, class EqKey, class CmpKey>
void GenericHeap<Obj, HashKey, EqKey, CmpKey>::reset()
{
    table.clear();
    _elts.resize(0);
}

/**
* GenericHeap::add()
 *
 * \brief Add object into GenericHeap.
 *
 * \param val Object to be added
 * \return none
 */
template<typename Obj, class HashKey, class EqKey, class CmpKey>
void GenericHeap<Obj, HashKey, EqKey, CmpKey>::add(Obj val)
{
    HashKey hk;

    assert(table.find(hk(val)) == table.end());
    table[hk(val)] = (unsigned int)_elts.size();
    _elts.push_back(val);
    heapifyUp(table[hk(val)]);
}

/**
* GenericHeap::decreaseKey()
 *
 * \brief Indicate that the key for a particular object has decreased.
 *
 * \param val Object which has had its key decreased
 * \return none
 */
template<typename Obj, class HashKey, class EqKey, class CmpKey>
void GenericHeap<Obj, HashKey, EqKey, CmpKey>::decreaseKey(Obj val)
{

// otherwise we get an unused variable warning when compiling in release mode
#ifndef NDEBUG 
    EqKey eq;
#endif
    HashKey hk;

    assert(eq(_elts[table[hk(val)]], val));
    _elts[table[hk(val)]] = val;
    heapifyUp(table[hk(val)]);
}

/**
* GenericHeap::isIn()
 *
 * \brief Returns true if the object is in the GenericHeap.
 *
 * \param val Object to be tested
 * \return true if the object is in the heap
 */
template<typename Obj, class HashKey, class EqKey, class CmpKey>
bool GenericHeap<Obj, HashKey, EqKey, CmpKey>::isIn(Obj val)
{
    EqKey eq;
    HashKey hk;

    if ((table.find(hk(val)) != table.end()) && (table[hk(val)] < _elts.size()) &&
        (eq(_elts[table[hk(val)]], val)))
        return true;
    return false;
}

/**
* GenericHeap::remove()
 *
 * \brief Remove the item with the lowest key from the heap & re-heapify.
 *
 * \return Object with lowest key
 */
template<typename Obj, class HashKey, class EqKey, class CmpKey>
Obj GenericHeap<Obj, HashKey, EqKey, CmpKey>::remove()
{
    HashKey hk;

    if (empty())
        return Obj();
    Obj ans = _elts[0];
    _elts[0] = _elts[_elts.size()-1];
    table[hk(_elts[0])] = 0;
    _elts.pop_back();
    table.erase(hk(ans));
    heapifyDown(0);
    
    return ans;
}

/**
* GenericHeap::find()
 *
 * \brief Find this object in the heap and return
 *
 * The object passed in only needs enough fields filled in to find
 * the hash key of the object. All fields will be filled in when returned.
 *
 * \param val Object to find
 * \return The same object from the heap.
 */
template<typename Obj, class HashKey, class EqKey, class CmpKey>
Obj GenericHeap<Obj, HashKey, EqKey, CmpKey>::find(Obj val)
{
    HashKey hk;

    if (!isIn(val))
        return Obj();
    return _elts[table[hk(val)]];
}

/**
 * GenericHeap::empty()
 *
 * \brief Returns true if no items are in the heap.
 *
 * \return true if no items are in the heap.
 */
/**
* Returns true if no items are in the GenericHeap.
 */
template<typename Obj, class HashKey, class EqKey, class CmpKey>
bool GenericHeap<Obj, HashKey, EqKey, CmpKey>::empty()
{
    return _elts.size() == 0;
}
    
/**
* GenericHeap::heapifyUp()
 *
 * \brief Check the object at the current index and see if it needs to move up the heap.
 *
 * An object moves up if it has a lower key than its parent.
 *
 * \param index Current index
 * \return none
 */
template<typename Obj, class HashKey, class EqKey, class CmpKey>
void GenericHeap<Obj, HashKey, EqKey, CmpKey>::heapifyUp(unsigned int index)
{
    if (index == 0) return;
    int parent = (index-1)/2;
    CmpKey compare;
    HashKey hk;
    
    if (compare(_elts[parent], _elts[index]))
    {
        Obj tmp = _elts[parent];
        _elts[parent] = _elts[index];
        _elts[index] = tmp;
        table[hk(_elts[parent])] = parent;
        table[hk(_elts[index])] = index;
#ifndef NDEBUG
        EqKey eq;
#endif
        assert(!eq(_elts[parent], _elts[index]));
        heapifyUp(parent);
    }
}

/**
* GenericHeap::heapifyDown()
 *
 * \brief Check the object at the current index and see if it needs to move down the heap.
 *
 * If an object has a higher key than one or more of its children, it is swapped
 * with its lowest-valued child.
 *
 * \param index Current index
 * \return none
 */
template<typename Obj, class HashKey, class EqKey, class CmpKey>
void GenericHeap<Obj, HashKey, EqKey, CmpKey>::heapifyDown(unsigned int index)
{
    HashKey hk;
    CmpKey compare;
    unsigned int child1 = index*2+1;
    unsigned int child2 = index*2+2;
    int which;
    unsigned int count = (unsigned int)_elts.size();
    // find smallest child
    if (child1 >= count)
        return;
    else if (child2 >= count)
        which = child1;
    //else if (fless(_elts[child1]->getKey(), _elts[child2]->getKey()))
    else if (!(compare(_elts[child1], _elts[child2])))
        which = child1;
    else
        which = child2;
    
    //if (fless(_elts[which]->getKey(), _elts[index]->getKey()))
    if (!(compare(_elts[which], _elts[index])))
    {
        Obj tmp = _elts[which];
        _elts[which] = _elts[index];
        table[hk(_elts[which])] = which;
        _elts[index] = tmp;
        table[hk(_elts[index])] = index;
        //    _elts[which]->key = which;
        //    _elts[index]->key = index;
        heapifyDown(which);
    }
}

#endif
