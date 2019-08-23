/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 WPL, Inc.
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
 * Author: Jared Dulmage <jared.dulmage@wpli.net>
 */

#ifndef ATTRIBUTE_CONTAINER_H
#define ATTRIBUTE_CONTAINER_H

#include <ns3/ptr.h>
#include <ns3/string.h>

#include <list>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <typeinfo>
#include <type_traits>
#include <utility>

namespace ns3 {

class AttributeChecker;

// A = attribute value type, C = container type to return
template <class A, template<class...> class C=std::list>
class AttributeContainerValue : public AttributeValue
{
public:
  typedef A attribute_type;
  typedef Ptr<A> value_type;
  typedef std::list<value_type> container_type;
  typedef typename container_type::const_iterator const_iterator;
  typedef typename container_type::iterator iterator;
  typedef typename container_type::size_type size_type;
  typedef typename AttributeContainerValue::const_iterator Iterator; // NS3 type

  // use underlying AttributeValue to get return element type
  typedef typename std::result_of<decltype(&A::Get)(A)>::type item_type;
  typedef C<item_type> result_type;

  AttributeContainerValue (char sep = ',');

  template <class CONTAINER>
  AttributeContainerValue (const CONTAINER &c);

  template <class ITER>
  AttributeContainerValue (const ITER begin, const ITER end);

  ~AttributeContainerValue ();

  Ptr<AttributeValue> Copy (void) const;

  bool DeserializeFromString (std::string value, Ptr<const AttributeChecker > checker);
  std::string SerializeToString (Ptr<const AttributeChecker> checker) const;

  // defacto pure virtuals to integrate with built-in accessor code
  result_type Get (void) const;
  template <class T>
  void Set (const T &c);

  // NS3 interface
  size_type GetN (void) const;
  Iterator Begin (void);
  Iterator End (void);

  size_type size (void) const;
  iterator begin (void);
  iterator end (void);
  const_iterator begin (void) const;
  const_iterator end (void) const;

private:
  template <class ITER>
  Ptr<AttributeContainerValue<A, C> > CopyFrom (const ITER begin, const ITER end);

  char m_sep;
  container_type m_container;
};

template <class A, template <class...> class C>
class AttributeContainerChecker : public AttributeChecker
{
public:
  virtual void SetItemChecker (Ptr<const AttributeChecker> itemchecker) = 0;
  virtual Ptr<const AttributeChecker> GetItemChecker (void) const = 0;
};

/**
 * Return checker using attribute value to provide types
 */
template <class A, template <class...> class C>
Ptr<AttributeContainerChecker<A, C> >
MakeAttributeContainerChecker (const AttributeContainerValue<A, C> &value);

/**
 * Return checker give types explicitly, defaults same
 * as AttributeContainerValue defaults
 */
template <class A, template <class...> class C=std::list>
Ptr<AttributeContainerChecker<A, C> >
MakeAttributeContainerChecker (Ptr<const AttributeChecker> itemchecker);

template <class A, template <class...> class C=std::list>
Ptr<AttributeContainerChecker<A, C> > MakeAttributeContainerChecker (void);

template <typename A, template <typename...> class C=std::list, typename T1>
Ptr<const AttributeAccessor> MakeAttributeContainerAccessor (T1 a1);

} // namespace ns3

/*****************************************************************************
 * Implementation below
 *****************************************************************************/

namespace ns3 {

namespace internal {

template <class A, template <class...> class C>
class AttributeContainerChecker : public ns3::AttributeContainerChecker<A, C>
{
public:
  AttributeContainerChecker (void);
  explicit AttributeContainerChecker (Ptr<const AttributeChecker> itemchecker);
  void SetItemChecker (Ptr<const AttributeChecker> itemchecker);
  Ptr<const AttributeChecker> GetItemChecker (void) const;

private:
  Ptr<const AttributeChecker> m_itemchecker;
};

template <class A, template <class...> class C>
AttributeContainerChecker<A, C>::AttributeContainerChecker (void)
  : m_itemchecker (0)
{}

template <class A, template <class...> class C>
AttributeContainerChecker<A, C>::AttributeContainerChecker (Ptr<const AttributeChecker> itemchecker)
  : m_itemchecker (itemchecker)
{}

template <class A, template <class...> class C>
void
AttributeContainerChecker<A, C>::SetItemChecker (Ptr<const AttributeChecker> itemchecker)
{
  m_itemchecker = itemchecker;
}

template <class A, template <class...> class C>
Ptr<const AttributeChecker>
AttributeContainerChecker<A, C>::GetItemChecker (void) const
{
  return m_itemchecker;
}

} // namespace internal

template <class A, template <class...> class C>
Ptr<AttributeContainerChecker<A, C> >
MakeAttributeContainerChecker (const AttributeContainerValue<A, C> &value)
{
  return MakeAttributeContainerChecker <A, C> ();
}

template <class A, template <class...> class C>
Ptr<AttributeContainerChecker<A, C> >
MakeAttributeContainerChecker (Ptr<const AttributeChecker> itemchecker)
{
  auto checker = MakeAttributeContainerChecker <A, C> ();
  checker->SetItemChecker (itemchecker);
  return checker;
}

template <class A, template <class...> class C>
Ptr<AttributeContainerChecker<A, C> >
MakeAttributeContainerChecker (void)
{
  std::string typeName, underlyingType;
  typedef AttributeContainerValue<A, C> T;
  {
    std::ostringstream oss;
    oss << "ns3::AttributeContainerValue<" << typeid (typename T::attribute_type).name ()
        << ", " << typeid (typename T::container_type).name () << ">";
    typeName = oss.str ();
  }

  {
    std::ostringstream oss;
    oss << "ns3::Ptr<" << typeid (typename T::attribute_type).name () << ">";
    underlyingType = oss.str ();
  }

  return DynamicCast<AttributeContainerChecker<A, C> > (
    MakeSimpleAttributeChecker<T, internal::AttributeContainerChecker<A, C> > (typeName, underlyingType)
  );
}

template <class A, template <class...> class C>
AttributeContainerValue<A, C>::AttributeContainerValue (char sep)
  : m_sep (sep)
{

}

template <class A, template <class...> class C>
template <class CONTAINER>
AttributeContainerValue<A, C>::AttributeContainerValue (const CONTAINER &c)
  : AttributeContainerValue<A, C> (c.begin (), c.end ())
{

}

template <class A, template <class...> class C>
template <class ITER>
AttributeContainerValue<A, C>::AttributeContainerValue (const ITER begin, const ITER end)
  : AttributeContainerValue ()
{
  CopyFrom (begin, end);
}

template <class A, template <class...> class C>
AttributeContainerValue<A, C>::~AttributeContainerValue ()
{
  m_container.clear ();
}

template <class A, template <class...> class C>
Ptr<AttributeValue>
AttributeContainerValue<A, C>::Copy (void) const
{
  auto c = Create<AttributeContainerValue<A, C> > ();
  c->m_sep = m_sep;
  c->m_container = m_container;
  return c;
}

template <class A, template <class...> class C>
bool
AttributeContainerValue<A, C>::DeserializeFromString (std::string value, Ptr<const AttributeChecker> checker)
{
  auto acchecker = DynamicCast<const AttributeContainerChecker<A, C> > (checker);
  if (!acchecker) return false;

  std::istringstream iss (value); // copies value
  while (std::getline (iss, value, m_sep))
    {
      auto avalue = acchecker->GetItemChecker ()->CreateValidValue (StringValue (value));
      if (!avalue) return false;

      auto attr = DynamicCast <A> (avalue);
      if (!attr) return false;

      // TODO(jared): make insertion more generic?
      m_container.push_back (attr);
    }
  return true;
}

template <class A, template <class...> class C>
std::string
AttributeContainerValue<A, C>::SerializeToString (Ptr<const AttributeChecker> checker) const
{
  std::ostringstream oss;
  bool first = true;
  for (auto attr: *this)
    {
      if (!first) oss << m_sep;
      oss << attr->SerializeToString (checker);
      first = false;
    }
  return oss.str ();
}

template <class A, template <class...> class C>
typename AttributeContainerValue<A, C>::result_type
AttributeContainerValue<A, C>::Get (void) const
{
  result_type c;
  for (const value_type a: *this)
    c.insert (c.end (), a->Get ());
  return c;
}

template <class A, template <class...> class C>
template <class T>
void
AttributeContainerValue<A, C>::Set (const T &c)
{
  m_container.clear ();
  CopyFrom (c.begin (), c.end ());
}

template <class A, template <class...> class C>
typename AttributeContainerValue<A, C>::size_type
AttributeContainerValue<A, C>::GetN (void) const
{
  return size ();
}

template <class A, template <class...> class C>
typename AttributeContainerValue<A, C>::Iterator
AttributeContainerValue<A, C>::Begin (void)
{
  return begin ();
}

template <class A, template <class...> class C>
typename AttributeContainerValue<A, C>::Iterator
AttributeContainerValue<A, C>::End (void)
{
  return end ();
}

template <class A, template <class...> class C>
typename AttributeContainerValue<A, C>::size_type
AttributeContainerValue<A, C>::size (void) const
{
  return m_container.size ();
}

template <class A, template <class...> class C>
typename AttributeContainerValue<A, C>::iterator
AttributeContainerValue<A, C>::begin (void)
{
  return m_container.begin ();
}

template <class A, template <class...> class C>
typename AttributeContainerValue<A, C>::iterator
AttributeContainerValue<A, C>::end (void)
{
  return m_container.end ();
}

template <class A, template <class...> class C>
typename AttributeContainerValue<A, C>::const_iterator
AttributeContainerValue<A, C>::begin (void) const
{
  return m_container.cbegin ();
}

template <class A, template <class...> class C>
typename AttributeContainerValue<A, C>::const_iterator
AttributeContainerValue<A, C>::end (void) const
{
  return m_container.cend ();
}

template <class A, template <class...> class C>
template <class ITER>
Ptr<AttributeContainerValue<A, C> >
AttributeContainerValue<A, C>::CopyFrom (const ITER begin, const ITER end)
{
  for (ITER iter = begin; iter != end; ++iter)
    {
      m_container.push_back (Create<A> (*iter));
    }
  return this;
}

template <typename A, template <typename...> class C, typename T1>
Ptr<const AttributeAccessor> MakeAttributeContainerAccessor (T1 a1)
{
  return MakeAccessorHelper<AttributeContainerValue<A, C> > (a1);
}

} // namespace ns3

#endif // ATTRIBUTE_CONTAINER_H