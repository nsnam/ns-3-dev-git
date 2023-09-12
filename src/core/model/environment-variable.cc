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

#include "string.h"

#include <cstdlib>  // std::getenv
#include <cstring>  // strlen
#include <iostream> // clog
#include <stdlib.h> // Global functions setenv, unsetenv

/**
 * \file
 * \ingroup core-environ
 * Class EnvironmentVariable implementation.
 */

#ifdef __WIN32__
#include <cerrno>

/**
 * Windows implementation of the POSIX function `setenv()`
 *
 * \param [in] var_name The environment variable to set.
 *             Must not be a null-pointer, and must not contain `=`.
 * \param [in] new_value The new value to set \p var_name to.
 *             Must not by a null pointer or empty.
 * \param [in] change_flag Must be non-zero to actually change the environment.
 * \returns 0 if successful, -1 if failed.
 */
int
setenv(const char* var_name, const char* new_value, int change_flag)
{
    std::string variable{var_name};
    std::string value{new_value};

    // In case arguments are null pointers, return invalid error
    // Windows does not accept empty environment variables
    if (variable.empty() || value.empty())
    {
        errno = EINVAL;
        return -1;
    }

    // Posix does not accept '=', so impose that here
    if (variable.find('=') != std::string::npos)
    {
        errno = EINVAL;
        return -1;
    }

    // Change flag equals to zero preserves a pre-existing value
    if (change_flag == 0)
    {
        char* old_value = std::getenv(var_name);
        if (old_value != nullptr)
        {
            return 0;
        }
    }

    // Write new value for the environment variable
    return _putenv_s(var_name, new_value);
}

/**
 * Windows implementation of the POSIX function `unsetenv()`
 * \param [in] var_name The environment variable to unset and remove from the environment.
 * \returns 0 if successful, -1 if failed.
 */
int
unsetenv(const char* var_name)
{
    return _putenv_s(var_name, "");
}

#endif // __WIN32__

namespace ns3
{

/**
 * \ingroup core-environ
 *
 * \def NS_LOCAL_LOG(msg)
 * File-local logging macro for environment-variable.cc
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

/* static */
bool
EnvironmentVariable::Set(const std::string& variable, const std::string& value)
{
    int fail = setenv(variable.c_str(), value.c_str(), 1);
    return !fail;
}

/* static */
bool
EnvironmentVariable::Unset(const std::string& variable)
{
    int fail = unsetenv(variable.c_str());
    return !fail;
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
