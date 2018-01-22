/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */


#ifndef LARA_WIFI_MANAGER_H
#define LARA_WIFI_MANAGER_H

#include "ns3/traced-value.h"
#include "wifi-remote-station-manager.h"

namespace ns3 {

class LaraWifiManager : public WifiRemoteStationManager
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  LaraWifiManager ();
  virtual ~LaraWifiManager ();

  // Inherited from WifiRemoteStationManager
  void SetHtSupported (bool enable);
  void SetVhtSupported (bool enable);
  void SetHeSupported (bool enable);


private:
  //overriden from base class
  WifiRemoteStation * DoCreateStation (void) const;
  void DoReportRxOk (WifiRemoteStation *station,
                     double rxSnr, WifiMode txMode);
  void DoReportRtsFailed (WifiRemoteStation *station);
  void DoReportDataFailed (WifiRemoteStation *station);
  void DoReportRtsOk (WifiRemoteStation *station,
                      double ctsSnr, WifiMode ctsMode, double rtsSnr);
  void DoReportDataOk (WifiRemoteStation *station,
                       double ackSnr, WifiMode ackMode, double dataSnr);
  void DoReportFinalRtsFailed (WifiRemoteStation *station);
  void DoReportFinalDataFailed (WifiRemoteStation *station);
  WifiTxVector DoGetDataTxVector (WifiRemoteStation *station);
  WifiTxVector DoGetRtsTxVector (WifiRemoteStation *station);
  bool IsLowLatency (void) const;

  uint32_t m_timerThreshold; ///< timer threshold
  uint32_t m_alpha; ///< alpha percentile

  TracedValue<uint64_t> m_currentRate; //!< Trace rate changes
};

} //namespace ns3

#endif /* LARA_WIFI_MANAGER_H */
