/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 Caliola Engineering, LLC.
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
 * Author: Jared Dulmage <jared.dulmage@caliola.com>
 */

#ifndef ATTRIBUTE_CONTAINER_ACCESSOR_HELPER_H
#define ATTRIBUTE_CONTAINER_ACCESSOR_HELPER_H

#include "attribute-container.h"

#include <ns3/attribute-helper.h>


#include <type_traits>
#include <list>

namespace ns3 {

/**
 * \ingroup attributeimpl
 *
 * DoMakeAccessorHelperOne specialization for member containers
 *
 * The template parameter list contains an extra parameter that disambiguates
 * the templated Ptr type from container types by taking advantage of the fact
 * that container types have multiple template arguments and Ptr has only one.
 * This disambiguation works but is not sufficiently robust as a long term solution.
 *
 * \tparam V  \explicit The specific AttributeValue type to use to represent
 *            the Attribute.
 * \tparam T  \deduced The class holding the data member.
 * \tparam U  \deduced The type of the container.
 * \tparam I  \deduced The type of item (s) within the container.
 * \param [in]  memberContainer  The address of the data member.
 * \returns The AttributeAccessor.
 */
template <typename V, typename T, template <typename...> class U, typename ...I,
          typename = typename std::enable_if< (sizeof...(I) > 1), void>::type >
inline
Ptr<const AttributeAccessor>
DoMakeAccessorHelperOne (U<I...> T::*memberContainer)
{
  /* AttributeAcessor implementation for a class member variable. */
  class MemberContainer : public AccessorHelper<T,V>
  {
    public:
    /*
     * Construct from a class data member address.
     * \param [in] memberContainer The class data member address.
     */
    MemberContainer (U<I...> T::*memberContainer)
      : AccessorHelper<T,V> (),
        m_memberContainer (memberContainer)
    {}
    private:
    virtual bool DoSet (T *object, const V *v) const {
      // typename AccessorTrait<U<I>::value_type>::Result tmp;
      // bool ok = v->GetAccessor (tmp);
      // if (!ok)
      //   {
      //     return false;
      //   }
      auto src = v->Get ();
      (object->*m_memberContainer).clear ();
      std::copy (src.begin (), src.end (), std::inserter ((object->*m_memberContainer), (object->*m_memberContainer).end ()));
      return true;
    }
    virtual bool DoGet (const T *object, V *v) const {
      v->Set (object->*m_memberContainer);
      return true;
    }
    virtual bool HasGetter (void) const {
      return true;
    }
    virtual bool HasSetter (void) const {
      return true;
    }

    U<I...> T::*m_memberContainer;  // Address of the class data member.
  };
  return Ptr<const AttributeAccessor> (new MemberContainer (memberContainer), false);
}

} // namespace ns3

#endif  // ATTRIBUTE_CONTAINER_ACCESSOR_HELPER_H