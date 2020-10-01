/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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
 * Author: Greg Steinbrecher <grs@fb.com>
 */

#include "ns3/data-rate.h"
#include "ns3/log.h"
#include "ns3/test.h"
#include "ns3/simulator.h"

using namespace ns3;

class DataRateTestCase : public TestCase
{
public:
  DataRateTestCase (std::string name);
  virtual ~DataRateTestCase ();
  void CheckTimesEqual (Time t1, Time t2, const std::string msg);

protected:
  virtual void DoRun (void) = 0;
};

DataRateTestCase::DataRateTestCase (std::string name) : TestCase (name)
{
}

DataRateTestCase::~DataRateTestCase ()
{
}

void
DataRateTestCase::CheckTimesEqual (Time actual, Time correct, const std::string msg)
{
  int64x64_t actualFemtos = actual.GetFemtoSeconds ();
  int64x64_t correctFemtos = correct.GetFemtoSeconds ();
  NS_TEST_EXPECT_MSG_EQ (actualFemtos, correctFemtos, msg);
}

class DataRateTestCase1 : public DataRateTestCase
{
public:
  DataRateTestCase1 ();
  void SingleTest (std::string rate, size_t nBits, Time correctTime);

private:
  virtual void DoRun (void);
};

DataRateTestCase1::DataRateTestCase1 ()
    : DataRateTestCase ("Test rounding of conversion from DataRate to time")
{
}

void
DataRateTestCase1::SingleTest (std::string rate, size_t nBits, Time correctTime)
{
  DataRate dr (rate);
  Time bitsTime = dr.CalculateBitsTxTime (nBits);
  CheckTimesEqual (bitsTime, correctTime, "CalculateBitsTxTime returned incorrect value");
  if ((nBits % 8) == 0)
    {
      Time bytesTime = dr.CalculateBytesTxTime (nBits / 8);
      CheckTimesEqual (bytesTime, correctTime, "CalculateBytesTxTime returned incorrect value");
    }
}

void
DataRateTestCase1::DoRun ()
{
  if (Time::GetResolution () != Time::FS)
    {
      Time::SetResolution (Time::FS);
    }
  SingleTest ("1GB/s", 512, Time (NanoSeconds (64)));
  SingleTest ("8Gb/s", 512, Time (NanoSeconds (64)));
  SingleTest ("1Gb/s", 512, Time (NanoSeconds (512)));
  SingleTest ("8GB/s", 512, Time (NanoSeconds (8)));
  size_t nBits;
  for (nBits = 0; nBits <= 512; nBits++)
    {
      SingleTest ("1Mb/s", nBits, Time (MicroSeconds (nBits)));
      SingleTest ("10Mb/s", nBits, Time (NanoSeconds (nBits * 100)));
      SingleTest ("100Mb/s", nBits, Time (NanoSeconds (nBits * 10)));
      SingleTest ("1Gb/s", nBits, Time (NanoSeconds (nBits)));
      SingleTest ("10Gb/s", nBits, Time (PicoSeconds (nBits * 100)));
      SingleTest ("25Gb/s", nBits, Time (PicoSeconds (nBits * 40)));
      SingleTest ("40Gb/s", nBits, Time (PicoSeconds (nBits * 25)));
      SingleTest ("100Gb/s", nBits, Time (PicoSeconds (nBits * 10)));
      SingleTest ("200Gb/s", nBits, Time (PicoSeconds (nBits * 5)));
      SingleTest ("400Gb/s", nBits, Time (FemtoSeconds (nBits * 2500)));
    }
}

class DataRateTestSuite : public TestSuite
{
public:
  DataRateTestSuite ();
};

DataRateTestSuite::DataRateTestSuite () : TestSuite ("data-rate", UNIT)
{
  AddTestCase (new DataRateTestCase1 (), TestCase::QUICK);
}

// Do not forget to allocate an instance of this TestSuite
static DataRateTestSuite sDataRateTestSuite;
