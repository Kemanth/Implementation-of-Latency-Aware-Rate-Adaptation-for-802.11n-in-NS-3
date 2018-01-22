/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */


#include "lara-wifi-manager.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"

#define Min(a,b) ((a < b) ? a : b)

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LaraWifiManager");

/**
 * \brief hold per-remote-station state for LARA Wifi manager.
 *
 * This struct extends from WifiRemoteStation struct to hold additional
 * information required by the LARA Wifi manager
 */
struct LaraWifiRemoteStation : public WifiRemoteStation
{
  uint32_t m_timer; ///< timer value
  uint32_t m_success; ///< success count
  uint32_t m_failed; ///< failed count
  bool m_recovery; ///< recovery
  uint32_t m_retry; ///< retry count
  uint32_t m_timerTimeout; ///< timer timeout
  uint32_t m_rate; ///< rate
  uint32_t m_alpha; ///< alpha percentile
};

NS_OBJECT_ENSURE_REGISTERED (LaraWifiManager);

TypeId
LaraWifiManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LaraWifiManager")
    .SetParent<WifiRemoteStationManager> ()
    .SetGroupName ("Wifi")
    .AddConstructor<LaraWifiManager> ()
    .AddAttribute ("Alpha", "The alpha percentile.",
                   UintegerValue (95),
                   MakeUintegerAccessor (&LaraWifiManager::m_alpha),
                   MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("Rate",
                     "Traced value for rate changes (b/s)",
                     MakeTraceSourceAccessor (&LaraWifiManager::m_currentRate),
                     "ns3::TracedValueCallback::Uint64")
  ;
  return tid;
}
