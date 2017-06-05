
#ifndef SMOOTHSTART_H
#define SMOOTHSTART_H

#include "ns3/object.h"
#include "ns3/timer.h"
#include "ns3/tcp-socket-base.h"
#include "tcp-congestion-ops.h"

namespace ns3 {
/**
 * \brief The Smooth Start implementation
 *
 * Implements Smooth Start, a variant of Slow Start
 * \see SlowStart
 * Function SlowStart overridden in this class, to implement Smooth Start 
 */
class SmoothStart : public TcpNewReno
{
// Smooth Start
  uint32_t              m_grainNumber;      //!< Grain Number in Smooth-Start
  uint32_t              m_depth;            //!< Depth in Smooth-Start
  uint32_t              m_ackThresh;        //!< The number of acknowledgements to be recieved before increasing cwnd
  TracedValue<uint32_t> m_smsThresh;        //!< Smooth Start threshold
  uint32_t              m_firstFlag;        //!< Set to zero when CongestionAvoidance starts 
  uint32_t              m_nAcked;           //!< Number of acknowldgements recieved since last increase of cwnd

protected:
  virtual uint32_t SlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  
public:
  static TypeId GetTypeId (void);
  SmoothStart ();
  /**
   * \brief Copy constructor.
   * \param sock object to copy.
   */
  SmoothStart (const SmoothStart& sock);

  ~SmoothStart ();
  virtual void IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);

};

} // namespace ns3

#endif // SMOOTHSTART_H
