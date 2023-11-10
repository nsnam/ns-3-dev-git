/*
 * Copyright (c) 2024 Universita' degli Studi di Napoli Federico II
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef SHUFFLE_H
#define SHUFFLE_H

/**
 * \file
 * \ingroup randomvariable
 * Function to shuffle elements in a given range.
 */

#include "random-variable-stream.h"

#include <algorithm>
#include <iterator>

namespace ns3
{

/**
 * Shuffle the elements in the range <i>first</i> to <i>last</i>. Given that the implementation
 * of std::shuffle is not dictated by the standard
 * [CppReference](https://en.cppreference.com/w/cpp/algorithm/random_shuffle), it is not guaranteed
 * that std::shuffle returns the same permutation of the given range using different compilers/OSes.
 * Therefore, this function provides a specific implementation of the shuffling algorithm reported
 * in "The Art of Computer Programming" of Donald Knuth and on
 * [Wikipedia](https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle#The_modern_algorithm),
 * which basically matches the implementation in libstdc++.
 *
 * The complexity of the implemented algorithm is linear with the distance between <i>first</i>
 * and <i>last</i>. For containers that do not provide random access iterators (e.g., lists, sets)
 * we can still achieve a linear complexity by copying the elements in a vector and shuffling the
 * elements of the vector.
 *
 * \tparam RND_ACCESS_ITER \deduced the iterator type (must be a random access iterator)
 * \param first an iterator pointing to the first element in the range to shuffle
 * \param last an iterator pointing to past-the-last element in the range to shuffle
 * \param rv pointer to a uniform random variable
 */
template <typename RND_ACCESS_ITER>
void
Shuffle(RND_ACCESS_ITER first, RND_ACCESS_ITER last, Ptr<UniformRandomVariable> rv)
{
    if (auto dist = std::distance(first, last); dist > 1)
    {
        for (--last; first < last; ++first, --dist)
        {
            if (auto i = rv->GetInteger(0, dist - 1); i != 0)
            {
                std::iter_swap(first, std::next(first, i));
            }
        }
    }
}

} // namespace ns3

#endif /* SHUFFLE_H */
