/*
 * Copyright (c) 2010 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef OUTPUT_STREAM_WRAPPER_H
#define OUTPUT_STREAM_WRAPPER_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/simple-ref-count.h"

#include <fstream>

namespace ns3
{

/**
 * @brief A class encapsulating an output stream.
 *
 * This class wraps a pointer to a C++ std::ostream and provides
 * reference counting of the object.  This class is recommended for users
 * who want to pass output streams in the ns-3 APIs, such as in callbacks
 * or tracing.
 *
 * This class is motivated by the fact that in C++, copy and assignment of
 * iostreams is forbidden by std::basic_ios<>, because it is not possible
 * to predict the semantics of the stream desired by a user.
 *
 * When writing traced information to a file, the tempting ns-3 idiom is to
 * create a bound callback with an ofstream as the bound object.  Unfortunately,
 * this implies a copy construction in order to get the ofstream object into the
 * callback.  This operation, as mentioned above, is forbidden by the STL.
 * Using this class in ns-3 APIs is generally preferable to passing global
 * pointers to ostream objects, or passing a pointer to a stack allocated
 * ostream (which creates object lifetime issues).
 *
 * One could imagine having this object inherit from stream to get the various
 * overloaded operator<< defined, but we're going to be using a
 * Ptr<OutputStreamWrapper> when passing this object around.  In this case, the
 * Ptr<> wouldn't understand the operators and we would have to dereference it
 * to access the underlying object methods.  Since we would have to dereference
 * the Ptr<>, we don't bother and just expect the user to Get a saved pointer
 * to an ostream and dereference it him or herself.  As in:
 *
 * \code{.cpp}
 * void
 * TraceSink(Ptr<OutputStreamWrapper> streamWrapper, Ptr<const Packet> packet)
 * {
 *     std::ostream *stream = streamWrapper->GetStream();
 *     *stream << "got packet" << std::endl;
 * }
 * @endcode
 *
 *
 * This class uses a basic ns-3 reference counting base class but is not
 * an ns3::Object with attributes, TypeId, or aggregation.
 */
class OutputStreamWrapper : public SimpleRefCount<OutputStreamWrapper>
{
  public:
    /**
     * Constructor
     * @param filename file name
     * @param filemode std::ios::openmode flags
     */
    OutputStreamWrapper(std::string filename, std::ios::openmode filemode);
    /**
     * Constructor
     * @param os output stream
     */
    OutputStreamWrapper(std::ostream* os);
    ~OutputStreamWrapper();

    /**
     * Return a pointer to an ostream previously set in the wrapper.
     *
     * @see SetStream
     *
     * @returns a pointer to the encapsulated std::ostream
     */
    std::ostream* GetStream();

  private:
    std::ostream* m_ostream; //!< The output stream
    bool m_destroyable;      //!< Can be destroyed
};

} // namespace ns3

#endif /* OUTPUT_STREAM_WRAPPER_H */
