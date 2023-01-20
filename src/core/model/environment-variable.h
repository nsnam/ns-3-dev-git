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

#ifndef ENVIRONMENT_VARIABLE_H
#define ENVIRONMENT_VARIABLE_H

/**
 * \file
 * \ingroup core-environ
 * Class Environment declaration.
 */

#include <memory> // shared_ptr
#include <string>
#include <unordered_map>
#include <utility> // pair

namespace ns3
{

/// \todo Reconsider the name?
/// Rename to just EnvironmentVariableDictionary?
/// System::EnvironmentVariable?

// Forward declaration
namespace tests
{
class EnvVarTestCase;
}

/**
 * \ingroup core-environ
 * Hold key,value dictionaries for environment variables.
 *
 * The environment variable can have multiple key,value pairs
 * separated by a delimiter, which is ";" by default.
 *
 * Individual pairs are connected by '='.  As an extension a bare key
 * will be assigned the empty string \c "".
 *
 * For example, `ENVVAR="key1=value1;key2;key3=value3"`.
 */
class EnvironmentVariable
{
  public:
    /**
     * Result of a key lookup.
     * The \p first is \c true if the key was found.
     *
     * The \p second contains the value associated with the key.
     */
    using KeyFoundType = std::pair<bool, std::string>;

    /**
     * Get the value corresponding to a key from an environment variable.
     *
     * If the \p key is empty just return the environment variable,
     * (or `{false, ""}` if the variable doesn't exist).
     *
     * If the \p key is not empty return the associated value
     * (or `{false, ""}` if the key is not found).
     * If the key is present but has no value return `{true, ""}`.
     *
     * Key-value pairs are separated by \p delim.  Individual keys and
     * values are separated by an `=` sign. If the `=` is not present
     * the value returned is the empty string.
     *
     * Notice that two cases both return `{false, ""}`:
     * * The environment variable doesn't exist, or
     * * It exists but the (non-empty) \p key wasn't found.
     *
     * Notice that two cases both return `{true, ""}`: the environment
     * variable exists and
     * * The \p key was empty and the environment variable was empty, or
     * * The (not empty) key was found, but with no value.
     *
     * In practice neither of these ambiguities is important:
     * * If the return contains \c false the key doesn't exist.
     * * If the return contains \c true but the string is empty either the
     *   (not empty) key exists with no value, or the key was empty and the
     *   environment variable itself is empty.
     *
     * \param [in] envvar The environment variable.
     * \param [in] key The key to extract from the environment variable.
     * \param [in] delim The delimiter between key,value pairs.
     * \returns Whether the key was found, and its value, as explained above.
     */
    static KeyFoundType Get(const std::string& envvar,
                            const std::string& key = "",
                            const std::string& delim = ";");

    // Forward
    class Dictionary;

    /**
     * Get the dictionary for a particular environment variable.
     *
     * This should be used when one needs to process all key,value
     * pairs, perhaps without knowing the set of possible keys.
     *
     * \param [in] envvar The environment variable.
     * \param [in] delim The delimiter between key,value pairs.
     * \returns The Dictionary.
     */
    static std::shared_ptr<Dictionary> GetDictionary(const std::string& envvar,
                                                     const std::string& delim = ";");

    /** Key, value dictionary for a single environment variable. */
    class Dictionary
    {
      public:
        /**
         * Constructor.
         *
         * Parse an environment variable containing keys and optional values.
         *
         * Keys in the environment variable are separated by the
         * \p delim character.  Keys may be assigned values by following
         * the key with the `=` character; any remaining text up to the next
         * delimiter will be taken as the value.  If no `=`
         * is given the enpty string will be stored as the value.
         *
         * \param [in] envvar The environment variable.
         * \param [in] delim The delimiter between key,value pairs.
         */
        Dictionary(const std::string& envvar, const std::string& delim = ";");

        /**
         * Get the value corresponding to a key from this dictionary.
         * If the \p key is empty return the full environment variable value.
         * \param [in] key The key to extract from the environment variable.
         * \returns \c true if the key was found, and the associated value.
         */
        KeyFoundType Get(const std::string& key = "") const;

        /** Key, value store type. */
        using KeyValueStore = std::unordered_map<std::string, std::string>;

        /** Get the underlying store, for iterating.
         * \returns The key, value store.
         */
        KeyValueStore GetStore() const;

      private:
        /** Whether the environment variable exists in the environment. */
        bool m_exists;
        /** The raw environment variable. */
        std::string m_variable;
        /** The key, value store. */
        KeyValueStore m_dict;

    }; // class Dictionary

    /**
     * Set an environment variable.
     *
     * To set a variable to the empty string use `Set(variable, "")`.
     * Note: empty environment variables are not portable (unsupported on Windows).
     *
     * \param [in] variable The environment variable to set.  Note this may not contain the `=`
     * character. \param [in] value The value to set.  Note this must not be an empty string on
     * Windows. \returns \c true if the variable was set successfully
     */
    static bool Set(const std::string& variable, const std::string& value);

    /**
     * Unset an environment variable.
     * This removes the variable from the environment.
     * To set a variable to the empty string use `Set(variable, "")`.
     *
     * \param [in] variable The environment variable to unset.  Note this may not contain the `=`
     * character. \returns \c true if the variable was unset successfully.
     */
    static bool Unset(const std::string& variable);

    /**
     * \name Singleton
     *
     * This class is a singleton, accessed by static member functions,
     * so the Rule of Five functions are all deleted.
     */
    /** @{ */
    EnvironmentVariable() = delete;
    EnvironmentVariable(const EnvironmentVariable&) = delete;
    EnvironmentVariable& operator=(const EnvironmentVariable&) = delete;
    EnvironmentVariable(EnvironmentVariable&&) = delete;
    EnvironmentVariable& operator=(EnvironmentVariable&&) = delete;
    /** @} */

  private:
    /**
     * How Dictionaries are stored.
     *
     * \p key: the environment variable name
     *
     * \p Dictionary: the parsed Dictionary for the \p key
     */
    using DictionaryList = std::unordered_map<std::string, std::shared_ptr<Dictionary>>;

    /**
     * Access the DictionaryStore instance.
     * \returns the DictionaryStore.
     */
    static DictionaryList& Instance();

    // Test needs to clear the instance
    friend class tests::EnvVarTestCase;

    /** Clear the instance, forcing all new lookups. */
    static void Clear();

}; // class EnvironmentVariable

} // namespace ns3

#endif /* ENVIRONMENT_VARIABLE_H */
