/*
 * Copyright (c) 2008,2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Kirill Andreev <andreev@iitp.ru>
 */

#ifndef METRIC_REPORT_H
#define METRIC_REPORT_H

#include "ns3/buffer.h"
#include "ns3/mesh-information-element-vector.h"

#include <stdint.h>

namespace ns3
{
namespace dot11s
{
/**
 * @brief a IEEE 802.11s Mesh ID 7.3.2.88 of 802.11s draft 3.0
 *
 */
class IeLinkMetricReport : public WifiInformationElement
{
  public:
    IeLinkMetricReport();
    /**
     * Constructor
     *
     * @param metric the metric
     */
    IeLinkMetricReport(uint32_t metric);
    /**
     * Set metric value
     * @param metric the metric
     */
    void SetMetric(uint32_t metric);
    /**
     * Get metric value
     * @returns the metric
     */
    uint32_t GetMetric() const;

    // Inherited from WifiInformationElement
    WifiInformationElementId ElementId() const override;
    void SerializeInformationField(Buffer::Iterator i) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;
    void Print(std::ostream& os) const override;
    uint16_t GetInformationFieldSize() const override;

  private:
    uint32_t m_metric; ///< metric
    /**
     * equality operator
     *
     * @param a lhs
     * @param b lhs
     * @returns true if equal
     */
    friend bool operator==(const IeLinkMetricReport& a, const IeLinkMetricReport& b);
    /**
     * greater than operator
     *
     * @param a lhs
     * @param b lhs
     * @returns true if equal
     */
    friend bool operator>(const IeLinkMetricReport& a, const IeLinkMetricReport& b);
    /**
     * less than operator
     *
     * @param a lhs
     * @param b lhs
     * @returns true if equal
     */
    friend bool operator<(const IeLinkMetricReport& a, const IeLinkMetricReport& b);
};

bool operator==(const IeLinkMetricReport& a, const IeLinkMetricReport& b);
bool operator>(const IeLinkMetricReport& a, const IeLinkMetricReport& b);
bool operator<(const IeLinkMetricReport& a, const IeLinkMetricReport& b);
std::ostream& operator<<(std::ostream& os, const IeLinkMetricReport& linkMetricReport);
} // namespace dot11s
} // namespace ns3
#endif /* METRIC_REPORT_H */
