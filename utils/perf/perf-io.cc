/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include <chrono>

#include <cstdio>
#include <cstdlib>

#include <iostream>
#include <fstream>

#include "ns3/core-module.h"

using namespace ns3;

/**
 * \ingroup system-tests-perf
 * 
 * Check the performance of writing to file.
 * 
 * \param file The file to write to.
 * \param n The number of writes to perform.
 * \param buffer The buffer to write.
 * \param size The buffer size.
 */
void
PerfFile (FILE *file, uint32_t n, const char *buffer, uint32_t size)
{
  for (uint32_t i = 0; i < n; ++i)
    {
      if (std::fwrite (buffer, 1, size, file) !=  size)
        {
          NS_ABORT_MSG ("PerfFile():  fwrite error");
        }
    }
}

/**
 * \ingroup system-tests-perf
 * 
 * Check the performance of writing to an output stream.
 * 
 * \param stream The output stream to write to.
 * \param n The number of writes to perform.
 * \param buffer The buffer to write.
 * \param size The buffer size.
 */
void
PerfStream (std::ostream &stream, uint32_t n, const char *buffer, uint32_t size)
{
  for (uint32_t i = 0; i < n; ++i)
    {
      stream.write (buffer, size);
    }
}

int 
main (int argc, char *argv[])
{
  uint32_t n = 100000;
  uint32_t iter = 50;
  bool doStream = false;
  bool binmode = true;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("n", "How many times to write (defaults to 100000", n);
  cmd.AddValue ("iter", "How many times to run the test looking for a min (defaults to 50)", iter);
  cmd.AddValue ("doStream", "Run the C++ I/O benchmark otherwise the C I/O ", doStream);
  cmd.AddValue ("binmode", "Select binary mode for the C++ I/O benchmark (defaults to true)", binmode);
  cmd.Parse (argc, argv);

  auto minResultNs = std::chrono::duration_cast<std::chrono::nanoseconds> (
    std::chrono::nanoseconds::max ());

  char buffer[1024];

  if (doStream)
    {
      //
      // This will probably run on a machine doing other things.  Run it some
      // relatively large number of times and try to find a minimum, which
      // will hopefully represent a time when it runs free of interference.
      //
      for (uint32_t i = 0; i < iter; ++i)
        {
          std::ofstream stream;
          if (binmode)
            {
              stream.open ("streamtest", std::ios_base::binary | std::ios_base::out);
            }
          else
            {
              stream.open ("streamtest", std::ios_base::out);
            }

          auto start = std::chrono::steady_clock::now ();
          PerfStream (stream, n, buffer, 1024);
          auto end = std::chrono::steady_clock::now ();
          auto resultNs = std::chrono::duration_cast<std::chrono::nanoseconds> (end - start);
          resultNs = std::min (resultNs, minResultNs);
          stream.close ();
          std::cout << "."; std::cout.flush ();
        }

      std::cout << std::endl;
    }
  else
    {
      //
      // This will probably run on a machine doing other things.  Run it some
      // relatively large number of times and try to find a minimum, which
      // will hopefully represent a time when it runs free of interference.
      //
      for (uint32_t i = 0; i < iter; ++i)
        {
          FILE *file = fopen ("filetest", "w");

          auto start = std::chrono::steady_clock::now ();
          PerfFile (file, n, buffer, 1024);
          auto end = std::chrono::steady_clock::now ();
          auto resultNs = std::chrono::duration_cast<std::chrono::nanoseconds> (end - start);
          resultNs = std::min (resultNs, minResultNs);
          fclose (file);
          file = 0;
          std::cout << "."; std::cout.flush ();
        }
      std::cout << std::endl;
    }

  std::cout << argv[0] << ": " << minResultNs.count () << "ns" << std::endl;

  return 0;
}
