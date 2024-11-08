/*
 * Copyright (c) 2013 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mitch Watrous (watrous@u.washington.edu)
 */

#ifndef GET_WILDCARD_MATCHES_H
#define GET_WILDCARD_MATCHES_H

#include <string>

namespace ns3
{

/**
 * @param configPath Config path to access the probe.
 * @param matchedPath the path that matched the Config path.
 * @param wildcardSeparator the text to put between the wildcard
 * matches.  By default, a space is used.
 * @return String value of text matches
 *
 * @brief Returns the text matches from the matched path for each of
 * the wildcards in the Config path, separated by the wild card
 * separator.
 */
std::string GetWildcardMatches(const std::string& configPath,
                               const std::string& matchedPath,
                               const std::string& wildcardSeparator = " ");

} // namespace ns3

#endif // GET_WILDCARD_MATCHES_H
