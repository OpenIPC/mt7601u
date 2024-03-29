obj-m += mt7601sta.o

mt7601sta-objs += os/linux/prealloc.o
mt7601sta-objs += os/linux/rt_profile.o
mt7601sta-objs += sta/assoc.o
mt7601sta-objs += sta/auth.o
mt7601sta-objs += sta/auth_rsp.o
mt7601sta-objs += sta/sync.o
mt7601sta-objs += sta/sanity.o
mt7601sta-objs += sta/rtmp_data.o
mt7601sta-objs += sta/connect.o
mt7601sta-objs += sta/wpa.o
mt7601sta-objs += sta/sta_cfg.o
mt7601sta-objs += common/crypt_md5.o
mt7601sta-objs += common/crypt_sha2.o
mt7601sta-objs += common/crypt_hmac.o
mt7601sta-objs += common/crypt_aes.o
mt7601sta-objs += common/crypt_arc4.o
mt7601sta-objs += common/mlme.o
mt7601sta-objs += common/cmm_wep.o
mt7601sta-objs += common/action.o
mt7601sta-objs += common/cmm_data.o
mt7601sta-objs += common/rtmp_init.o
mt7601sta-objs += common/rtmp_init_inf.o
mt7601sta-objs += common/cmm_tkip.o
mt7601sta-objs += common/cmm_aes.o
mt7601sta-objs += common/cmm_sync.o
mt7601sta-objs += common/eeprom.o
mt7601sta-objs += common/cmm_sanity.o
mt7601sta-objs += common/cmm_info.o
mt7601sta-objs += common/cmm_cfg.o
mt7601sta-objs += common/cmm_wpa.o
mt7601sta-objs += common/cmm_radar.o
mt7601sta-objs += common/spectrum.o
mt7601sta-objs += common/rtmp_timer.o
mt7601sta-objs += common/rt_channel.o
mt7601sta-objs += common/cmm_profile.o
mt7601sta-objs += common/cmm_asic.o
mt7601sta-objs += common/scan.o
mt7601sta-objs += common/cmm_cmd.o
mt7601sta-objs += common/uapsd.o
mt7601sta-objs += common/ps.o
mt7601sta-objs += rate_ctrl/ra_ctrl.o
mt7601sta-objs += rate_ctrl/alg_legacy.o
mt7601sta-objs += rate_ctrl/alg_ags.o
mt7601sta-objs += chips/rtmp_chip.o
mt7601sta-objs += common/txpower.o
mt7601sta-objs += mac/rtmp_mac.o
mt7601sta-objs += mgmt/mgmt_hw.o
mt7601sta-objs += mgmt/mgmt_entrytb.o
mt7601sta-objs += phy/rtmp_phy.o
mt7601sta-objs += phy/rlt_phy.o
mt7601sta-objs += phy/rlt_rf.o
mt7601sta-objs += common/ba_action.o
mt7601sta-objs += mgmt/mgmt_ht.o
mt7601sta-objs += common/wsc.o
mt7601sta-objs += common/wsc_tlv.o
mt7601sta-objs += common/crypt_biginteger.o
mt7601sta-objs += common/crypt_dh.o
mt7601sta-objs += common/wsc_v2.o
mt7601sta-objs += common/rt_os_util.o
mt7601sta-objs += os/linux/sta_ioctl.o
mt7601sta-objs += os/linux/rt_linux.o
mt7601sta-objs += os/linux/rt_main_dev.o
mt7601sta-objs += common/cmm_wpa_adhoc.o
mt7601sta-objs += common/rt_led.o
mt7601sta-objs += ate/common/rt_ate.o
mt7601sta-objs += ate/common/rt_qa.o
mt7601sta-objs += common/cmm_mac_usb.o
mt7601sta-objs += common/cmm_data_usb.o
mt7601sta-objs += common/rtusb_io.o
mt7601sta-objs += common/rtusb_data.o
mt7601sta-objs += common/rtusb_bulk.o
mt7601sta-objs += os/linux/rt_usb.o
mt7601sta-objs += common/ee_prom.o
mt7601sta-objs += common/ee_efuse.o
mt7601sta-objs += mcu/rtmp_and.o
mt7601sta-objs += mcu/rtmp_mcu.o
mt7601sta-objs += mcu/rtmp_M51.o
mt7601sta-objs += common/rt_rf.o
mt7601sta-objs += chips/mt7601.o
mt7601sta-objs += mac/ral_omac.o
mt7601sta-objs += ate/chips/mt7601_ate.o
mt7601sta-objs += ate/common/ate_usb.o
mt7601sta-objs += os/linux/rt_usb_util.o
mt7601sta-objs += os/linux/usb_main_dev.o
mt7601sta-objs += common/rtusb_dev_id.o
mt7601sta-objs += common/frq_cal.o
mt7601sta-objs += os/linux/cfg80211.o
mt7601sta-objs += os/linux/cfg80211drv.o
mt7601sta-objs += os/linux/cfg80211_inf.o
mt7601sta-objs += common/p2p_inf.o
mt7601sta-objs += ap/ap_mbss.o
mt7601sta-objs += ap/ap.o
mt7601sta-objs += ap/ap_assoc.o
mt7601sta-objs += ap/ap_auth.o
mt7601sta-objs += ap/ap_connect.o
mt7601sta-objs += ap/ap_mlme.o
mt7601sta-objs += ap/ap_sanity.o
mt7601sta-objs += ap/ap_sync.o
mt7601sta-objs += ap/ap_wpa.o
mt7601sta-objs += ap/ap_data.o
mt7601sta-objs += ap/ap_autoChSel.o
mt7601sta-objs += ap/ap_qload.o
mt7601sta-objs += sta/p2pcli.o 
mt7601sta-objs += sta/p2pcli_ctrl.o 
mt7601sta-objs += sta/p2pcli_sync.o
mt7601sta-objs += sta/p2pcli_auth.o
mt7601sta-objs += sta/p2pcli_assoc.o
mt7601sta-objs += common/cmm_mat.o
mt7601sta-objs += common/cmm_mat_iparp.o
mt7601sta-objs += common/cmm_mat_pppoe.o
mt7601sta-objs += common/cmm_mat_ipv6.o
mt7601sta-objs += common/p2p_dbg.o
mt7601sta-objs += common/p2p_cfg.o
mt7601sta-objs += common/p2p.o
mt7601sta-objs += common/p2p_packet.o
mt7601sta-objs += common/p2p_action.o
mt7601sta-objs += common/p2p_table.o
mt7601sta-objs += common/p2p_ctrl.o
mt7601sta-objs += common/p2p_disc_mng.o
mt7601sta-objs += common/p2p_nego_mng.o
mt7601sta-objs += common/p2p_dbg.o
mt7601sta-objs += common/p2p.o
mt7601sta-objs += common/p2p_packet.o
mt7601sta-objs += common/p2p_action.o
mt7601sta-objs += common/p2p_table.o
mt7601sta-objs += common/p2p_ctrl.o
mt7601sta-objs += common/p2p_disc_mng.o
mt7601sta-objs += common/p2p_nego_mng.o
mt7601sta-objs += common/wsc.o
mt7601sta-objs += common/wsc_tlv.o
mt7601sta-objs += common/wsc_ufd.o
mt7601sta-objs += common/crypt_biginteger.o
mt7601sta-objs += common/crypt_dh.o
mt7601sta-objs += os/linux/ap_ioctl.o

EXTRA_CFLAGS += -DAGGREGATION_SUPPORT -DPIGGYBACK_SUPPORT -DWMM_SUPPORT -DLINUX -DSYSTEM_LOG_SUPPORT -DRESOURCE_PRE_ALLOC -DCONFIG_STA_SUPPORT -DSCAN_SUPPORT -DADHOC_WPA2PSK_SUPPORT -DWPA_SUPPLICANT_SUPPORT -DNATIVE_WPA_SUPPLICANT_SUPPORT -DWSC_STA_SUPPORT -DWSC_V2_SUPPORT -DRALINK_ATE -DCONFIG_RT2880_ATE_CMD_NEW -DRALINK_QA -DDOT11_N_SUPPORT -DSTATS_COUNT_SUPPORT -DRELEASE_EXCLUDE -DLED_CONTROL_SUPPORT -DMT7601U -DMT7601 -DRLT_MAC -DRLT_RF -DRTMP_MAC_USB -DRTMP_USB_SUPPORT -DRTMP_TIMER_TASK_SUPPORT -DRX_DMA_SCATTER -DVCORECAL_SUPPORT -DRTMP_EFUSE_SUPPORT -DNEW_MBSSID_MODE -DRTMP_INTERNAL_TX_ALC -DCONFIG_ANDES_SUPPORT -DCONFIG_RX_CSO_SUPPORT -DRTMP_FREQ_CALIBRATION_SUPPORT
EXTRA_CFLAGS += -DDYNAMIC_PD_SUPPORT -DUSB_BULK_BUF_PREALLOC -DRT_CFG80211_SUPPORT -DEXT_BUILD_CHANNEL_LIST
EXTRA_CFLAGS += -DUSB_BULK_BUF_ALIGMENT -DALIGMENT_BULKAGGRE_SIZE=3 -DBUF_ALIGMENT_RINGSIZE=2
EXTRA_CFLAGS += -DP2P_SUPPORT -DAPCLI_SUPPORT -DMAT_SUPPORT -DAP_SCAN_SUPPORT -DSCAN_SUPPORT -DP2P_APCLI_SUPPORT -DCONFIG_AP_SUPPORT -DUAPSD_SUPPORT -DMBSS_SUPPORT -DIAPP_SUPPORT -DDOT1X_SUPPORT -DWSC_AP_SUPPORT -DWSC_STA_SUPPORT

EXTRA_CFLAGS += -I$(PWD)/include -I$(PWD)/ate/include
EXTRA_CFLAGS += -Wno-address-of-packed-member -Wno-misleading-indentation -Wno-format-truncation -Wno-frame-larger-than -Wno-restrict -Wno-address -Wno-unused-variable

ifeq ($(CONFIG_RT5370V2_STA_WSC),y)
EXTRA_CFLAGS += -DWSC_STA_SUPPORT
ifeq ($(CONFIG_RT5370V2_STA_WSC_V2),y)
EXTRA_CFLAGS += -DWSC_V2_SUPPORT
endif
ifeq ($(CONFIG_RT5370V2_STA_WSC_LED),y)
EXTRA_CFLAGS += -DWSC_LED_SUPPORT
endif
endif

modules:
	$(MAKE) -C $(KSRC) M=$(PWD) modules

clean:
	$(MAKE) -C $(KSRC) M=$(PWD) clean
