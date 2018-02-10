/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */


#ifndef LLRA_WIFI_MANAGER_H
#define LLRA_WIFI_MANAGER_H

#include "ns3/traced-value.h"
#include "wifi-remote-station-manager.h"

namespace ns3 {

class LlraWifiManager : public WifiRemoteStationManager
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  LlraWifiManager ();
  virtual ~LlraWifiManager ();

  void SetupPhy (const Ptr<WifiPhy> phy);

private:
  //overriden from base class
  void DoInitialize (void);
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
  void DoReportAmpduTxStatus (WifiRemoteStation *station,
                              uint8_t nSuccessfulMpdus, uint8_t nFailedMpdus,
                              double rxSnr, double dataSnr);
  WifiTxVector DoGetDataTxVector (WifiRemoteStation *station);
  WifiTxVector DoGetRtsTxVector (WifiRemoteStation *station);
  bool IsLowLatency (void) const;

  uint32_t m_timerThreshold; ///< timer threshold
  uint32_t m_alpha; ///< alpha percentile
    
  /**
   * Return the minimum SNR needed to successfully transmit
   * data with this WifiTxVector at the specified BER.
   *
   * \param txVector WifiTxVector (containing valid mode, width, and nss)
   *
   * \return the minimum SNR for the given WifiTxVector
   */
  double GetSnrThreshold (WifiTxVector txVector) const;
  /**
   * Adds a pair of WifiTxVector and the minimum SNR for that given vector
   * to the list.
   *
   * \param txVector the WifiTxVector storing mode, channel width, and nss
   * \param snr the minimum SNR for the given txVector
   */
  void AddSnrThreshold (WifiTxVector txVector, double snr);

  /**
   * Convenience function for selecting a channel width for legacy mode
   * \param mode non-(V)HT WifiMode
   * \return the channel width (MHz) for the selected mode
   */
  uint8_t GetChannelWidthForMode (WifiMode mode) const;

  /**
   * A vector of <snr, WifiTxVector> pair holding the minimum SNR for the
   * WifiTxVector
   */
  typedef std::vector<std::pair<double, WifiTxVector> > Thresholds;

  double m_ber;             //!< The maximum Bit Error Rate acceptable at any transmission mode
  Thresholds m_thresholds;  //!< List of WifiTxVector and the minimum SNR pair
  TracedValue<uint64_t> m_currentRate; //!< Trace rate changes
};

} //namespace ns3

#endif /* LLRA_WIFI_MANAGER_H */
