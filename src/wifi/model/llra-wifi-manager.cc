/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */


#include "llra-wifi-manager.h"
#include "ns3/log.h"
#include "wifi-phy.h"
#include "ns3/uinteger.h"

#define Min(a,b) ((a < b) ? a : b)

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LlraWifiManager");

/**
 * \brief hold per-remote-station state for LLRA Wifi manager.
 *
 * This struct extends from WifiRemoteStation struct to hold additional
 * information required by the LLRA Wifi manager
 */
struct LlraWifiRemoteStation : public WifiRemoteStation
{
  uint32_t m_packets; ///< timer value
  uint32_t m_success; ///< success count
  uint32_t m_failed; ///< failed count
  uint32_t Nrt[100];
  uint32_t m_retry; ///< retry count
  uint32_t m_timerTimeout; ///< timer timeout
  uint32_t m_rate; ///< rate
  uint32_t m_alpha; ///< alpha percentile
  double m_lastSnrObserved;  //!< SNR of most recently reported packet sent to the remote station
  double m_lastSnrCached;    //!< SNR most recently used to select a rate
  double m_nss;          //!< SNR most recently used to select a rate
  WifiMode m_lastMode;       //!< Mode most recently used to the remote station
};

/// To avoid using the cache before a valid value has been cached
static const double CACHE_INITIAL_VALUE = -100;
NS_OBJECT_ENSURE_REGISTERED (LlraWifiManager);

TypeId
LlraWifiManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LlraWifiManager")
    .SetParent<WifiRemoteStationManager> ()
    .SetGroupName ("Wifi")
    .AddConstructor<LlraWifiManager> ()
    .AddAttribute ("Alpha", "The alpha percentile.",
                   UintegerValue (95),
                   MakeUintegerAccessor (&LlraWifiManager::m_alpha),
                   MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("Rate",
                     "Traced value for rate changes (b/s)",
                     MakeTraceSourceAccessor (&LlraWifiManager::m_currentRate),
                     "ns3::TracedValueCallback::Uint64")
  ;
  return tid;
}


LlraWifiManager::LlraWifiManager ()
  : m_currentRate (0)
{
}

LlraWifiManager::~LlraWifiManager ()
{
}

void
LlraWifiManager::SetupPhy (const Ptr<WifiPhy> phy)
{
  NS_LOG_FUNCTION (this << phy);
  WifiRemoteStationManager::SetupPhy (phy);
}

uint8_t
LlraWifiManager::GetChannelWidthForMode (WifiMode mode) const
{
  NS_ASSERT (mode.GetModulationClass () != WIFI_MOD_CLASS_HT
             && mode.GetModulationClass () != WIFI_MOD_CLASS_VHT
             && mode.GetModulationClass () != WIFI_MOD_CLASS_HE);
  if (mode.GetModulationClass () == WIFI_MOD_CLASS_DSSS
      || mode.GetModulationClass () == WIFI_MOD_CLASS_HR_DSSS)
    {
      return 22;
    }
  else
    {
      return 20;
    }
}

void
LlraWifiManager::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
  WifiMode mode;
  WifiTxVector txVector;
  uint8_t nss = 1;
  uint8_t nModes = GetPhy ()->GetNModes ();
  for (uint8_t i = 0; i < nModes; i++)
    {
      mode = GetPhy ()->GetMode (i);
      txVector.SetChannelWidth (GetChannelWidthForMode (mode));
      txVector.SetNss (nss);
      txVector.SetMode (mode);
      NS_LOG_DEBUG ("Initialize, adding mode = " << mode.GetUniqueName ());
      AddSnrThreshold (txVector, GetPhy ()->CalculateSnr (txVector, m_ber));
    }
  // Add all Ht and Vht MCSes
  if (HasVhtSupported () == true || HasHtSupported () == true || HasHeSupported () == true)
    {
      nModes = GetPhy ()->GetNMcs ();
      for (uint8_t i = 0; i < nModes; i++)
        {
          for (uint16_t j = 20; j <= GetPhy ()->GetChannelWidth (); j *= 2)
            {
              txVector.SetChannelWidth (j);
              mode = GetPhy ()->GetMcs (i);
              if (mode.GetModulationClass () == WIFI_MOD_CLASS_HT)
                {
                  uint16_t guardInterval = GetPhy ()->GetShortGuardInterval () ? 400 : 800;
                  txVector.SetGuardInterval (guardInterval);
                  //derive NSS from the MCS index
                  nss = (mode.GetMcsValue () / 8) + 1;
                  NS_LOG_DEBUG ("Initialize, adding mode = " << mode.GetUniqueName () <<
                                " channel width " << (uint16_t) j <<
                                " nss " << (uint16_t) nss <<
                                " GI " << guardInterval);
                  NS_LOG_DEBUG ("In SetupPhy, adding mode = " << mode.GetUniqueName ());
                  txVector.SetNss (nss);
                  txVector.SetMode (mode);
                  AddSnrThreshold (txVector, GetPhy ()->CalculateSnr (txVector, m_ber));
                }
              else //VHT or HE
                {
                  uint16_t guardInterval;
                  if (mode.GetModulationClass () == WIFI_MOD_CLASS_VHT)
                    {
                      guardInterval = GetPhy ()->GetShortGuardInterval () ? 400 : 800;
                    }
                  else
                    {
                      guardInterval = GetPhy ()->GetGuardInterval ().GetNanoSeconds ();
                    }
                  for (uint8_t i = 1; i <= GetPhy ()->GetMaxSupportedTxSpatialStreams (); i++)
                    {
                      NS_LOG_DEBUG ("Initialize, adding mode = " << mode.GetUniqueName () <<
                                    " channel width " << (uint16_t) j <<
                                    " nss " << (uint16_t) i <<
                                    " GI " << guardInterval);
                      NS_LOG_DEBUG ("In SetupPhy, adding mode = " << mode.GetUniqueName ());
                      txVector.SetNss (i);
                      txVector.SetMode (mode);
                      AddSnrThreshold (txVector, GetPhy ()->CalculateSnr (txVector, m_ber));
                    }
                }
            }
        }
    }
}

double
LlraWifiManager::GetSnrThreshold (WifiTxVector txVector) const
{
  NS_LOG_FUNCTION (this << txVector.GetMode ().GetUniqueName ());
  for (Thresholds::const_iterator i = m_thresholds.begin (); i != m_thresholds.end (); i++)
    {
      NS_LOG_DEBUG ("Checking " << i->second.GetMode ().GetUniqueName () <<
                    " nss " << (uint16_t) i->second.GetNss () <<
                    " GI " << i->second.GetGuardInterval () <<
                    " width " << (uint16_t) i->second.GetChannelWidth ());
      NS_LOG_DEBUG ("against TxVector " << txVector.GetMode ().GetUniqueName () <<
                    " nss " << (uint16_t) txVector.GetNss () <<
                    " GI " << txVector.GetGuardInterval () <<
                    " width " << (uint16_t) txVector.GetChannelWidth ());
      if (txVector.GetMode () == i->second.GetMode ()
          && txVector.GetNss () == i->second.GetNss ()
          && txVector.GetChannelWidth () == i->second.GetChannelWidth ())
        {
          return i->first;
        }
    }
  NS_ASSERT (false);
  return 0.0;
}

void
LlraWifiManager::AddSnrThreshold (WifiTxVector txVector, double snr)
{
  NS_LOG_FUNCTION (this << txVector.GetMode ().GetUniqueName () << snr);
  m_thresholds.push_back (std::make_pair (snr, txVector));
}

WifiRemoteStation *
LlraWifiManager::DoCreateStation (void) const
{
  NS_LOG_FUNCTION (this);
  LlraWifiRemoteStation *station = new LlraWifiRemoteStation ();
  station->m_lastSnrObserved = 0.0;
  station->m_lastSnrCached = CACHE_INITIAL_VALUE;
  station->m_lastMode = GetDefaultMode ();
  station->m_nss = 1;
  return station;
}


void
LlraWifiManager::DoReportRxOk (WifiRemoteStation *station,
                                double rxSnr, WifiMode txMode)
{
}

void
LlraWifiManager::DoReportRtsFailed (WifiRemoteStation *station)
{
}

void
LlraWifiManager::DoReportDataFailed (WifiRemoteStation *station)
{
  NS_LOG_FUNCTION (this << st << ackSnr << ackMode.GetUniqueName () << dataSnr);
  LlraWifiRemoteStation *station = (LlraWifiRemoteStation *)st;
  station->packets++;
  CalculateLatency(station);
}

void
LlraWifiManager::DoReportRtsOk (WifiRemoteStation *st,
                                 double ctsSnr, WifiMode ctsMode, double rtsSnr)
{
  NS_LOG_FUNCTION (this << st << ctsSnr << ctsMode.GetUniqueName () << rtsSnr);
  LlraWifiRemoteStation *station = (LlraWifiRemoteStation *)st;
  station->m_lastSnrObserved = rtsSnr;
}

void
LlraWifiManager::DoReportDataOk (WifiRemoteStation *st,
                                  double ackSnr, WifiMode ackMode, double dataSnr)
{
  NS_LOG_FUNCTION (this << st << ackSnr << ackMode.GetUniqueName () << dataSnr);
  LlraWifiRemoteStation *station = (LlraWifiRemoteStation *)st;
  station->packets++;
  CalculateLatency(station);
  if (dataSnr == 0)
    {
      NS_LOG_WARN ("DataSnr reported to be zero; not saving this report.");
      return;
    }
  station->m_lastSnrObserved = dataSnr;
}

void
LlraWifiManager::DoReportAmpduTxStatus (WifiRemoteStation *st, uint8_t nSuccessfulMpdus, uint8_t nFailedMpdus, double rxSnr, double dataSnr)
{
  NS_LOG_FUNCTION (this << st << (uint16_t)nSuccessfulMpdus << (uint16_t)nFailedMpdus << rxSnr << dataSnr);
  LlraWifiRemoteStation *station = (LlraWifiRemoteStation *)st;
  if (dataSnr == 0)
    {
      NS_LOG_WARN ("DataSnr reported to be zero; not saving this report.");
      return;
    }
  station->m_lastSnrObserved = dataSnr;
}


void
LlraWifiManager::DoReportFinalRtsFailed (WifiRemoteStation *station)
{
}

void
LlraWifiManager::DoReportFinalDataFailed (WifiRemoteStation *station)
{
}

void
LlraWifiManager::ProbeMode (WifiRemoteStation *st)
{

}

double
LlraWifiManager::CalculateLatency (WifiRemoteStation *st)
{
  double latency = Tinitial + Tserv;


}

WifiTxVector
LlraWifiManager::DoGetDataTxVector (WifiRemoteStation *st)
{
  NS_LOG_FUNCTION (this << st);
  LlraWifiRemoteStation *station = (LlraWifiRemoteStation *)st;
  
  if(station->packets % 100 == 0){

    if(station->Nrt[station->alpha] > 0)
      ProbeMode(station);
  }

}

WifiTxVector
LlraWifiManager::DoGetRtsTxVector (WifiRemoteStation *st)
{
}

bool
LlraWifiManager::IsLowLatency (void) const
{
  return true;
}

} //namespace ns3


