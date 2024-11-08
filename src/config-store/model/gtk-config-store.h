/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Faker Moatamri <faker.moatamri@sophia.inria.fr>
 *          Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef GTK_CONFIG_STORE_H
#define GTK_CONFIG_STORE_H

namespace ns3
{

/**
 * @ingroup configstore
 * @brief A class that provides a GTK-based front end to ns3::ConfigStore
 *
 */
class GtkConfigStore
{
  public:
    GtkConfigStore();

    /**
     * Process default values
     */
    void ConfigureDefaults();
    /**
     * Process attribute values
     */
    void ConfigureAttributes();
};

} // namespace ns3

#endif /* GTK_CONFIG_STORE_H */
