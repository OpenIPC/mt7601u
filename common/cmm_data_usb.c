/*
   All functions in this file must be USB-depended, or you should out your function
	in other files.

*/

#ifdef RTMP_MAC_USB

#include	"rt_config.h"


NDIS_STATUS RTUSBFreeDescriptorRelease(RTMP_ADAPTER *pAd, UCHAR BulkOutPipeId)
{
	HT_TX_CONTEXT *pHTTXContext;
	unsigned long IrqFlags;


	pHTTXContext = &pAd->TxContext[BulkOutPipeId];
	RTMP_IRQ_LOCK(&pAd->TxContextQueueLock[BulkOutPipeId], IrqFlags);
	pHTTXContext->bCurWriting = FALSE;
	RTMP_IRQ_UNLOCK(&pAd->TxContextQueueLock[BulkOutPipeId], IrqFlags);

	return NDIS_STATUS_SUCCESS;
}


/*
	========================================================================

	Routine	Description:
		This subroutine will scan through releative ring descriptor to find
		out avaliable free ring descriptor and compare with request size.
		
	Arguments:
		pAd	Pointer	to our adapter
		RingType	Selected Ring
		
	Return Value:
		NDIS_STATUS_FAILURE		Not enough free descriptor
		NDIS_STATUS_SUCCESS		Enough free descriptor

	Note:
	
	========================================================================
*/
NDIS_STATUS	RTUSBFreeDescRequest(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR BulkOutPipeId,
	IN UINT32 req_cnt)
{
	NDIS_STATUS	 Status = NDIS_STATUS_FAILURE;
	unsigned long IrqFlags;
	HT_TX_CONTEXT *pHTTXContext;


	pHTTXContext = &pAd->TxContext[BulkOutPipeId];
	RTMP_IRQ_LOCK(&pAd->TxContextQueueLock[BulkOutPipeId], IrqFlags);
#ifdef USB_BULK_BUF_ALIGMENT
	if( ((pHTTXContext->CurWriteIdx< pHTTXContext->NextBulkIdx  ) &&   (pHTTXContext->NextBulkIdx - pHTTXContext->CurWriteIdx == 1)) 
		|| ((pHTTXContext->CurWriteIdx ==(BUF_ALIGMENT_RINGSIZE -1) ) &&  (pHTTXContext->NextBulkIdx == 0 )))
	{
		RTUSB_SET_BULK_FLAG(pAd, (fRTUSB_BULK_OUT_DATA_NORMAL << BulkOutPipeId));

	}
	else if (pHTTXContext->bCurWriting == TRUE)
	{
		DBGPRINT(RT_DEBUG_TRACE,("BUF_ALIGMENT RTUSBFreeD c3 --> QueIdx=%d, CWPos=%ld, NBOutPos=%ld!\n", BulkOutPipeId, pHTTXContext->CurWritePosition, pHTTXContext->NextBulkOutPosition));
		RTUSB_SET_BULK_FLAG(pAd, (fRTUSB_BULK_OUT_DATA_NORMAL << BulkOutPipeId));
	}

#else
	if ((pHTTXContext->CurWritePosition < pHTTXContext->NextBulkOutPosition) && ((pHTTXContext->CurWritePosition + req_cnt + LOCAL_TXBUF_SIZE) > pHTTXContext->NextBulkOutPosition))
	{
		
		RTUSB_SET_BULK_FLAG(pAd, (fRTUSB_BULK_OUT_DATA_NORMAL << BulkOutPipeId));
	}
	else if ((pHTTXContext->CurWritePosition == 8) && (pHTTXContext->NextBulkOutPosition < (req_cnt + LOCAL_TXBUF_SIZE)))
	{
		RTUSB_SET_BULK_FLAG(pAd, (fRTUSB_BULK_OUT_DATA_NORMAL << BulkOutPipeId));
	}
	else if (pHTTXContext->bCurWriting == TRUE)
	{
		DBGPRINT(RT_DEBUG_TRACE,("RTUSBFreeD c3 --> QueIdx=%d, CWPos=%ld, NBOutPos=%ld!\n", BulkOutPipeId, pHTTXContext->CurWritePosition, pHTTXContext->NextBulkOutPosition));
		RTUSB_SET_BULK_FLAG(pAd, (fRTUSB_BULK_OUT_DATA_NORMAL << BulkOutPipeId));
	}
#endif /* USB_BULK_BUF_ALIGMENT */
	else
	{
		Status = NDIS_STATUS_SUCCESS;
	}
	RTMP_IRQ_UNLOCK(&pAd->TxContextQueueLock[BulkOutPipeId], IrqFlags);

	
	return Status;
}


BOOLEAN	RTUSBNeedQueueBackForAgg(RTMP_ADAPTER *pAd, UCHAR BulkOutPipeId)
{
	HT_TX_CONTEXT *pHTTXContext;
	BOOLEAN needQueBack = FALSE;
	unsigned long   IrqFlags;


	pHTTXContext = &pAd->TxContext[BulkOutPipeId];
	
	RTMP_IRQ_LOCK(&pAd->TxContextQueueLock[BulkOutPipeId], IrqFlags);
	if ((pHTTXContext->IRPPending == TRUE)  /*&& (pAd->TxSwQueue[BulkOutPipeId].Number == 0) */)
	{
		if ((pHTTXContext->CurWritePosition < pHTTXContext->ENextBulkOutPosition) && 
			(((pHTTXContext->ENextBulkOutPosition+MAX_AGGREGATION_SIZE) < MAX_TXBULK_LIMIT) || (pHTTXContext->CurWritePosition > MAX_AGGREGATION_SIZE)))
		{
			needQueBack = TRUE;
		}
		else if ((pHTTXContext->CurWritePosition > pHTTXContext->ENextBulkOutPosition) && 
				 ((pHTTXContext->ENextBulkOutPosition + MAX_AGGREGATION_SIZE) < pHTTXContext->CurWritePosition))
		{
			needQueBack = TRUE;
		}
	}
	RTMP_IRQ_UNLOCK(&pAd->TxContextQueueLock[BulkOutPipeId], IrqFlags);

	return needQueBack;

}


/*
	========================================================================
	
	Routine	Description:
		Calculates the duration which is required to transmit out frames 
	with given size and specified rate.
		
	Arguments:
		pTxD		Pointer to transmit descriptor
		Ack			Setting for Ack requirement bit
		Fragment	Setting for Fragment bit
		RetryMode	Setting for retry mode
		Ifs			Setting for IFS gap
		Rate		Setting for transmit rate
		Service		Setting for service
		Length		Frame length
		TxPreamble  Short or Long preamble when using CCK rates
		QueIdx - 0-3, according to 802.11e/d4.4 June/2003
		
	Return Value:
		None
		
	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL
	
	========================================================================
*/
static VOID rlt_usb_write_txinfo(
	IN RTMP_ADAPTER *pAd,
	IN TXINFO_STRUC *pTxInfo,
	IN USHORT USBDMApktLen,
	IN BOOLEAN bWiv,
	IN UCHAR QueueSel,
	IN UCHAR NextValid,
	IN UCHAR TxBurst,
	IN UCHAR pkt_80211 )
{
#ifdef RLT_MAC
	struct _TXINFO_NMAC_PKT *nmac_info;

	nmac_info = (struct _TXINFO_NMAC_PKT *)pTxInfo;
#ifdef HDR_TRANS_SUPPORT
	nmac_info->pkt_80211 = pkt_80211;
#else
	nmac_info->pkt_80211 = 1;
#endif /* HDR_TRANS_SUPPORT */
	nmac_info->info_type = 0;
	nmac_info->d_port = 0;
	nmac_info->cso = 0;
	nmac_info->tso = 0;
#endif /* RLT_MAC */



	pTxInfo->TxInfoPktLen = USBDMApktLen;
	pTxInfo->TxInfoQSEL = QueueSel;
	if (QueueSel != FIFO_EDCA)
		DBGPRINT(RT_DEBUG_TRACE, ("====> QueueSel != FIFO_EDCA <====\n"));
	pTxInfo->TxInfoUDMANextVld = FALSE; /*NextValid;   Need to check with Jan about this.*/
	pTxInfo->TxInfoUDMATxburst = TxBurst;
	pTxInfo->TxInfoWIV = bWiv;
	
#ifndef USB_BULK_BUF_ALIGMENT
	pTxInfo->TxInfoSwLstRnd = 0;
#else
#ifdef RTMP_MAC
	pTxInfo->bFragLasAlignmentsectiontRound = 0;
#endif /* RTMP_MAC */
#endif /* USB_BULK_BUF_ALIGMENT */

}

#if 0
static VOID rlt_usb_update_txinfo(
	IN RTMP_ADAPTER *pAd,
	IN TXINFO_STRUC *pTxInfo,
	IN TX_BLK *pTxBlk)
{
#ifdef RLT_MAC
#endif /* RLT_MAC */
}
#endif

#ifdef CONFIG_STA_SUPPORT
VOID ComposePsPoll(RTMP_ADAPTER *pAd)
{
	TXINFO_STRUC *pTxInfo;
	TXWI_STRUC *pTxWI;
	UINT8 TXWISize = pAd->chipCap.TXWISize;
	UCHAR *buf;
	USHORT data_len;


	DBGPRINT(RT_DEBUG_TRACE, ("ComposePsPoll\n"));
	NdisZeroMemory(&pAd->PsPollFrame, sizeof (PSPOLL_FRAME));

	pAd->PsPollFrame.FC.PwrMgmt = 0;
	pAd->PsPollFrame.FC.Type = BTYPE_CNTL;
	pAd->PsPollFrame.FC.SubType = SUBTYPE_PS_POLL;
	pAd->PsPollFrame.Aid = pAd->StaActive.Aid | 0xC000;
	COPY_MAC_ADDR(pAd->PsPollFrame.Bssid, pAd->CommonCfg.Bssid);
	COPY_MAC_ADDR(pAd->PsPollFrame.Ta, pAd->CurrentAddress);

	buf = &pAd->PsPollContext.TransferBuffer->field.WirelessPacket[0];
	pTxInfo = (TXINFO_STRUC *)buf;
	pTxWI = (TXWI_STRUC *)&buf[TXINFO_SIZE];
	RTMPZeroMemory(buf, 100);
	data_len = sizeof (PSPOLL_FRAME);
	rlt_usb_write_txinfo(pAd, pTxInfo, data_len + TXWISize + TSO_SIZE, TRUE, 
						EpToQueue[MGMTPIPEIDX], FALSE, FALSE, 1);
	RTMPWriteTxWI(pAd, pTxWI, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, 0,
		      BSSID_WCID, data_len, 0, 0,
		      (UCHAR) pAd->CommonCfg.MlmeTransmit.field.MCS,
		      IFS_BACKOFF, FALSE, &pAd->CommonCfg.MlmeTransmit);
	RTMPMoveMemory((VOID *)&buf[TXWISize + TXINFO_SIZE + TSO_SIZE], (VOID *)&pAd->PsPollFrame, data_len);
	/* Append 4 extra zero bytes. */
	pAd->PsPollContext.BulkOutSize = TXINFO_SIZE + TXWISize + TSO_SIZE + data_len + 4;
}
#endif /* CONFIG_STA_SUPPORT */


/* IRQL = DISPATCH_LEVEL */
VOID ComposeNullFrame(RTMP_ADAPTER *pAd)
{
	TXINFO_STRUC *pTxInfo;
	TXWI_STRUC *pTxWI;
	UCHAR *buf;
	UINT8 TXWISize = pAd->chipCap.TXWISize;
	USHORT data_len = sizeof(pAd->NullFrame);;


	NdisZeroMemory(&pAd->NullFrame, data_len);
	pAd->NullFrame.FC.Type = BTYPE_DATA;
	pAd->NullFrame.FC.SubType = SUBTYPE_NULL_FUNC;
	pAd->NullFrame.FC.ToDs = 1;
	COPY_MAC_ADDR(pAd->NullFrame.Addr1, pAd->CommonCfg.Bssid);
	COPY_MAC_ADDR(pAd->NullFrame.Addr2, pAd->CurrentAddress);
	COPY_MAC_ADDR(pAd->NullFrame.Addr3, pAd->CommonCfg.Bssid);
	buf = &pAd->NullContext.TransferBuffer->field.WirelessPacket[0];
	RTMPZeroMemory(buf, 100);
	pTxInfo = (TXINFO_STRUC *)buf;
	pTxWI = (TXWI_STRUC *)&buf[TXINFO_SIZE];
	rlt_usb_write_txinfo(pAd, pTxInfo,
			(USHORT)(data_len + TXWISize + TSO_SIZE), TRUE,
			EpToQueue[MGMTPIPEIDX], FALSE, FALSE, 1);
	RTMPWriteTxWI(pAd, pTxWI, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, 0,
		      BSSID_WCID, data_len, 0, 0,
		      (UCHAR)pAd->CommonCfg.MlmeTransmit.field.MCS,
		      IFS_BACKOFF, FALSE, &pAd->CommonCfg.MlmeTransmit);
	RTMPMoveMemory((VOID *)&buf[TXWISize + TXINFO_SIZE], (VOID *)&pAd->NullFrame, data_len);
	pAd->NullContext.BulkOutSize = TXINFO_SIZE + TXWISize + TSO_SIZE + data_len + 4;
}


/*
	We can do copy the frame into pTxContext when match following conditions.
		=>
		=>
		=>
*/
static inline NDIS_STATUS RtmpUSBCanDoWrite(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR QueIdx,
	IN HT_TX_CONTEXT *pHTTXContext)
{
	NDIS_STATUS	canWrite = NDIS_STATUS_RESOURCES;

#ifdef USB_BULK_BUF_ALIGMENT
	if( ((pHTTXContext->CurWriteIdx< pHTTXContext->NextBulkIdx  ) &&   (pHTTXContext->NextBulkIdx - pHTTXContext->CurWriteIdx == 1)) 
		|| ((pHTTXContext->CurWriteIdx ==(BUF_ALIGMENT_RINGSIZE -1) ) &&  (pHTTXContext->NextBulkIdx == 0 )))
	{
		DBGPRINT(RT_DEBUG_ERROR,("RtmpUSBCanDoWrite USB_BULK_BUF_ALIGMENT c1!\n"));
		RTUSB_SET_BULK_FLAG(pAd, (fRTUSB_BULK_OUT_DATA_NORMAL << QueIdx));		
	}
	else if (pHTTXContext->bCurWriting == TRUE)
	{
		DBGPRINT(RT_DEBUG_ERROR,("RtmpUSBCanDoWrite USB_BULK_BUF_ALIGMENT c3!!\n"));

	}
#else

	if (((pHTTXContext->CurWritePosition) < pHTTXContext->NextBulkOutPosition) && (pHTTXContext->CurWritePosition + LOCAL_TXBUF_SIZE) > pHTTXContext->NextBulkOutPosition)
	{
		DBGPRINT(RT_DEBUG_ERROR,("RtmpUSBCanDoWrite c1!\n"));
		RTUSB_SET_BULK_FLAG(pAd, (fRTUSB_BULK_OUT_DATA_NORMAL << QueIdx));
	}
	else if ((pHTTXContext->CurWritePosition == 8) && (pHTTXContext->NextBulkOutPosition < LOCAL_TXBUF_SIZE))
	{
		DBGPRINT(RT_DEBUG_ERROR,("RtmpUSBCanDoWrite c2!\n"));
		RTUSB_SET_BULK_FLAG(pAd, (fRTUSB_BULK_OUT_DATA_NORMAL << QueIdx));
	}
	else if (pHTTXContext->bCurWriting == TRUE)
	{
		DBGPRINT(RT_DEBUG_ERROR,("RtmpUSBCanDoWrite c3!\n"));
	}
	else if ((pHTTXContext->ENextBulkOutPosition == 8)  && ((pHTTXContext->CurWritePosition + 7912 ) > MAX_TXBULK_LIMIT)  )
	{
		DBGPRINT(RT_DEBUG_ERROR,("RtmpUSBCanDoWrite c4!\n"));
		RTUSB_SET_BULK_FLAG(pAd, (fRTUSB_BULK_OUT_DATA_NORMAL << QueIdx));
	}

#endif /* USB_BULK_BUF_ALIGMENT */
	else
	{
		canWrite = NDIS_STATUS_SUCCESS;
	}
	

	return canWrite;
}


USHORT RtmpUSB_WriteSubTxResource(
	IN RTMP_ADAPTER *pAd,
	IN TX_BLK *pTxBlk,
	IN BOOLEAN bIsLast,
	OUT	USHORT *freeCnt)
{

	/* Dummy function. Should be removed in the future.*/
	return 0;
	
}

USHORT	RtmpUSB_WriteFragTxResource(
	IN RTMP_ADAPTER *pAd,
	IN TX_BLK *pTxBlk,
	IN UCHAR fragNum,
	OUT	USHORT *freeCnt)
{
	HT_TX_CONTEXT	*pHTTXContext;
	USHORT			hwHdrLen;	/* The hwHdrLen consist of 802.11 header length plus the header padding length.*/
	UINT32			fillOffset;
	TXINFO_STRUC	*pTxInfo;
	TXWI_STRUC		*pTxWI;
	PUCHAR			pWirelessPacket = NULL;
	UCHAR			QueIdx;
	NDIS_STATUS		Status;
	unsigned long	IrqFlags;
	UINT32			USBDMApktLen = 0, DMAHdrLen, padding;
#ifdef USB_BULK_BUF_ALIGMENT	
	BOOLEAN			bLasAlignmentsectiontRound = FALSE;
	INT idx=0;
#else
	BOOLEAN			TxQLastRound = FALSE;
#endif /* USB_BULK_BUF_ALIGMENT */
	UINT8 TXWISize = pAd->chipCap.TXWISize;
	
	
	/* get Tx Ring Resource & Dma Buffer address*/
	
	QueIdx = pTxBlk->QueIdx;
	pHTTXContext  = &pAd->TxContext[QueIdx];

	RTMP_IRQ_LOCK(&pAd->TxContextQueueLock[QueIdx], IrqFlags);
	
	pHTTXContext  = &pAd->TxContext[QueIdx];
	fillOffset = pHTTXContext->CurWritePosition;

	if(fragNum == 0)
	{
		/* Check if we have enough space for this bulk-out batch.*/
		Status = RtmpUSBCanDoWrite(pAd, QueIdx, pHTTXContext);
		if (Status == NDIS_STATUS_SUCCESS)
		{
			pHTTXContext->bCurWriting = TRUE;

#ifndef USB_BULK_BUF_ALIGMENT			
			/* Reserve space for 8 bytes padding.*/
			if ((pHTTXContext->ENextBulkOutPosition == pHTTXContext->CurWritePosition))
			{
				pHTTXContext->ENextBulkOutPosition += 8;
				pHTTXContext->CurWritePosition += 8;
				fillOffset += 8;
			}
#endif /* USB_BULK_BUF_ALIGMENT */
			pTxBlk->Priv = 0;
			pHTTXContext->CurWriteRealPos = pHTTXContext->CurWritePosition;
		}
		else
		{
			RTMP_IRQ_UNLOCK(&pAd->TxContextQueueLock[QueIdx], IrqFlags);
			
			RELEASE_NDIS_PACKET(pAd, pTxBlk->pPacket, NDIS_STATUS_FAILURE);
			return(Status);
		}
	}
	else 
	{
		/* For sub-sequent frames of this bulk-out batch. Just copy it to our bulk-out buffer.*/
		Status = ((pHTTXContext->bCurWriting == TRUE) ? NDIS_STATUS_SUCCESS : NDIS_STATUS_FAILURE);
		if (Status == NDIS_STATUS_SUCCESS)
		{
			fillOffset += pTxBlk->Priv;
		}
		else 
		{
			RTMP_IRQ_UNLOCK(&pAd->TxContextQueueLock[QueIdx], IrqFlags);
			
			RELEASE_NDIS_PACKET(pAd, pTxBlk->pPacket, NDIS_STATUS_FAILURE);
			return(Status);
		}
	}
	
	NdisZeroMemory((PUCHAR)(&pTxBlk->HeaderBuf[0]), TXINFO_SIZE);
	pTxInfo = (TXINFO_STRUC *)(&pTxBlk->HeaderBuf[0]);
	pTxWI= (TXWI_STRUC *)(&pTxBlk->HeaderBuf[TXINFO_SIZE]);

#ifdef USB_BULK_BUF_ALIGMENT
	idx = pHTTXContext->CurWriteIdx;
	pWirelessPacket = &pHTTXContext->TransferBuffer[idx]->field.WirelessPacket[fillOffset];
#else
	pWirelessPacket = &pHTTXContext->TransferBuffer->field.WirelessPacket[fillOffset];
#endif /* USB_BULK_BUF_ALIGMENT */

	/* copy TXWI + WLAN Header + LLC into DMA Header Buffer*/
	/*hwHdrLen = ROUND_UP(pTxBlk->MpduHeaderLen, 4);*/
	hwHdrLen = pTxBlk->MpduHeaderLen + pTxBlk->HdrPadLen;

	/* Build our URB for USBD*/
	DMAHdrLen = TXWISize + hwHdrLen;
	USBDMApktLen = DMAHdrLen + pTxBlk->SrcBufLen;
	padding = (4 - (USBDMApktLen % 4)) & 0x03;	/* round up to 4 byte alignment*/
	USBDMApktLen += padding;

	pTxBlk->Priv += (TXINFO_SIZE + USBDMApktLen);

	/* For TxInfo, the length of USBDMApktLen = TXWI_SIZE + 802.11 header + payload*/
	rlt_usb_write_txinfo(pAd, pTxInfo, (USHORT)(USBDMApktLen), FALSE, FIFO_EDCA, FALSE /*NextValid*/,  FALSE, 1);
	
	if (fragNum == pTxBlk->TotalFragNum) 
	{
		pTxInfo->TxInfoUDMATxburst = 0;

#ifdef USB_BULK_BUF_ALIGMENT	
		/*
			when CurWritePosition > 0x6000  mean that it is at the max bulk out  size,
			we CurWriteIdx must move to the next alignment section.
			Otherwirse,  CurWriteIdx will be moved to the next section at databulkout.

			
			(((pHTTXContext->CurWritePosition + 3906)& 0x00007fff) & 0xffff6000) == 0x00006000)
			we must make sure that the last fragNun packet just over the 0x6000
			otherwise it will error because the last frag packet will at the section but will not bulk out.
			ex:   when secoend packet writeresouce and it > 0x6000
				And the last packet writesource and it also > 0x6000  at this time CurWriteIdx++
				but when data bulk out , because at second packet it will > 0x6000 , the last packet will not bulk out.
			
		*/

		if ( ((pHTTXContext->CurWritePosition + 3906)  & BULKOUT_SIZE) == BULKOUT_SIZE)
		{

			bLasAlignmentsectiontRound = TRUE;
			pTxInfo->TxInfoSwLstRnd = 1;
		}
#else
		if ((pHTTXContext->CurWritePosition + pTxBlk->Priv + 3906)> MAX_TXBULK_LIMIT)
		{
			pTxInfo->TxInfoSwLstRnd = 1;
			TxQLastRound = TRUE;
		}
#endif /* USB_BULK_BUF_ALIGMENT */
	}
	else
	{
		pTxInfo->TxInfoUDMATxburst = 1;
	}

	NdisMoveMemory(pWirelessPacket, pTxBlk->HeaderBuf, TXINFO_SIZE + TXWISize + hwHdrLen); 
#ifdef RT_BIG_ENDIAN
	RTMPFrameEndianChange(pAd, (PUCHAR)(pWirelessPacket + TXINFO_SIZE + TXWISize), DIR_WRITE, FALSE);
#endif /* RT_BIG_ENDIAN */
	pWirelessPacket += (TXINFO_SIZE + TXWISize + hwHdrLen);
	pHTTXContext->CurWriteRealPos += (TXINFO_SIZE + TXWISize + hwHdrLen);
	
	RTMP_IRQ_UNLOCK(&pAd->TxContextQueueLock[QueIdx], IrqFlags);
	
	NdisMoveMemory(pWirelessPacket, pTxBlk->pSrcBufData, pTxBlk->SrcBufLen);

	/*	Zero the last padding.*/
	pWirelessPacket += pTxBlk->SrcBufLen;
	NdisZeroMemory(pWirelessPacket, padding + 8);

	if (fragNum == pTxBlk->TotalFragNum)
	{
		RTMP_IRQ_LOCK(&pAd->TxContextQueueLock[QueIdx], IrqFlags);
		
		/* Update the pHTTXContext->CurWritePosition. 3906 used to prevent the NextBulkOut is a A-RALINK/A-MSDU Frame.*/
		pHTTXContext->CurWritePosition += pTxBlk->Priv;
#ifndef USB_BULK_BUF_ALIGMENT	
		if (TxQLastRound == TRUE)
			pHTTXContext->CurWritePosition = 8;
#endif /* USB_BULK_BUF_ALIGMENT */
#ifdef USB_BULK_BUF_ALIGMENT
		if(bLasAlignmentsectiontRound == TRUE)
		{	
			CUR_WRITE_IDX_INC(pHTTXContext->CurWriteIdx, BUF_ALIGMENT_RINGSIZE);
			pHTTXContext->CurWritePosition = 0;
		}
#endif /* USB_BULK_BUF_ALIGMENT */

		pHTTXContext->CurWriteRealPos = pHTTXContext->CurWritePosition;

#ifdef UAPSD_SUPPORT
#ifdef DOT11Z_TDLS_SUPPORT
		UAPSD_TagFrame(pAd, pTxBlk->pPacket, pTxBlk->Wcid, pHTTXContext->CurWritePosition);
#else
#ifdef CONFIG_AP_SUPPORT
#ifdef P2P_SUPPORT
		if (P2P_GO_ON(pAd))
#else
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
#endif /* P2P_SUPPORT */
		{
			UAPSD_TagFrame(pAd, pTxBlk->pPacket, pTxBlk->Wcid, pHTTXContext->CurWritePosition);
		}
#endif /* CONFIG_AP_SUPPORT */
#endif /* DOT11Z_TDLS_SUPPORT */
#endif /* UAPSD_SUPPORT */

		/* Finally, set bCurWriting as FALSE*/
	pHTTXContext->bCurWriting = FALSE;

		RTMP_IRQ_UNLOCK(&pAd->TxContextQueueLock[QueIdx], IrqFlags);

		/* succeed and release the skb buffer*/
		RELEASE_NDIS_PACKET(pAd, pTxBlk->pPacket, NDIS_STATUS_SUCCESS);
	}

		
	return(Status);
	
}


USHORT RtmpUSB_WriteSingleTxResource(
	IN RTMP_ADAPTER *pAd,
	IN TX_BLK *pTxBlk,
	IN BOOLEAN bIsLast,
	OUT	USHORT *freeCnt)
{
	HT_TX_CONTEXT *pHTTXContext;
	UINT32 fillOffset;
	TXINFO_STRUC *pTxInfo;
	TXWI_STRUC *pTxWI;
	UCHAR *pWirelessPacket, *buf;
	UCHAR QueIdx;
	unsigned long	IrqFlags;
	NDIS_STATUS Status;
	UINT32 hdr_copy_len, hdr_len, dma_len = 0, padding;
#ifdef USB_BULK_BUF_ALIGMENT
	INT idx=0;
#endif /* USB_BULK_BUF_ALIGMENT */	

#ifndef USB_BULK_BUF_ALIGMENT
	BOOLEAN bTxQLastRound = FALSE;
#endif /* USB_BULK_BUF_ALIGMENT */		
	UINT8 TXWISize = pAd->chipCap.TXWISize;

	/* get Tx Ring Resource & Dma Buffer address*/
	QueIdx = pTxBlk->QueIdx;

	RTMP_IRQ_LOCK(&pAd->TxContextQueueLock[QueIdx], IrqFlags);
	pHTTXContext  = &pAd->TxContext[QueIdx];
	fillOffset = pHTTXContext->CurWritePosition;

	
	
	/* Check ring full */
	Status = RtmpUSBCanDoWrite(pAd, QueIdx, pHTTXContext);
	if(Status == NDIS_STATUS_SUCCESS)
	{
		pHTTXContext->bCurWriting = TRUE;
		buf = &pTxBlk->HeaderBuf[0];
		pTxInfo = (TXINFO_STRUC *)buf;
		pTxWI= (TXWI_STRUC *)&buf[TXINFO_SIZE];

#ifndef USB_BULK_BUF_ALIGMENT
		/* Reserve space for 8 bytes padding.*/
		if ((pHTTXContext->ENextBulkOutPosition == pHTTXContext->CurWritePosition))
		{
			pHTTXContext->ENextBulkOutPosition += 8;
			pHTTXContext->CurWritePosition += 8;
			fillOffset += 8;
		}
#endif /* USB_BULK_BUF_ALIGMENT */
		pHTTXContext->CurWriteRealPos = pHTTXContext->CurWritePosition;
		
#ifdef USB_BULK_BUF_ALIGMENT
		idx = pHTTXContext->CurWriteIdx;
		pWirelessPacket = &pHTTXContext->TransferBuffer[idx]->field.WirelessPacket[fillOffset];
#else
		pWirelessPacket = &pHTTXContext->TransferBuffer->field.WirelessPacket[fillOffset];
#endif /* USB_BULK_BUF_ALIGMENT */

		/* Build our URB for USBD */
		hdr_len = TXWISize + TSO_SIZE + pTxBlk->MpduHeaderLen + pTxBlk->HdrPadLen;
		hdr_copy_len = TXINFO_SIZE + hdr_len;
		dma_len = hdr_len + pTxBlk->SrcBufLen;
		padding = (4 - (dma_len % 4)) & 0x03;	/* round up to 4 byte alignment*/
		dma_len += padding;

		pTxBlk->Priv = (TXINFO_SIZE + dma_len);

		/* For TxInfo, the length of USBDMApktLen = TXWI_SIZE + TSO_SIZE + 802.11 header + payload */
#ifdef HDR_TRANS_SUPPORT
		rlt_usb_write_txinfo(pAd, pTxInfo, (USHORT)(dma_len), FALSE, FIFO_EDCA, FALSE /*NextValid*/,  FALSE, pTxBlk->NeedTrans?0:1);
#else

		rlt_usb_write_txinfo(pAd, pTxInfo, (USHORT)(dma_len), FALSE, FIFO_EDCA, FALSE /*NextValid*/,  FALSE, 1);
#endif /* HDR_TRANS_SUPPORT */


#ifndef USB_BULK_BUF_ALIGMENT
		if ((pHTTXContext->CurWritePosition + 3906 + pTxBlk->Priv) > MAX_TXBULK_LIMIT)
		{
			pTxInfo->TxInfoSwLstRnd = 1;
			bTxQLastRound = TRUE;
		}
#endif /* USB_BULK_BUF_ALIGMENT */

		NdisMoveMemory(pWirelessPacket, pTxBlk->HeaderBuf, hdr_copy_len);
#ifdef RT_BIG_ENDIAN
		RTMPFrameEndianChange(pAd, (PUCHAR)(pWirelessPacket + TXINFO_SIZE + TXWISize + TSO_SIZE), DIR_WRITE, FALSE);
#endif /* RT_BIG_ENDIAN */
		pWirelessPacket += (hdr_copy_len);

		/* We unlock it here to prevent the first 8 bytes maybe over-writed issue.*/
		/*	1. First we got CurWritePosition but the first 8 bytes still not write to the pTxcontext.*/
		/*	2. An interrupt break our routine and handle bulk-out complete.*/
		/*	3. In the bulk-out compllete, it need to do another bulk-out, */
		/*			if the ENextBulkOutPosition is just the same as CurWritePosition, it will save the first 8 bytes from CurWritePosition,*/
		/*			but the payload still not copyed. the pTxContext->SavedPad[] will save as allzero. and set the bCopyPad = TRUE.*/
		/*	4. Interrupt complete.*/
		/*  5. Our interrupted routine go back and fill the first 8 bytes to pTxContext.*/
		/*	6. Next time when do bulk-out, it found the bCopyPad==TRUE and will copy the SavedPad[] to pTxContext->NextBulkOutPosition.*/
		/*		and the packet will wrong.*/
		pHTTXContext->CurWriteRealPos += hdr_copy_len;
#ifndef USB_BULK_BUF_ALIGMENT
		RTMP_IRQ_UNLOCK(&pAd->TxContextQueueLock[QueIdx], IrqFlags);
#endif /* USB_BULK_BUF_ALIGMENT */

#ifdef TX_PKT_SG
		if (pTxBlk->pkt_info.BufferCount > 1) {
			INT i, len;
			void *data;
			PKT_SG_T *sg = &pTxBlk->pkt_info.sg_list[0];
			
			for (i = 0 ; i < pTxBlk->pkt_info.BufferCount; i++) {
				data = sg[i].data;
				len = sg[i].len;
				if (i == 0) {
					len -= ((ULONG)pTxBlk->pSrcBufData - (ULONG)sg[i].data);
					data = pTxBlk->pSrcBufData;
				}
				//DBGPRINT(RT_DEBUG_TRACE, ("%s:sg[%d]=0x%x, len=%d\n", __FUNCTION__, i, data, len));
				if (len <= 0) {
					DBGPRINT(RT_DEBUG_ERROR, ("%s():sg[%d] info error, sg.data=0x%x, sg.len=%d, pTxBlk->pSrcBufData=0x%x, pTxBlk->SrcBufLen=%d, data=0x%x, len=%d\n",
								__FUNCTION__, i, sg[i].data, sg[i].len, pTxBlk->pSrcBufData, pTxBlk->SrcBufLen, data, len));
					break;
				}
				NdisMoveMemory(pWirelessPacket, data, len);
				pWirelessPacket += len;
			}
		}
		else
#endif /* TX_PKT_SG */
		{
			NdisMoveMemory(pWirelessPacket, pTxBlk->pSrcBufData, pTxBlk->SrcBufLen);
			pWirelessPacket += pTxBlk->SrcBufLen;
		}

#ifndef USB_BULK_BUF_ALIGMENT
		NdisZeroMemory(pWirelessPacket, padding + 8);
		RTMP_IRQ_LOCK(&pAd->TxContextQueueLock[QueIdx], IrqFlags);
#endif /* USB_BULK_BUF_ALIGMENT */

		pHTTXContext->CurWritePosition += pTxBlk->Priv;
#ifdef UAPSD_SUPPORT
#ifdef DOT11Z_TDLS_SUPPORT
		UAPSD_TagFrame(pAd, pTxBlk->pPacket, pTxBlk->Wcid, pHTTXContext->CurWritePosition);
#else
#ifdef CONFIG_AP_SUPPORT
#ifdef P2P_SUPPORT
		if (P2P_GO_ON(pAd))
#else
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
#endif /* P2P_SUPPORT */
		{
			UAPSD_TagFrame(pAd, pTxBlk->pPacket, pTxBlk->Wcid, pHTTXContext->CurWritePosition);
		}
#endif /* CONFIG_AP_SUPPORT */
#endif /* DOT11Z_TDLS_SUPPORT */
#endif /* UAPSD_SUPPORT */
#ifdef USB_BULK_BUF_ALIGMENT
		/*
			when CurWritePosition > 0x6000  mean that it is at the max bulk out size,
			we CurWriteIdx must move to the next alignment section.
			Otherwirse,  CurWriteIdx will be moved to the next section at databulkout.

			Writingflag = TRUE ,mean that when we writing resource ,and databulkout happen, 
			So we bulk out when this packet finish.
		*/
		if ( (pHTTXContext->CurWritePosition  & BULKOUT_SIZE) == BULKOUT_SIZE)
		{
			CUR_WRITE_IDX_INC(pHTTXContext->CurWriteIdx, BUF_ALIGMENT_RINGSIZE);
			pHTTXContext->CurWritePosition = 0;
		}
#else
		if (bTxQLastRound)
			pHTTXContext->CurWritePosition = 8;
#endif /* USB_BULK_BUF_ALIGMENT */

		pHTTXContext->CurWriteRealPos = pHTTXContext->CurWritePosition;
		pHTTXContext->bCurWriting = FALSE;
	}

	
	RTMP_IRQ_UNLOCK(&pAd->TxContextQueueLock[QueIdx], IrqFlags);


	/* succeed and release the skb buffer*/
	RELEASE_NDIS_PACKET(pAd, pTxBlk->pPacket, NDIS_STATUS_SUCCESS);
	
	return(Status);

}


USHORT RtmpUSB_WriteMultiTxResource(
	IN RTMP_ADAPTER *pAd,
	IN TX_BLK *pTxBlk,
	IN UCHAR frmNum,
	OUT USHORT *freeCnt)
{
	HT_TX_CONTEXT *pHTTXContext;
	USHORT hwHdrLen;	/* The hwHdrLen consist of 802.11 header length plus the header padding length.*/
	UINT32 fillOffset;
	TXINFO_STRUC *pTxInfo;
	TXWI_STRUC *pTxWI;
	UCHAR *pWirelessPacket = NULL;
	UCHAR QueIdx;
	NDIS_STATUS Status;
	unsigned long IrqFlags;
	UINT8 TXWISize = pAd->chipCap.TXWISize;
#ifdef USB_BULK_BUF_ALIGMENT
	INT idx=0;
#endif
	
	/* get Tx Ring Resource & Dma Buffer address*/
	QueIdx = pTxBlk->QueIdx;
	pHTTXContext  = &pAd->TxContext[QueIdx];

	RTMP_IRQ_LOCK(&pAd->TxContextQueueLock[QueIdx], IrqFlags);
	
	if(frmNum == 0)	
	{	
		/* Check if we have enough space for this bulk-out batch.*/
		Status = RtmpUSBCanDoWrite(pAd, QueIdx, pHTTXContext);
		if (Status == NDIS_STATUS_SUCCESS)
		{
			pHTTXContext->bCurWriting = TRUE;

			pTxInfo = (TXINFO_STRUC *)(&pTxBlk->HeaderBuf[0]);
			pTxWI= (TXWI_STRUC *)(&pTxBlk->HeaderBuf[TXINFO_SIZE]);
			
#ifndef USB_BULK_BUF_ALIGMENT	
			/* Reserve space for 8 bytes padding.*/
			if ((pHTTXContext->ENextBulkOutPosition == pHTTXContext->CurWritePosition))
			{
				
				pHTTXContext->CurWritePosition += 8;
				pHTTXContext->ENextBulkOutPosition += 8;
			}
#endif /* USB_BULK_BUF_ALIGMENT */
			fillOffset = pHTTXContext->CurWritePosition;
			pHTTXContext->CurWriteRealPos = pHTTXContext->CurWritePosition;
#ifdef USB_BULK_BUF_ALIGMENT
			idx = pHTTXContext->CurWriteIdx;
			pWirelessPacket = &pHTTXContext->TransferBuffer[idx]->field.WirelessPacket[fillOffset];
#else
			pWirelessPacket = &pHTTXContext->TransferBuffer->field.WirelessPacket[fillOffset];
#endif /* USB_BULK_BUF_ALIGMENT */
			
			/* Copy TXINFO + TXWI + WLAN Header + LLC into DMA Header Buffer*/
			
			if (pTxBlk->TxFrameType == TX_AMSDU_FRAME)
				/*hwHdrLen = ROUND_UP(pTxBlk->MpduHeaderLen-LENGTH_AMSDU_SUBFRAMEHEAD, 4)+LENGTH_AMSDU_SUBFRAMEHEAD;*/
				hwHdrLen = pTxBlk->MpduHeaderLen-LENGTH_AMSDU_SUBFRAMEHEAD + pTxBlk->HdrPadLen + LENGTH_AMSDU_SUBFRAMEHEAD;
			else if (pTxBlk->TxFrameType == TX_RALINK_FRAME)
				/*hwHdrLen = ROUND_UP(pTxBlk->MpduHeaderLen-LENGTH_ARALINK_HEADER_FIELD, 4)+LENGTH_ARALINK_HEADER_FIELD;*/
				hwHdrLen = pTxBlk->MpduHeaderLen-LENGTH_ARALINK_HEADER_FIELD + pTxBlk->HdrPadLen + LENGTH_ARALINK_HEADER_FIELD;
			else
				/*hwHdrLen = ROUND_UP(pTxBlk->MpduHeaderLen, 4);*/
				hwHdrLen = pTxBlk->MpduHeaderLen + pTxBlk->HdrPadLen;

			/* Update the pTxBlk->Priv.*/
			pTxBlk->Priv = TXINFO_SIZE + TXWISize + hwHdrLen;

			/*	pTxInfo->USBDMApktLen now just a temp value and will to correct latter.*/
			rlt_usb_write_txinfo(pAd, pTxInfo, (USHORT)(pTxBlk->Priv), FALSE, FIFO_EDCA, FALSE /*NextValid*/,  FALSE, 1);
			
			/* Copy it.*/
			NdisMoveMemory(pWirelessPacket, pTxBlk->HeaderBuf, pTxBlk->Priv); 
#ifdef RT_BIG_ENDIAN
			RTMPFrameEndianChange(pAd, (PUCHAR)(pWirelessPacket+ TXINFO_SIZE + TXWISize), DIR_WRITE, FALSE);
#endif /* RT_BIG_ENDIAN */
			pHTTXContext->CurWriteRealPos += pTxBlk->Priv;
			pWirelessPacket += pTxBlk->Priv;
		}
	}
	else
	{	/* For sub-sequent frames of this bulk-out batch. Just copy it to our bulk-out buffer.*/
	
		Status = ((pHTTXContext->bCurWriting == TRUE) ? NDIS_STATUS_SUCCESS : NDIS_STATUS_FAILURE);
		if (Status == NDIS_STATUS_SUCCESS)
		{
			fillOffset =  (pHTTXContext->CurWritePosition + pTxBlk->Priv);
#ifdef USB_BULK_BUF_ALIGMENT
			idx = pHTTXContext->CurWriteIdx;
			pWirelessPacket = &pHTTXContext->TransferBuffer[idx]->field.WirelessPacket[fillOffset];
#else
			pWirelessPacket = &pHTTXContext->TransferBuffer->field.WirelessPacket[fillOffset];
#endif /* USB_BULK_BUF_ALIGMENT */

			/*hwHdrLen = pTxBlk->MpduHeaderLen;*/
			NdisMoveMemory(pWirelessPacket, pTxBlk->HeaderBuf, pTxBlk->MpduHeaderLen);
			pWirelessPacket += (pTxBlk->MpduHeaderLen);
			pTxBlk->Priv += pTxBlk->MpduHeaderLen;
		}
		else
		{	/* It should not happened now unless we are going to shutdown.*/
			DBGPRINT(RT_DEBUG_ERROR, ("WriteMultiTxResource():bCurWriting is FALSE when handle sub-sequent frames.\n"));
			Status = NDIS_STATUS_FAILURE;
		}
	}


	/*
		We unlock it here to prevent the first 8 bytes maybe over-write issue.
		1. First we got CurWritePosition but the first 8 bytes still not write to the pTxContext.
		2. An interrupt break our routine and handle bulk-out complete.
		3. In the bulk-out compllete, it need to do another bulk-out,
			if the ENextBulkOutPosition is just the same as CurWritePosition, it will save the first 8 bytes from CurWritePosition,
			but the payload still not copyed. the pTxContext->SavedPad[] will save as allzero. and set the bCopyPad = TRUE.
		4. Interrupt complete.
		5. Our interrupted routine go back and fill the first 8 bytes to pTxContext.
		6. Next time when do bulk-out, it found the bCopyPad==TRUE and will copy the SavedPad[] to pTxContext->NextBulkOutPosition.
			and the packet will wrong.
	*/
	RTMP_IRQ_UNLOCK(&pAd->TxContextQueueLock[QueIdx], IrqFlags);

	if (Status != NDIS_STATUS_SUCCESS)
	{
		DBGPRINT(RT_DEBUG_ERROR,("WriteMultiTxResource: CWPos = %ld, NBOutPos = %ld.\n", pHTTXContext->CurWritePosition, pHTTXContext->NextBulkOutPosition));
		goto done;
	}

	/* Copy the frame content into DMA buffer and update the pTxBlk->Priv*/
	NdisMoveMemory(pWirelessPacket, pTxBlk->pSrcBufData, pTxBlk->SrcBufLen);
	pWirelessPacket += pTxBlk->SrcBufLen;
	pTxBlk->Priv += pTxBlk->SrcBufLen;

done:	
	/* Release the skb buffer here*/
	RELEASE_NDIS_PACKET(pAd, pTxBlk->pPacket, NDIS_STATUS_SUCCESS);	

	return(Status);

}


VOID RtmpUSB_FinalWriteTxResource(
	IN RTMP_ADAPTER	*pAd,
	IN TX_BLK *pTxBlk,
	IN USHORT totalMPDUSize,
	IN USHORT TxIdx)
{
	UCHAR			QueIdx;
	HT_TX_CONTEXT	*pHTTXContext;
	UINT32			fillOffset;
	TXINFO_STRUC	*pTxInfo;
	TXWI_STRUC		*pTxWI;
	UINT32			USBDMApktLen, padding;
	unsigned long	IrqFlags;
	PUCHAR			pWirelessPacket;
#ifdef USB_BULK_BUF_ALIGMENT
	INT idx=0;
#endif /* USB_BULK_BUF_ALIGMENT */

	QueIdx = pTxBlk->QueIdx;
	pHTTXContext  = &pAd->TxContext[QueIdx];

#ifdef USB_BULK_BUF_ALIGMENT
	idx = pHTTXContext->CurWriteIdx;
#endif /* USB_BULK_BUF_ALIGMENT */
	RTMP_IRQ_LOCK(&pAd->TxContextQueueLock[QueIdx], IrqFlags);

	if (pHTTXContext->bCurWriting == TRUE)
	{		
		fillOffset = pHTTXContext->CurWritePosition;
#ifndef USB_BULK_BUF_ALIGMENT
		if (((pHTTXContext->ENextBulkOutPosition == pHTTXContext->CurWritePosition) || ((pHTTXContext->ENextBulkOutPosition-8) == pHTTXContext->CurWritePosition))
			&& (pHTTXContext->bCopySavePad == TRUE))
			pWirelessPacket = (PUCHAR)(&pHTTXContext->SavedPad[0]);
		else 
#endif /* USB_BULK_BUF_ALIGMENT */
#ifdef USB_BULK_BUF_ALIGMENT
			pWirelessPacket = (PUCHAR)(&pHTTXContext->TransferBuffer[idx]->field.WirelessPacket[fillOffset]);
#else
			pWirelessPacket = (PUCHAR)(&pHTTXContext->TransferBuffer->field.WirelessPacket[fillOffset]);
#endif /* USB_BULK_BUF_ALIGMENT */

		
		/* Update TxInfo->USBDMApktLen , */
		/*		the length = TXWI_SIZE + 802.11_hdr + 802.11_hdr_pad + payload_of_all_batch_frames + Bulk-Out-padding*/
		
		pTxInfo = (TXINFO_STRUC *)(pWirelessPacket);

		/* Calculate the bulk-out padding*/
		USBDMApktLen = pTxBlk->Priv - TXINFO_SIZE;
		padding = (4 - (USBDMApktLen % 4)) & 0x03;	/* round up to 4 byte alignment*/
		USBDMApktLen += padding;
		
		pTxInfo->TxInfoPktLen = USBDMApktLen;

		
		/*
			Update TXWI->TxWIMPDUByteCnt,
				the length = 802.11 header + payload_of_all_batch_frames
		*/
		pTxWI= (TXWI_STRUC *)(pWirelessPacket + TXINFO_SIZE);
		pTxWI->TxWIMPDUByteCnt = totalMPDUSize;

		
		/* Update the pHTTXContext->CurWritePosition*/
		
		pHTTXContext->CurWritePosition += (TXINFO_SIZE + USBDMApktLen);
#ifdef USB_BULK_BUF_ALIGMENT
		/*
			when CurWritePosition > 0x6000  mean that it is at the max bulk out size,
			we CurWriteIdx must move to the next alignment section.
			Otherwirse,  CurWriteIdx will be moved to the next section at databulkout.

			Writingflag = TRUE ,mean that when we writing resource ,and databulkout happen, 
			So we bulk out when this packet finish.
		*/
		if ( (pHTTXContext->CurWritePosition  & BULKOUT_SIZE) == BULKOUT_SIZE)
		{
			CUR_WRITE_IDX_INC(pHTTXContext->CurWriteIdx, BUF_ALIGMENT_RINGSIZE);
			pHTTXContext->CurWritePosition = 0;
		}

#else
		if ((pHTTXContext->CurWritePosition + 3906)> MAX_TXBULK_LIMIT)
		{	/* Add 3906 for prevent the NextBulkOut packet size is a A-RALINK/A-MSDU Frame.*/
			pHTTXContext->CurWritePosition = 8;
			pTxInfo->TxInfoSwLstRnd = 1;
		}
#endif /* USB_BULK_BUF_ALIGMENT */		

		pHTTXContext->CurWriteRealPos = pHTTXContext->CurWritePosition;
		
#ifdef UAPSD_SUPPORT
#ifdef DOT11Z_TDLS_SUPPORT
		UAPSD_TagFrame(pAd, pTxBlk->pPacket, pTxBlk->Wcid, pHTTXContext->CurWritePosition);
#else
#ifdef CONFIG_AP_SUPPORT
#ifdef P2P_SUPPORT
		if (P2P_GO_ON(pAd))
#else
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
#endif /* P2P_SUPPORT */
		{
			UAPSD_TagFrame(pAd, pTxBlk->pPacket, pTxBlk->Wcid, pHTTXContext->CurWritePosition);
		}
#endif /* CONFIG_AP_SUPPORT */
#endif /* DOT11Z_TDLS_SUPPORT */
#endif /* UAPSD_SUPPORT */

		
		/*	Zero the last padding.*/
#ifdef USB_BULK_BUF_ALIGMENT	
		pWirelessPacket = (&pHTTXContext->TransferBuffer[idx]->field.WirelessPacket[fillOffset + pTxBlk->Priv]);
#else		
		pWirelessPacket = (&pHTTXContext->TransferBuffer->field.WirelessPacket[fillOffset + pTxBlk->Priv]);
#endif /* USB_BULK_BUF_ALIGMENT */
		NdisZeroMemory(pWirelessPacket, padding + 8);
		
		/* Finally, set bCurWriting as FALSE*/
		pHTTXContext->bCurWriting = FALSE;

	}
	else
	{	/* It should not happened now unless we are going to shutdown.*/
		DBGPRINT(RT_DEBUG_ERROR, ("FinalWriteTxResource():bCurWriting is FALSE when handle last frames.\n"));
	}
	
	RTMP_IRQ_UNLOCK(&pAd->TxContextQueueLock[QueIdx], IrqFlags);

}


VOID RtmpUSBDataLastTxIdx(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR QueIdx,
	IN USHORT TxIdx)
{
	/* DO nothing for USB.*/
}


/*
	When can do bulk-out:
		1. TxSwFreeIdx < TX_RING_SIZE;
			It means has at least one Ring entity is ready for bulk-out, kick it out.
		2. If TxSwFreeIdx == TX_RING_SIZE
			Check if the CurWriting flag is FALSE, if it's FALSE, we can do kick out.

*/
VOID RtmpUSBDataKickOut(
	IN RTMP_ADAPTER *pAd,
	IN TX_BLK *pTxBlk,
	IN UCHAR QueIdx)
{
	RTUSB_SET_BULK_FLAG(pAd, (fRTUSB_BULK_OUT_DATA_NORMAL << QueIdx));
	RTUSBKickBulkOut(pAd);

}


/*
	Must be run in Interrupt context
	This function handle RT2870 specific TxDesc and cpu index update and kick the packet out.
 */
int RtmpUSBMgmtKickOut(
	IN RTMP_ADAPTER *pAd, 
	IN UCHAR QueIdx,
	IN PNDIS_PACKET pPacket,
	IN UCHAR *pSrcBufVA,
	IN UINT SrcBufLen)
{
	TXINFO_STRUC *pTxInfo;
	ULONG BulkOutSize;
	UCHAR padLen;
	PUCHAR pDest;
	ULONG SwIdx = pAd->MgmtRing.TxCpuIdx;
	TX_CONTEXT *pMLMEContext = (PTX_CONTEXT)pAd->MgmtRing.Cell[SwIdx].AllocVa;
	ULONG IrqFlags;

	
	pTxInfo = (TXINFO_STRUC *)(pSrcBufVA);

	/* Build our URB for USBD*/
	BulkOutSize = (SrcBufLen + 3) & (~3);
	rlt_usb_write_txinfo(pAd, pTxInfo, (USHORT)(BulkOutSize - TXINFO_SIZE), TRUE, EpToQueue[MGMTPIPEIDX], FALSE,  FALSE, 1);
	
	BulkOutSize += 4; /* Always add 4 extra bytes at every packet.*/

//+++Add by shiang for debug
if (0) {
	DBGPRINT(RT_DEBUG_OFF, ("-->%s():shiang-6590, QueIdx=%d, SrcBufLen=%d\n", __FUNCTION__, QueIdx, SrcBufLen));
	dump_txinfo(pAd, pTxInfo);
	dumpTxWI(pAd, (TXWI_STRUC *)(pSrcBufVA + TXINFO_SIZE));
}
//---Add by shiang for debug

/* WY , it cause Tx hang on Amazon_SE , Max said the padding is useless*/
	/* If BulkOutSize is multiple of BulkOutMaxPacketSize, add extra 4 bytes again.*/
/*	if ((BulkOutSize % pAd->BulkOutMaxPacketSize) == 0)*/
/*		BulkOutSize += 4;*/

	padLen = BulkOutSize - SrcBufLen;
	ASSERT((padLen <= RTMP_PKT_TAIL_PADDING));
	
	/* Now memzero all extra padding bytes.*/
	pDest = (PUCHAR)(pSrcBufVA + SrcBufLen);
	OS_PKT_TAIL_BUF_EXTEND(pPacket, padLen);
	NdisZeroMemory(pDest, padLen);

	RTMP_IRQ_LOCK(&pAd->MLMEBulkOutLock, IrqFlags);
	
	pAd->MgmtRing.Cell[pAd->MgmtRing.TxCpuIdx].pNdisPacket = pPacket;
	pMLMEContext->TransferBuffer = (PTX_BUFFER)(GET_OS_PKT_DATAPTR(pPacket));

	/* Length in TxInfo should be 8 less than bulkout size.*/
	pMLMEContext->BulkOutSize = BulkOutSize;
	pMLMEContext->InUse = TRUE;
	pMLMEContext->bWaitingBulkOut = TRUE;

#ifdef UAPSD_SUPPORT
		/*
			If the packet is QoS Null frame, we mark the packet with its WCID;
			If not, we mark the packet with bc/mc WCID = 0.

			We will handle it in rtusb_mgmt_dma_done_tasklet().

			Even AP send a QoS Null frame but not EOSP frame in USB mode,
			then we will call UAPSD_SP_Close() and we will check
			pEntry->bAPSDFlagSPStart() so do not worry about it.
		*/
#ifdef DOT11Z_TDLS_SUPPORT
	if (RTMP_GET_PACKET_QOS_NULL(pPacket) != 0x00)
		pMLMEContext->Wcid = RTMP_GET_PACKET_WCID(pPacket);
	else
		pMLMEContext->Wcid = MCAST_WCID;
#else
#ifdef CONFIG_AP_SUPPORT
#ifdef P2P_SUPPORT
	if (P2P_GO_ON(pAd))
#else
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
#endif /* P2P_SUPPORT */
	{
		if (RTMP_GET_PACKET_QOS_NULL(pPacket) != 0x00)
			pMLMEContext->Wcid = RTMP_GET_PACKET_WCID(pPacket);
		else
			pMLMEContext->Wcid = MCAST_WCID;
	}
#endif /* CONFIG_AP_SUPPORT */
#endif /* DOT11Z_TDLS_SUPPORT */
#endif /* UAPSD_SUPPORT */

	/*hex_dump("RtmpUSBMgmtKickOut", &pMLMEContext->TransferBuffer->field.WirelessPacket[0], (pMLMEContext->BulkOutSize > 16 ? 16 : pMLMEContext->BulkOutSize));*/

/*
	pAd->RalinkCounters.KickTxCount++;
	pAd->RalinkCounters.OneSecTxDoneCount++;

	if (pAd->MgmtRing.TxSwFreeIdx == MGMT_RING_SIZE)
		needKickOut = TRUE;
*/

	/* Decrease the TxSwFreeIdx and Increase the TX_CTX_IDX*/
	pAd->MgmtRing.TxSwFreeIdx--;
	INC_RING_INDEX(pAd->MgmtRing.TxCpuIdx, MGMT_RING_SIZE);
	
	RTMP_IRQ_UNLOCK(&pAd->MLMEBulkOutLock, IrqFlags);	
	
	RTUSB_SET_BULK_FLAG(pAd, fRTUSB_BULK_OUT_MLME);
	/*if (needKickOut)*/
	RTUSBKickBulkOut(pAd);
	
	return 0;
}


VOID RtmpUSBNullFrameKickOut(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR QueIdx,
	IN UCHAR *pNullFrame,
	IN UINT32 frameLen)
{
	if (pAd->NullContext.InUse == FALSE)
	{
		PTX_CONTEXT pNullContext;
		TXINFO_STRUC *pTxInfo;
		TXWI_STRUC *pTxWI;
		UCHAR *pWirelessPkt;
		UINT8 TXWISize = pAd->chipCap.TXWISize;

		pNullContext = &(pAd->NullContext);

		/* Set the in use bit*/
		pNullContext->InUse = TRUE;
		pWirelessPkt = (PUCHAR)&pNullContext->TransferBuffer->field.WirelessPacket[0];

		RTMPZeroMemory(&pWirelessPkt[0], 100);
		pTxInfo = (TXINFO_STRUC *)&pWirelessPkt[0];
		rlt_usb_write_txinfo(pAd, pTxInfo, (USHORT)(frameLen + TXWISize + TSO_SIZE), TRUE, EpToQueue[MGMTPIPEIDX], FALSE,  FALSE, 1);
		pTxInfo->TxInfoQSEL = FIFO_EDCA;
		pTxWI = (TXWI_STRUC *)&pWirelessPkt[TXINFO_SIZE];
		RTMPWriteTxWI(pAd, pTxWI,  FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, 0, BSSID_WCID, frameLen,
			0, 0, (UCHAR)pAd->CommonCfg.MlmeTransmit.field.MCS, IFS_HTTXOP, FALSE, &pAd->CommonCfg.MlmeTransmit);
#ifdef RT_BIG_ENDIAN
		RTMPWIEndianChange(pAd, (PUCHAR)pTxWI, TYPE_TXWI);
#endif /* RT_BIG_ENDIAN */
		RTMPMoveMemory(&pWirelessPkt[TXWISize + TXINFO_SIZE + TSO_SIZE], pNullFrame, frameLen);
#ifdef RT_BIG_ENDIAN
		RTMPFrameEndianChange(pAd, (PUCHAR)&pWirelessPkt[TXINFO_SIZE + TXWISize + TSO_SIZE], DIR_WRITE, FALSE);
#endif /* RT_BIG_ENDIAN */
		pAd->NullContext.BulkOutSize =  TXINFO_SIZE + TXWISize + TSO_SIZE + frameLen + 4;

		/* Fill out frame length information for global Bulk out arbitor*/
		/*pNullContext->BulkOutSize = TransferBufferLength;*/
		DBGPRINT(RT_DEBUG_ERROR, ("%s - Send NULL Frame @%d Mbps...\n", __FUNCTION__, RateIdToMbps[pAd->CommonCfg.TxRate]));
		RTUSB_SET_BULK_FLAG(pAd, fRTUSB_BULK_OUT_DATA_NULL);

		pAd->Sequence = (pAd->Sequence+1) & MAXSEQ;
		
		/* Kick bulk out */
		RTUSBKickBulkOut(pAd);
	}
	else
	{
		/*YF@20130204: For Debug the Null Frame Kickout success or fail */
		DBGPRINT(RT_DEBUG_ERROR, ("%s - Send NULL Frame Fail Due to In Null Context using...\n", __FUNCTION__));
	}
}


/*
========================================================================
Routine Description:
    Get a received packet.

Arguments:
	pAd					device control block
	pSaveRxD			receive descriptor information
	*pbReschedule		need reschedule flag
	*pRxPending			pending received packet flag

Return Value:
    the recieved packet

Note:
========================================================================
*/
PNDIS_PACKET GetPacketFromRxRing(
	IN RTMP_ADAPTER *pAd,
	OUT RX_BLK *pRxBlk,
	OUT BOOLEAN *pbReschedule,
	INOUT UINT32 *pRxPending)
{
	RX_CONTEXT *pRxContext;
	PNDIS_PACKET pNetPkt;
	UCHAR *pData;
	ULONG ThisFrameLen, RxBufferLength, valid_len;
	RXWI_STRUC *pRxWI;
	UINT8 RXWISize = pAd->chipCap.RXWISize;
	RXINFO_STRUC *pRxInfo;
#ifdef RLT_MAC
	RXFCE_INFO *pRxFceInfo;
#endif /* RLT_MAC */


	pRxContext = &pAd->RxContext[pAd->NextRxBulkInReadIndex];
	if ((pRxContext->Readable == FALSE) || (pRxContext->InUse == TRUE))
		return NULL;

	RxBufferLength = pRxContext->BulkInOffset - pAd->ReadPosition;
	valid_len = RXDMA_FIELD_SIZE + RXWISize + sizeof(RXINFO_STRUC);
#ifdef RLT_MAC
	valid_len += sizeof(RXFCE_INFO);
#endif /* RLT_MAC */
	if (RxBufferLength < valid_len)
	{
		goto label_null;
	}
	
	pData = &pRxContext->TransferBuffer[pAd->ReadPosition];
//+++Add by shiang for debug
if (0) {
	hex_dump("GetPacketFromRxRing", pData, (RxBufferLength > 7000 ? 7000 : RxBufferLength));
}
//---Add by shiang for debug

	/* The RXDMA field is 4 bytes, now just use the first 2 bytes. The Length including the (RXWI + MSDU + Padding) */
	ThisFrameLen = *pData + (*(pData+1)<<8);
	if (ThisFrameLen == 0)
	{	    
		DBGPRINT(RT_DEBUG_TRACE, ("BIRIdx(%d): RXDMALen is zero.[%ld], BulkInBufLen = %ld)\n", 
								pAd->NextRxBulkInReadIndex, ThisFrameLen, pRxContext->BulkInOffset));     
		goto label_null;
	}   
	if ((ThisFrameLen & 0x3) != 0)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("BIRIdx(%d): RXDMALen not multiple of 4.[%ld], BulkInBufLen = %ld)\n", 
								pAd->NextRxBulkInReadIndex, ThisFrameLen, pRxContext->BulkInOffset));
		goto label_null;
	}

	if ((ThisFrameLen + 8) > RxBufferLength)	/* 8 for (RXDMA_FIELD_SIZE + sizeof(RXINFO_STRUC))*/
	{
		DBGPRINT(RT_DEBUG_ERROR,("BIRIdx(%d):FrameLen(0x%lx) outranges. BulkInLen=0x%lx, remaining RxBufLen=0x%lx, ReadPos=0x%lx\n", 
						pAd->NextRxBulkInReadIndex, ThisFrameLen, pRxContext->BulkInOffset, RxBufferLength, pAd->ReadPosition));

		/* error frame. finish this loop*/
		goto label_null;
	}

	/* skip USB frame length field*/
	pData += RXDMA_FIELD_SIZE;
#ifdef RLT_MAC
	do {
		struct _RXWI_OMAC *rxwi_n;

		pRxInfo = (RXINFO_STRUC *)pData;
		pRxFceInfo = (RXFCE_INFO *)(pData + ThisFrameLen);

		pData += RXINFO_SIZE;

		rxwi_n = (struct _RXWI_OMAC *)pData;
		pRxBlk->pRxWI = (RXWI_STRUC *) pData;
		pRxBlk->wcid = rxwi_n->wcid;
		pRxBlk->DataSize = rxwi_n->MPDUtotalByteCnt;
		pRxBlk->key_idx = rxwi_n->key_idx;
		pRxBlk->bss_idx = rxwi_n->bss_idx;
		pRxBlk->ccmp_pn = 0;
		pRxBlk->ccmp_pn_valid = FALSE;
	} while (0);
#endif /* RLT_MAC */
#ifdef RTMP_MAC
	pRxInfo = (RXINFO_STRUC *)(pData + ThisFrameLen);
#endif /* RTMP_MAC */

	pRxWI = (RXWI_STRUC *)pData;


#ifdef RT_BIG_ENDIAN
	RTMPWIEndianChange(pAd, pData, TYPE_RXWI);
#endif /* RT_BIG_ENDIAN */
	if (pRxWI->RxWIMPDUByteCnt > ThisFrameLen)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s():pRxWIMPDUtotalByteCount(%d) large than RxDMALen(%ld)\n", 
									__FUNCTION__, pRxWI->RxWIMPDUByteCnt, ThisFrameLen));
		goto label_null;
	}
#ifdef RT_BIG_ENDIAN
	RTMPWIEndianChange(pAd, pData, TYPE_RXWI);
#endif /* RT_BIG_ENDIAN */

	/* allocate a rx packet*/
	pNetPkt = RTMP_AllocateFragPacketBuffer(pAd, ThisFrameLen);
	if (pNetPkt == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR,("%s():Cannot Allocate sk buffer for this Bulk-In buffer!\n", __FUNCTION__));
		goto label_null;
	}

	/* copy the rx packet*/
	RTMP_USB_PKT_COPY(get_netdev_from_bssid(pAd, BSS0), pNetPkt, ThisFrameLen, pData);

#ifdef RT_BIG_ENDIAN
	RTMPDescriptorEndianChange((PUCHAR)pRxInfo, TYPE_RXINFO);
#endif /* RT_BIG_ENDIAN */	

#ifdef RLT_MAC
	 {
		NdisMoveMemory((VOID *)&pRxBlk->hw_rx_info[0], (VOID *)pRxFceInfo, sizeof(RXFCE_INFO));
		pRxBlk->pRxFceInfo = (RXFCE_INFO *)&pRxBlk->hw_rx_info[0];
	}
#endif /* RLT_MAC */

	NdisMoveMemory(&pRxBlk->hw_rx_info[RXINFO_OFFSET], pRxInfo, RXINFO_SIZE);
	pRxBlk->pRxInfo = (RXINFO_STRUC *)&pRxBlk->hw_rx_info[RXINFO_OFFSET];


	/* update next packet read position.*/
	pAd->ReadPosition += (ThisFrameLen + RXDMA_FIELD_SIZE + RXINFO_SIZE);	/* 8 for (RXDMA_FIELD_SIZE + sizeof(RXINFO_STRUC))*/

	return pNetPkt;

label_null:
	
	return NULL;
}


#ifdef CONFIG_STA_SUPPORT
/*
	========================================================================

	Routine	Description:
		Check Rx descriptor, return NDIS_STATUS_FAILURE if any error dound
		
	Arguments:
		pRxD		Pointer	to the Rx descriptor
		
	Return Value:
		NDIS_STATUS_SUCCESS		No err
		NDIS_STATUS_FAILURE		Error
		
	Note:
	
	========================================================================
*/
NDIS_STATUS	RTMPCheckRxError(
	IN RTMP_ADAPTER *pAd,
	IN PHEADER_802_11 pHeader,	
	IN RX_BLK * pRxBlk,
	IN RXWI_STRUC *pRxWI,	
	IN RXINFO_STRUC *pRxInfo)
{	
	PCIPHER_KEY pWpaKey;
	INT dBm;
	
	if(pRxInfo == NULL)
		return(NDIS_STATUS_FAILURE);

	/* Phy errors & CRC errors*/
	if (pRxInfo->Crc)
	{
		/* Check RSSI for Noise Hist statistic collection.*/
		dBm = (INT) (pRxWI->RxWIRSSI0) - pAd->BbpRssiToDbmDelta;
		if (dBm <= -87)
			pAd->StaCfg.RPIDensity[0] += 1;
		else if (dBm <= -82)
			pAd->StaCfg.RPIDensity[1] += 1;
		else if (dBm <= -77)
			pAd->StaCfg.RPIDensity[2] += 1;
		else if (dBm <= -72)
			pAd->StaCfg.RPIDensity[3] += 1;
		else if (dBm <= -67)
			pAd->StaCfg.RPIDensity[4] += 1;
		else if (dBm <= -62)
			pAd->StaCfg.RPIDensity[5] += 1;
		else if (dBm <= -57)
			pAd->StaCfg.RPIDensity[6] += 1;
		else if (dBm > -57)
			pAd->StaCfg.RPIDensity[7] += 1;
		
		return(NDIS_STATUS_FAILURE);
	}
	
	/* Add Rx size to channel load counter, we should ignore error counts*/
	pAd->StaCfg.CLBusyBytes += (pRxWI->RxWIMPDUByteCnt + 14);

#ifndef CLIENT_WDS
	if (pHeader->FC.ToDs
		)
	{
		DBGPRINT_RAW(RT_DEBUG_TRACE, ("Err;FC.ToDs\n"));
		return NDIS_STATUS_FAILURE;
	}
#endif /* CLIENT_WDS */

	/* Paul 04-03 for OFDM Rx length issue*/
	if (pRxWI->RxWIMPDUByteCnt > MAX_AGGREGATION_SIZE)
	{
		DBGPRINT_RAW(RT_DEBUG_ERROR, ("received packet too long\n"));
		return NDIS_STATUS_FAILURE;
	}

	/* Drop not U2M frames, cant's drop here because we will drop beacon in this case*/
	/* I am kind of doubting the U2M bit operation*/
	/* if (pRxD->U2M == 0)*/
	/*	return(NDIS_STATUS_FAILURE);*/

	/* drop decyption fail frame*/
	if (pRxInfo->Decrypted && pRxInfo->CipherErr)
	{
						
		if (((pRxInfo->CipherErr & 1) == 1) && INFRA_ON(pAd))
            RTMPSendWirelessEvent(pAd, IW_ICV_ERROR_EVENT_FLAG, pAd->MacTab.Content[BSSID_WCID].Addr, BSS0, 0);			

		if (((pRxInfo->CipherErr & 2) == 2) && INFRA_ON(pAd))
                RTMPSendWirelessEvent(pAd, IW_MIC_ERROR_EVENT_FLAG, pAd->MacTab.Content[BSSID_WCID].Addr, BSS0, 0);

		
		/* MIC Error*/
		if ((pRxInfo->CipherErr == 2) && pRxInfo->MyBss)
		{			
			pWpaKey = &pAd->SharedKey[BSS0][pRxWI->RxWIKeyIndex];
#ifdef WPA_SUPPLICANT_SUPPORT                                    
			if (pAd->StaCfg.WpaSupplicantUP)
				WpaSendMicFailureToWpaSupplicant(pAd->net_dev, pHeader->Addr2,
						(pWpaKey->Type == PAIRWISEKEY) ? TRUE:FALSE,
						(INT) pRxBlk->key_idx, NULL);
            else
#endif /* WPA_SUPPLICANT_SUPPORT */
			RTMPReportMicError(pAd, pWpaKey);
			DBGPRINT_RAW(RT_DEBUG_ERROR,("Rx MIC Value error\n"));
		}
		
		if (pRxInfo->Decrypted &&
			(pAd->SharedKey[BSS0][pRxWI->RxWIKeyIndex].CipherAlg == CIPHER_AES) &&
			(pHeader->Sequence == pAd->FragFrame.Sequence))
		{
			
			/* Acceptable since the First FragFrame no CipherErr problem.*/
			return(NDIS_STATUS_SUCCESS);
		}

		return(NDIS_STATUS_FAILURE);
	}

	return(NDIS_STATUS_SUCCESS);
}


VOID RtmpUsbStaAsicForceWakeupTimeout(
	IN PVOID SystemSpecific1, 
	IN PVOID FunctionContext, 
	IN PVOID SystemSpecific2, 
	IN PVOID SystemSpecific3)
{
	RTMP_ADAPTER *pAd = (RTMP_ADAPTER *)FunctionContext;

	

	if (pAd && pAd->Mlme.AutoWakeupTimerRunning)
	{
		
#ifdef MT7601
		if ( IS_MT7601(pAd) )
		{
			if ( !OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_DOZE) )
			{
				RTMPSetTimer(&pAd->Mlme.AutoWakeupTimer, AUTO_WAKEUP_TIMEOUT);
				return;
			}
				
			ASIC_RADIO_ON(pAd, MLME_RADIO_ON);
		}
		else
#endif /* MT7601 */
		{
			RTUSBBulkReceive(pAd);

			AsicSendCommandToMcu(pAd, 0x31, 0xff, 0x00, 0x02, FALSE);
		}


		OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_DOZE);
		pAd->Mlme.AutoWakeupTimerRunning = FALSE;
	}
}


VOID RT28xxUsbStaAsicForceWakeup(
	IN PRTMP_ADAPTER pAd,
	IN BOOLEAN       bFromTx)
{
	BOOLEAN	Canceled;

	if (pAd->Mlme.AutoWakeupTimerRunning)
	{
		if ( !OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_DOZE) )
		{
			return;	
		}
		RTMPCancelTimer(&pAd->Mlme.AutoWakeupTimer, &Canceled);
		pAd->Mlme.AutoWakeupTimerRunning = FALSE;
	}

#ifdef MT7601
	if ( IS_MT7601(pAd) )
	{
		ASIC_RADIO_ON(pAd, DOT11_RADIO_ON);
	}
	else
#endif /* MT7601 */
	{
	AsicSendCommandToMcu(pAd, 0x31, 0xff, 0x00, 0x02, FALSE);
	}

	OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_DOZE);
}


VOID RT28xxUsbStaAsicSleepThenAutoWakeup(
	IN PRTMP_ADAPTER pAd, 
	IN USHORT TbttNumToNextWakeUp) 
{


	/* Not going to sleep if in the Count Down Time*/
	if (pAd->CountDowntoPsm > 0)
		return;

	if (pAd->Mlme.AutoWakeupTimerRunning == TRUE)
		return;

	/* we have decided to SLEEP, so at least do it for a BEACON period.*/
	if (TbttNumToNextWakeUp == 0)
		TbttNumToNextWakeUp = 1;


	RTMPSetTimer(&pAd->Mlme.AutoWakeupTimer, AUTO_WAKEUP_TIMEOUT);
	pAd->Mlme.AutoWakeupTimerRunning = TRUE;

#ifdef MT7601
	if ( IS_MT7601(pAd) )
	{
		ASIC_RADIO_OFF(pAd, DOT11_RADIO_OFF);
	}
	else
#endif /* MT7601 */
	{
		AsicSendCommandToMcu(pAd, 0x30, 0xff, 0xff, 0x02, FALSE);   /* send POWER-SAVE command to MCU. Timeout 40us.*/
		/* cancel bulk-in IRPs prevent blocking CPU enter C3.*/
		if((pAd->PendingRx > 0) && (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)))
		{
			RTUSBCancelPendingBulkInIRP(pAd);
			/* resend bulk-in IRPs to receive beacons after a period of (pAd->CommonCfg.BeaconPeriod - 40) ms*/
			pAd->PendingRx = 0;
		}
	}

	OPSTATUS_SET_FLAG(pAd, fOP_STATUS_DOZE);

}
#endif /* CONFIG_STA_SUPPORT */

#endif /* RTMP_MAC_USB */

