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

#include <ns3/test.h>
#include <ns3/log.h>
#include <ns3/pair.h>
#include <ns3/double.h>
#include <ns3/integer.h>
#include <ns3/string.h>
#include <ns3/ptr.h>
#include <ns3/object.h>
#include <ns3/type-id.h>
#include <ns3/address.h>
#include <ns3/mac48-address.h>

#include <algorithm>
#include <iterator>
#include <sstream>
#include <utility>


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("PairTestSuite");

class PairObject : public Object
{
public:
  PairObject ();
  virtual ~PairObject ();

  static
  TypeId GetTypeId ();

  friend std::ostream &operator <<(std::ostream &os, const PairObject &obj);

private:
  std::pair <std::string, std::string> m_stringpair;
  std::pair <Address, int> m_addresspair;
};

PairObject::PairObject ()
{

}

PairObject::~PairObject ()
{

}

TypeId
PairObject::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::PairObject")
    .SetParent<Object> ()
    .SetGroupName("Test")
    .AddConstructor<PairObject> ()
    .AddAttribute ("StringPair", "Pair: string, string",
                   PairValue <StringValue, StringValue> (),
                   MakePairAccessor <StringValue, StringValue> (&PairObject::m_stringpair),
                   MakePairChecker<StringValue, StringValue> (MakeStringChecker (), MakeStringChecker ()))
    .AddAttribute ("AddressPair", "Pair: address int",
                   // the container value container differs from the underlying object
                   PairValue <AddressValue, IntegerValue> (),
                   MakePairAccessor <AddressValue, IntegerValue> (&PairObject::m_addresspair),
                   MakePairChecker<AddressValue, IntegerValue> (MakeAddressChecker (), MakeIntegerChecker<int> ()))
    ;
  return tid;
}

std::ostream &
operator << (std::ostream &os, const PairObject &obj)
{
  os << "StringPair = { " << obj.m_stringpair << " } ";
  os << "AddressPair = { " << obj.m_addresspair << " }";
  return os;
}

/* Test instantiation, initialization, access */
class PairValueTestCase : public TestCase
{
public:
  PairValueTestCase ();
  virtual ~PairValueTestCase () {}

private:
  virtual void DoRun ();
};

PairValueTestCase::PairValueTestCase ()
  : TestCase ("test instantiation, initialization, access")
{

}

void
PairValueTestCase::DoRun ()
{
 {
    std::pair<const int, double> ref = {1, 2.4};

    PairValue<IntegerValue, DoubleValue> ac (ref);

    std::pair<const int, double> rv = ac.Get ();
    NS_TEST_ASSERT_MSG_EQ (rv, ref, "Attribute value does not equal original");
  }

  {
    std::pair<const std::string, Mac48Address> ref = {"hello", Mac48Address::Allocate ()};

    PairValue<StringValue, Mac48AddressValue> ac;
    ac.Set (ref);
    std::pair<const std::string, Mac48Address> rv = ac.Get ();
    NS_TEST_ASSERT_MSG_EQ (rv, ref, "Attribute value does not equal original");
  }

}

class PairValueSettingsTestCase : public TestCase
{
public:
  PairValueSettingsTestCase ();

  void DoRun ();
};

PairValueSettingsTestCase::PairValueSettingsTestCase ()
  : TestCase ("test setting through attribute interface")
{}

void
PairValueSettingsTestCase::DoRun ()
{
  Address addr = Mac48Address::Allocate ();

  auto p = CreateObject <PairObject> ();
  p->SetAttribute ("StringPair", PairValue<StringValue, StringValue> (std::make_pair ("hello", "world")));
  p->SetAttribute ("AddressPair", PairValue<AddressValue, IntegerValue> (std::make_pair (addr, 31)));

  std::ostringstream oss, ref;
  oss << *p;

  ref << "StringPair = { hello world } AddressPair = { " << addr << " 31 }";

  NS_TEST_ASSERT_MSG_EQ ((oss.str ()), (ref.str ()), "Pairs not correctly set");
}

class PairValueTestSuite : public TestSuite
{
  public:
    PairValueTestSuite ();
};

PairValueTestSuite::PairValueTestSuite ()
  : TestSuite ("pair-value-test-suite", UNIT)
{
  AddTestCase (new PairValueTestCase (), TestCase::QUICK);
  AddTestCase (new PairValueSettingsTestCase (), TestCase::QUICK);
}

static PairValueTestSuite pairValueTestSuite; //!< Static variable for test initialization
