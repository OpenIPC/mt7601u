/*

*/

#include "rt_config.h"


struct wifi_dev *get_wdev_by_idx(RTMP_ADAPTER *pAd, INT idx)
{
	struct wifi_dev *wdev = NULL;
	
	do
	{
#if define(P2P_SUPPORT) || define(SOFTAP_SUPPORT)
		if (idx >= MIN_NET_DEVICE_FOR_P2P_GO)
		{
			wdev = &pAd->ApCfg.MBSSID[idx - MIN_NET_DEVICE_FOR_P2P_GO].wdev;
			break;
		}
#endif /* P2P_SUPPORT */


#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT	
		if (idx >= MIN_NET_DEVICE_FOR_APCLI)
		{				
			idx -= MIN_NET_DEVICE_FOR_APCLI;		
			if (idx < MAX_APCLI_NUM)
			{
				wdev = &pAd->ApCfg.ApCliTab[idx].wdev;
				break;
			}
		}
#endif /* APCLI_SUPPORT */

		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		{

			if ((idx < pAd->ApCfg.BssidNum) && (idx < MAX_MBSSID_NUM(pAd)) && (idx < HW_BEACON_MAX_NUM))
				wdev = &pAd->ApCfg.MBSSID[idx].wdev;

			break;
		}			
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		{
			wdev = &pAd->StaCfg.wdev;
			break;
		}	
#endif /* CONFIG_STA_SUPPORT */
	} while (FALSE);

	if (wdev == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("RTMPSetIndividualHT: invalid idx(%d)\n", idx));
	}

	return wdev;
}

