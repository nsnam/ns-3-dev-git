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

#ifndef PAIR_H
#define PAIR_H

#include <ns3/attribute.h>
#include <ns3/ptr.h>
#include <ns3/string.h>

#include <sstream>
#include <typeinfo> // typeid
#include <type_traits>
#include <utility>

namespace ns3 {

class AttributeChecker;

// TODO: useful to maybe define some kind of configurable formatter?
template <class A, class B>
std::ostream &
operator << (std::ostream &os, const std::pair<A, B> &p)
{
  os << "(" << p.first << "," << p.second << ")";
  return os;
}

// A = attribute value type, C = container type to return
template <class A, class B>
class PairValue : public AttributeValue
{
public:
  typedef std::pair<Ptr<A>, Ptr<B> > value_type;
  typedef typename std::result_of<decltype(&A::Get)(A)>::type first_type;
  typedef typename std::result_of<decltype(&B::Get)(B)>::type second_type;
  typedef typename std::pair<first_type, second_type> result_type;
  typedef typename std::pair<const first_type, second_type> map_value_type;

  PairValue ();
  PairValue (const result_type &value); // "import" constructor

  Ptr<AttributeValue> Copy (void) const;

  bool DeserializeFromString (std::string value, Ptr<const AttributeChecker> checker);
  std::string SerializeToString (Ptr<const AttributeChecker> checker) const;

  result_type Get (void) const;
  void Set (const result_type &value);

  template <typename T>
  bool GetAccessor (T &value) const;

private:
  value_type m_value;
};

template <class A, class B>
class PairChecker : public AttributeChecker
{
public:
  typedef std::pair<Ptr<const AttributeChecker>, Ptr<const AttributeChecker> > checker_pair_type;

  virtual void SetCheckers (Ptr<const AttributeChecker> firstchecker, Ptr<const AttributeChecker> secondchecker) = 0;
  virtual checker_pair_type GetCheckers (void) const = 0;
};

template <class A, class B>
Ptr<PairChecker<A, B> >
MakePairChecker (const PairValue<A, B> &value);

template <class A, class B>
Ptr<PairChecker<A, B> >
MakePairChecker (Ptr<const AttributeChecker> firstchecker, Ptr<const AttributeChecker> secondchecker);

template <class A, class B>
Ptr<PairChecker<A, B> > MakePairChecker (void);

template <typename A, typename B, typename T1>
Ptr<const AttributeAccessor> MakePairAccessor (T1 a1);

}   // namespace ns3

/*****************************************************************************
 * Implementation below
 *****************************************************************************/

namespace ns3 {

namespace internal {

template <class A, class B>
class PairChecker : public ns3::PairChecker<A, B>
{
public:
  PairChecker (void);
  PairChecker (Ptr<const AttributeChecker> firstchecker, Ptr<const AttributeChecker> secondchecker);
  void SetCheckers (Ptr<const AttributeChecker> firstchecker, Ptr<const AttributeChecker> secondchecker);
  typename ns3::PairChecker<A, B>::checker_pair_type GetCheckers (void) const;

private:
  Ptr<const AttributeChecker> m_firstchecker;
  Ptr<const AttributeChecker> m_secondchecker;
};

template <class A, class B>
PairChecker<A, B>::PairChecker (void)
  : m_firstchecker (0),
  m_secondchecker (0)
{}

template <class A, class B>
PairChecker<A, B>::PairChecker (Ptr<const AttributeChecker> firstchecker, Ptr<const AttributeChecker> secondchecker)
  : m_firstchecker (firstchecker),
  m_secondchecker (secondchecker)
{}

template <class A, class B>
void
PairChecker<A, B>::SetCheckers (Ptr<const AttributeChecker> firstchecker, Ptr<const AttributeChecker> secondchecker)
{
  m_firstchecker = firstchecker;
  m_secondchecker = secondchecker;
}

template <class A, class B>
typename ns3::PairChecker<A, B>::checker_pair_type
PairChecker<A, B>::GetCheckers (void) const
{
  return std::make_pair (m_firstchecker, m_secondchecker);
}

} // namespace internal

template <class A, class B>
Ptr<PairChecker<A, B> >
MakePairChecker (const PairValue<A, B> &value)
{
  return MakePairChecker <A, B> ();
}

template <class A, class B>
Ptr<PairChecker<A, B> >
MakePairChecker (Ptr<const AttributeChecker> firstchecker, Ptr<const AttributeChecker> secondchecker)
{
  auto checker = MakePairChecker <A, B> ();
  checker->SetCheckers (firstchecker, secondchecker);
  return checker;
}

template <class A, class B>
Ptr<PairChecker<A, B> >
MakePairChecker (void)
{
  std::string pairName, underlyingType;
  typedef PairValue<A, B> T;
  std::string first_type_name = typeid (typename T::value_type::first_type).name ();
  std::string second_type_name = typeid (typename T::value_type::second_type).name ();
  {
    std::ostringstream oss;
    oss << "ns3::PairValue<" << first_type_name << ", " << second_type_name << ">";
    pairName = oss.str ();
  }

  {
    std::ostringstream oss;
    oss << typeid (typename T::value_type).name ();
    underlyingType = oss.str ();
  }

  return DynamicCast<PairChecker<A, B> > (
    MakeSimpleAttributeChecker<T, internal::PairChecker<A, B> > (pairName, underlyingType)
    );
}

template <class A, class B>
PairValue<A, B>::PairValue ()
  : m_value (std::make_pair (Create <A> (), Create <B> ()))
{}

template <class A, class B>
PairValue<A, B>::PairValue (const typename PairValue<A, B>::result_type &value)
{
  Set (value);
}

template <class A, class B>
Ptr<AttributeValue>
PairValue<A, B>::Copy (void) const
{
  auto p = Create <PairValue <A, B> > ();
  // deep copy if non-null
  if (m_value.first)
    p->m_value = std::make_pair (DynamicCast<A> (m_value.first->Copy ()),
                                 DynamicCast<B> (m_value.second->Copy ()));
  return p;
}

template <class A, class B>
bool
PairValue<A, B>::DeserializeFromString (std::string value, Ptr<const AttributeChecker> checker)
{
  auto pchecker = DynamicCast<const PairChecker<A, B> > (checker);
  if (!pchecker) return false;

  std::istringstream iss (value);  // copies value
  iss >> value;
  auto first = pchecker->GetCheckers ().first->CreateValidValue (StringValue (value));
  if (!first) return false;

  auto firstattr = DynamicCast <A> (first);
  if (!firstattr) return false;

  iss >> value;
  auto second = pchecker->GetCheckers ().second->CreateValidValue (StringValue (value));
  if (!second) return false;

  auto secondattr = DynamicCast <B> (second);
  if (!secondattr) return false;

  m_value = std::make_pair (firstattr, secondattr);
  return true;
}

template <class A, class B>
std::string
PairValue<A, B>::SerializeToString (Ptr<const AttributeChecker> checker) const
{
  std::ostringstream oss;
  oss << m_value.first->SerializeToString (checker);
  oss << " ";
  oss << m_value.second->SerializeToString (checker);

  return oss.str ();
}

template <class A, class B>
typename PairValue<A, B>::result_type
PairValue<A, B>::Get (void) const
{
  return std::make_pair (m_value.first->Get (), m_value.second->Get ());
}

template <class A, class B>
void
PairValue<A, B>::Set (const typename PairValue<A, B>::result_type &value)
{
  m_value = std::make_pair (Create <A> (value.first), Create <B> (value.second));
}

template <class A, class B>
template <typename T>
bool
PairValue<A, B>::GetAccessor (T &value) const
{
  value = T (Get ());
  return true;
}

template <typename A, typename B, typename T1>
Ptr<const AttributeAccessor> MakePairAccessor (T1 a1)
{
  return MakeAccessorHelper<PairValue<A, B> > (a1);
}

} // namespace ns3

#endif // PAIR_H