/*
 * Copyright (c) 2022 Lawrence Livermore National Laboratory
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
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "environment-variable.h"

#include "ns3/string.h"

#include <cstdlib>  // getenv
#include <cstring>  // strlen
#include <iostream> // clog

/**
 * \file
 * \ingroup core-environ
 * Class EnvironmentVariable implementation.
 */
namespace ns3
{

/**
 * \ingroup core-environ
 *
 * \def NS_LOCAL_LOG(msg)
 * File-local loggging macro for environment-variable.cc
 * Our usual Logging doesn't work here because these functions
 * get called during static initialization of Logging itself.
 * \param msg The message stream to log
 *
 * \def NS_LOCAL_ASSERT(cond, msg)
 * File-local assert macro for environment-variable.cc
 * Our usual assert doesn't work here because these functions
 * get called during static initialization of Logging itself.
 * \param cond The condition which is asserted to be \c true
 * \param msg The message stream to log
 */
#if 0
#define NS_LOCAL_LOG(msg)                                                                          \
    std::cerr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "(): " << msg << std::endl

#define NS_LOCAL_ASSERT(cond, msg)                                                                 \
    do                                                                                             \
    {                                                                                              \
        if (!(cond))                                                                               \
        {                                                                                          \
            NS_LOCAL_LOG("assert failed. cond=\"" << #cond << "\", " << msg);                      \
        }                                                                                          \
    } while (false)

#else
#define NS_LOCAL_LOG(msg)
#define NS_LOCAL_ASSERT(cond, msg)
#endif

/* static */
EnvironmentVariable::DictionaryList&
EnvironmentVariable::Instance()
{
    static DictionaryList instance;
    return instance;
}

/* static */
void
EnvironmentVariable::Clear()
{
    Instance().clear();
}

/* static */
std::shared_ptr<EnvironmentVariable::Dictionary>
EnvironmentVariable::GetDictionary(const std::string& envvar, const std::string& delim /* ";" */)
{
    NS_LOCAL_LOG(envvar << ", " << delim);
    std::shared_ptr<Dictionary> dict;
    auto loc = Instance().find(envvar);
    if (loc != Instance().end())
    {
        NS_LOCAL_LOG("found envvar in cache");
        dict = loc->second;
    }
    else
    {
        NS_LOCAL_LOG("envvar not in cache, checking environment");
        dict = std::make_shared<Dictionary>(envvar, delim);
        Instance().insert({envvar, dict});
    }

    return dict;
}

/* static */
EnvironmentVariable::KeyFoundType
EnvironmentVariable::Get(const std::string& envvar,
                         const std::string& key /* "" */,
                         const std::string& delim /* ";" */)
{
    auto dict = GetDictionary(envvar, delim);
    return dict->Get(key);
}

EnvironmentVariable::KeyFoundType
EnvironmentVariable::Dictionary::Get(const std::string& key) const
{
    NS_LOCAL_LOG(key);

    if (!m_exists)
    {
        return {false, ""};
    }

    if (key.empty())
    {
        // Empty key is request for entire value
        return {true, m_variable};
    }

    auto loc = m_dict.find(key);
    if (loc != m_dict.end())
    {
        NS_LOCAL_LOG("found key in dictionary");
        NS_LOCAL_LOG("found: key '" << key << "', value: '" << loc->second << "'");
        return {true, loc->second};
    }

    // key not found
    return {false, ""};
}

EnvironmentVariable::Dictionary::Dictionary(const std::string& envvar,
                                            const std::string& delim /* "=" */)
{
    NS_LOCAL_LOG(envvar << ", " << delim);

    const char* envCstr = std::getenv(envvar.c_str());
    // Returns null pointer if envvar doesn't exist
    if (!envCstr)
    {
        m_exists = false;
        return;
    }

    // So it exists
    m_exists = true;
    m_variable = envCstr;
    NS_LOCAL_LOG("found envvar in environment with value '" << m_variable << "'");

    // ...but might be empty
    if (m_variable.empty())
    {
        return;
    }

    StringVector keyvals = SplitString(m_variable, delim);
    NS_LOCAL_ASSERT(keyvals.empty(), "Unexpected empty keyvals from non-empty m_variable");
    for (const auto& keyval : keyvals)
    {
        if (keyval.empty())
        {
            continue;
        }

        std::size_t equals = keyval.find_first_of('=');
        std::string key{keyval, 0, equals};
        std::string value;
        if (equals < keyval.size() - 1)
        {
            value = keyval.substr(equals + 1, keyval.size());
        }
        NS_LOCAL_LOG("found key '" << key << "' with value '" << value << "'");
        m_dict.insert({key, value});
    }
}

EnvironmentVariable::Dictionary::KeyValueStore
EnvironmentVariable::Dictionary::GetStore() const
{
    return m_dict;
}

} // namespace ns3
