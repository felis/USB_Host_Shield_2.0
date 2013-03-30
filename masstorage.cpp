#include "masstorage.h"

const uint8_t BulkOnly::epDataInIndex = 1;
const uint8_t BulkOnly::epDataOutIndex = 2;
const uint8_t BulkOnly::epInterruptInIndex = 3;

bool BulkOnly::IsValidCSW(CommandStatusWrapper *pcsw, CommandBlockWrapperBase *pcbw) {
        if (pcsw->dCSWSignature != MASS_CSW_SIGNATURE) {
                Notify(PSTR("CSW:Sig error\r\n"), 0x80);
                return false;
        }
        if (pcsw->dCSWTag != pcbw->dCBWTag) {
                Notify(PSTR("CSW:Wrong tag\r\n"), 0x80);
                return false;
        }
        return true;
}

BulkOnly::BulkOnly(USB *p) :
pUsb(p),
bAddress(0),
bIface(0),
bNumEP(1),
qNextPollTime(0),
bPollEnable(false),
dCBWTag(0),
bLastUsbError(0) {
        for (uint8_t i = 0; i < MASS_MAX_ENDPOINTS; i++) {
                epInfo[i].epAddr = 0;
                epInfo[i].maxPktSize = (i) ? 0 : 8;
                epInfo[i].epAttribs = 0;

                if (!i)
                        epInfo[i].bmNakPower = USB_NAK_MAX_POWER;
        }
        if (pUsb)
                pUsb->RegisterDeviceClass(this);
}

uint8_t BulkOnly::Init(uint8_t parent, uint8_t port, bool lowspeed) {

        const uint8_t constBufSize = sizeof (USB_DEVICE_DESCRIPTOR);

        uint8_t buf[constBufSize];
        uint8_t rcode;
        UsbDevice *p = NULL;
        EpInfo *oldep_ptr = NULL;
        uint8_t num_of_conf; // number of configurations

        for (uint8_t i = 0; i < MASS_MAX_ENDPOINTS; i++) {
                epInfo[i].epAddr = 0;
                epInfo[i].maxPktSize = (i) ? 0 : 8;
                epInfo[i].epAttribs = 0;

                if (!i)
                        epInfo[i].bmNakPower = USB_NAK_MAX_POWER;
        }

        AddressPool &addrPool = pUsb->GetAddressPool();


        if (bAddress)
                return USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE;

        USBTRACE("MS Init\r\n");
        // Get pointer to pseudo device with address 0 assigned
        p = addrPool.GetUsbDevicePtr(0);

        if (!p) {
                return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
        }

        if (!p->epinfo) {
                USBTRACE("epinfo\r\n");
                return USB_ERROR_EPINFO_IS_NULL;
        }

        // Save old pointer to EP_RECORD of address 0
        oldep_ptr = p->epinfo;

        // Temporary assign new pointer to epInfo to p->epinfo in order to avoid toggle inconsistence
        p->epinfo = epInfo;

        p->lowspeed = lowspeed;
        // Get device descriptor
        rcode = pUsb->getDevDescr(0, 0, constBufSize, (uint8_t*)buf);

        // Restore p->epinfo
        p->epinfo = oldep_ptr;

        if (rcode) {
                goto FailGetDevDescr;
        }
        // Allocate new address according to device class
        bAddress = addrPool.AllocAddress(parent, false, port);

        if (!bAddress)
                return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;

        // Extract Max Packet Size from the device descriptor
        epInfo[0].maxPktSize = (uint8_t)((USB_DEVICE_DESCRIPTOR*)buf)->bMaxPacketSize0;

        // Assign new address to the device
        rcode = pUsb->setAddr(0, 0, bAddress);

        if (rcode) {
                p->lowspeed = false;
                addrPool.FreeAddress(bAddress);
                bAddress = 0;
                USBTRACE2("setAddr:", rcode);
                return rcode;
        }

        USBTRACE2("Addr:", bAddress);

        p->lowspeed = false;

        p = addrPool.GetUsbDevicePtr(bAddress);

        if (!p)
                return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

        p->lowspeed = lowspeed;

        num_of_conf = ((USB_DEVICE_DESCRIPTOR*)buf)->bNumConfigurations;

        // Assign epInfo to epinfo pointer
        rcode = pUsb->setEpInfoEntry(bAddress, 1, epInfo);

        if (rcode)
                goto FailSetDevTblEntry;

        USBTRACE2("NC:", num_of_conf);

        for (uint8_t i = 0; i < num_of_conf; i++) {
                ConfigDescParser< USB_CLASS_MASS_STORAGE,
                        MASS_SUBCLASS_SCSI,
                        MASS_PROTO_BBB,
                        CP_MASK_COMPARE_CLASS |
                        CP_MASK_COMPARE_SUBCLASS |
                        CP_MASK_COMPARE_PROTOCOL > BulkOnlyParser(this);

                rcode = pUsb->getConfDescr(bAddress, 0, i, &BulkOnlyParser);

                if (rcode)
                        goto FailGetConfDescr;

                if (bNumEP > 1)
                        break;
        } // for

        if (bNumEP < 3)
                return USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;

        // Assign epInfo to epinfo pointer
        rcode = pUsb->setEpInfoEntry(bAddress, bNumEP, epInfo);

        USBTRACE2("Conf:", bConfNum);

        // Set Configuration Value
        rcode = pUsb->setConf(bAddress, 0, bConfNum);

        if (rcode)
                goto FailSetConfDescr;

        delay(10000);

        rcode = GetMaxLUN(&bMaxLUN);
        if (rcode)
                goto FailGetMaxLUN;

        ErrorMessage<uint8_t > (PSTR("MaxLUN"), bMaxLUN);

        delay(10);

        bTheLUN = bMaxLUN;

        //if (bMaxLUN > 0)
        {
                for (uint8_t lun = 0; lun <= bMaxLUN; lun++) {
                        ErrorMessage<uint8_t > (PSTR("\r\nLUN"), lun);
                        Notify(PSTR("--------\r\n"), 0x80);

                        uint8_t count = 0;

                        MediaCTL(lun, 0x01);
                        while (rcode = TestUnitReady(lun)) {
                                if (rcode == MASS_ERR_NO_MEDIA)
                                        break;

                                if (rcode == MASS_ERR_DEVICE_DISCONNECTED)
                                        goto Fail;

                                if (!count)
                                        Notify(PSTR("Not ready...\r\n"), 0x80);

                                if (count == 0xff)
                                        break;

                                delay(100);
                                count++;
                        }
                        if (count == 0xff)
                                continue;

                        rcode = 0;
                        InquiryResponse response;
                        rcode = Inquiry(lun, sizeof (InquiryResponse), (uint8_t*) & response);

                        if (rcode)
                                ErrorMessage<uint8_t > (PSTR("Inquiry"), rcode);

                        rcode = 0;
                        Capacity capacity;
                        rcode = ReadCapacity(lun, sizeof (Capacity), (uint8_t*) & capacity);

                        if (rcode)
                                ErrorMessage<uint8_t > (PSTR("ReadCapacity"), rcode);
                        else {
                                for (uint8_t i = 0; i<sizeof (Capacity); i++)
                                        PrintHex<uint8_t > (capacity.data[i], 0x80);
                                Notify(PSTR("\r\n\r\n"), 0x80);
                                // Only 512/1024/2048/4096 are valid values!
                                uint32_t c = ((uint32_t)capacity.data[4] << 24) + ((uint32_t)capacity.data[5] << 16) + ((uint32_t)capacity.data[6] << 8) + (uint32_t)capacity.data[7];
                                if (c != 0x0200LU && c != 0x0400LU && c != 0x0800LU && c != 0x1000LU) {
                                        rcode = 255;
                                        goto FailInvalidSectorSize;
                                }
                        }

                        rcode = 0;
#if 0
                        {
                                uint8_t buf[512];
                                rcode = Read(lun, 0, 512, 1, buf);

                                if (rcode)
                                        ErrorMessage<uint8_t > (PSTR("Read"), rcode);
                                else {
                                        Notify(PSTR("Read: OK\r\n\r\n"), 0x80);
                                        /*
                                        for(int i=0; i<512; i++) {
                                                PrintHex<uint8_t>(buf[i], 0x80);
                                                Notify(PSTR(" "), 0x80);
                                        }
                                        Notify(PSTR("\r\n\r\n"), 0x80);
                                         */
                                }
                        }
                        {
                                uint8_t buf[192];
                                rcode = ModeSense(lun, 0, 0x3f, 0, 192, buf);

                                if (rcode)
                                        ErrorMessage<uint8_t > (PSTR("ModeSense"), rcode);
                                else
                                        Notify(PSTR("ModeSense: OK\r\n\r\n"), 0x80);
                        }
#endif
                }
                Notify(PSTR("==========\r\n"), 0x80);
        }

        if (TestUnitReady(bTheLUN)) {
                Notify(PSTR("Unit not ready\r\n"), 0x80);

                rcode = MASS_ERR_UNIT_NOT_READY;
                //goto FailOnInit;
        }

        rcode = OnInit();

        if (rcode)
                goto FailOnInit;

        USBTRACE("MS configured\r\n\r\n");

        bPollEnable = true;

        //USBTRACE("Poll enabled\r\n");
        return 0;

FailGetDevDescr:
        NotifyFailGetDevDescr();
        goto Fail;

FailSetDevTblEntry:
        NotifyFailSetDevTblEntry();
        goto Fail;

FailGetConfDescr:
        NotifyFailGetConfDescr();
        goto Fail;

FailSetConfDescr:
        NotifyFailSetConfDescr();
        goto Fail;

FailOnInit:
        USBTRACE("OnInit:");
        goto Fail;

FailGetMaxLUN:
        USBTRACE("GetMaxLUN:");
        goto Fail;

FailInquiry:
        USBTRACE("Inquiry:");
        goto Fail;

FailReadCapacity:
        USBTRACE("ReadCapacity:");
        goto Fail;

FailInvalidSectorSize:
        USBTRACE("Sector Size is NOT VALID: ");
        goto Fail;

FailRead0:
        USBTRACE("Read0:");
        goto Fail;

FailModeSense0:
        USBTRACE("ModeSense0:");
        goto Fail;

FailModeSense1:
        USBTRACE("ModeSense1:");

Fail:
        NotifyFail(rcode);
        Release();
        return rcode;
}

void BulkOnly::EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR *pep) {
        ErrorMessage<uint8_t > (PSTR("Conf.Val"), conf);
        ErrorMessage<uint8_t > (PSTR("Iface Num"), iface);
        ErrorMessage<uint8_t > (PSTR("Alt.Set"), alt);

        bConfNum = conf;

        uint8_t index;

        if ((pep->bmAttributes & 0x03) == 3 && (pep->bEndpointAddress & 0x80) == 0x80)
                index = epInterruptInIndex;
        else
                if ((pep->bmAttributes & 0x02) == 2)
                index = ((pep->bEndpointAddress & 0x80) == 0x80) ? epDataInIndex : epDataOutIndex;
        else
                return;

        // Fill in the endpoint info structure
        epInfo[index].epAddr = (pep->bEndpointAddress & 0x0F);
        epInfo[index].maxPktSize = (uint8_t)pep->wMaxPacketSize;
        epInfo[index].epAttribs = 0;

        bNumEP++;

        PrintEndpointDescriptor(pep);
}

uint8_t BulkOnly::Release() {
        pUsb->GetAddressPool().FreeAddress(bAddress);

        bIface = 0;
        bNumEP = 1;

        bAddress = 0;
        qNextPollTime = 0;
        bPollEnable = false;
        bLastUsbError = 0;
        bMaxLUN = 0;
        bTheLUN = 0;
        dCBWTag = 0;
        return 0;
}

uint8_t BulkOnly::Poll() {
        uint8_t rcode = 0;

        if (!bPollEnable)
                return 0;

        return rcode;
}

uint8_t BulkOnly::Reset() {
        return ( pUsb->ctrlReq(bAddress, 0, bmREQ_MASSOUT, MASS_REQ_BOMSR, 0, 0, bIface, 0, 0, NULL, NULL));
}

uint8_t BulkOnly::GetMaxLUN(uint8_t *plun) {
        uint8_t ret = pUsb->ctrlReq(bAddress, 0, bmREQ_MASSIN, MASS_REQ_GET_MAX_LUN, 0, 0, bIface, 1, 1, plun, NULL);

        if (ret == hrSTALL)
                *plun = 0;

        return 0;
}

uint8_t BulkOnly::HandleUsbError(uint8_t error, uint8_t index) {
        uint8_t count = 3;

        bLastUsbError = error;

        while (error && count) {
                if (error != hrSUCCESS) {
                        ErrorMessage<uint8_t > (PSTR("USB Error"), error);
                        ErrorMessage<uint8_t > (PSTR("Index"), index);
                }
                switch (error) {
                        case hrSUCCESS: return MASS_ERR_SUCCESS;
                        case hrBUSY: return MASS_ERR_UNIT_BUSY;
                        case hrTIMEOUT:
                        case hrJERR: return MASS_ERR_DEVICE_DISCONNECTED;
                        case hrSTALL:
                                if (index == 0)
                                        return MASS_ERR_SUCCESS;

                                error = ClearEpHalt(index);
                                //return MASS_ERR_STALL;
                                return MASS_ERR_SUCCESS;

                                //error = ClearEpHalt(index);
                                //break;
                        case hrTOGERR:
                                if (bAddress && bConfNum) {
                                        error = pUsb->setConf(bAddress, 0, bConfNum);

                                        if (error)
                                                break;
                                }
                                return MASS_ERR_SUCCESS;
                        default:
                                ErrorMessage<uint8_t > (PSTR("\r\nUSB"), error);
                                return MASS_ERR_GENERAL_USB_ERROR;
                }
                count--;
        } // while

        return ((error && !count) ? MASS_ERR_GENERAL_USB_ERROR : MASS_ERR_SUCCESS);
}

uint8_t BulkOnly::ClearEpHalt(uint8_t index) {
        if (index == 0)
                return 0;

        uint8_t ret = 0;

        ret = (pUsb->ctrlReq(bAddress, 0, USB_SETUP_HOST_TO_DEVICE | USB_SETUP_TYPE_STANDARD | USB_SETUP_RECIPIENT_ENDPOINT,
                USB_REQUEST_CLEAR_FEATURE, USB_FEATURE_ENDPOINT_HALT, 0, ((index == epDataInIndex) ? (0x80 | epInfo[index].epAddr) : epInfo[index].epAddr), 0, 0, NULL, NULL));

        ////ret = (pUsb->ctrlReq(bAddress, 0, USB_SETUP_HOST_TO_DEVICE|USB_SETUP_RECIPIENT_ENDPOINT|USB_SETUP_TYPE_STANDARD,
        ////USB_REQUEST_CLEAR_FEATURE, USB_FEATURE_ENDPOINT_HALT, 0, epInfo[index].epAddr, 0, 0, NULL, NULL));
        ////
        if (ret) {
                ErrorMessage<uint8_t > (PSTR("ClearEpHalt"), ret);
                ErrorMessage<uint8_t > (PSTR("EP"), ((index == epDataInIndex) ? (0x80 | epInfo[index].epAddr) : epInfo[index].epAddr));
                return ret;
        }
        epInfo[index].epAttribs = 0;
        return 0;
}

uint8_t BulkOnly::ResetRecovery() {
        Notify(PSTR("\r\nResetRecovery\r\n"), 0x80);
        Notify(PSTR("-----------------\r\n"), 0x80);

        bLastUsbError = Reset();

        if (bLastUsbError) {
                return bLastUsbError;
        }
        delay(6);

        bLastUsbError = ClearEpHalt(epDataInIndex);

        if (bLastUsbError) {
                return bLastUsbError;
        }
        delay(6);

        bLastUsbError = ClearEpHalt(epDataOutIndex);

        delay(6);

        return bLastUsbError;
}

uint8_t BulkOnly::Inquiry(uint8_t lun, uint16_t bsize, uint8_t *buf) {
        Notify(PSTR("\r\nInquiry\r\n"), 0x80);
        Notify(PSTR("---------\r\n"), 0x80);

        CommandBlockWrapper cbw;

        cbw.dCBWSignature = MASS_CBW_SIGNATURE;
        cbw.dCBWTag = ++dCBWTag;
        cbw.dCBWDataTransferLength = bsize;
        cbw.bmCBWFlags = MASS_CMD_DIR_IN,
                cbw.bmCBWLUN = lun;
        cbw.bmCBWCBLength = 6;

        for (uint8_t i = 0; i < 16; i++)
                cbw.CBWCB[i] = 0;

        cbw.CBWCB[0] = SCSI_CMD_INQUIRY;
        cbw.CBWCB[4] = bsize;

        return HandleSCSIError(Transaction(&cbw, bsize, buf, 0));
}

uint8_t BulkOnly::RequestSense(uint8_t lun, uint16_t size, uint8_t *buf) {
        Notify(PSTR("\r\nRequestSense\r\n"), 0x80);
        Notify(PSTR("----------------\r\n"), 0x80);

        CommandBlockWrapper cbw;

        cbw.dCBWSignature = MASS_CBW_SIGNATURE;
        cbw.dCBWTag = ++dCBWTag;
        cbw.dCBWDataTransferLength = size;
        cbw.bmCBWFlags = MASS_CMD_DIR_IN,
                cbw.bmCBWLUN = lun;
        cbw.bmCBWCBLength = 6;

        for (uint8_t i = 0; i < 16; i++)
                cbw.CBWCB[i] = 0;

        cbw.CBWCB[0] = SCSI_CMD_REQUEST_SENSE;
        cbw.CBWCB[4] = size;

        return HandleSCSIError(Transaction(&cbw, size, buf, 0));
}

uint8_t BulkOnly::ReadCapacity(uint8_t lun, uint16_t bsize, uint8_t *buf) {
        Notify(PSTR("\r\nReadCapacity\r\n"), 0x80);
        Notify(PSTR("---------------\r\n"), 0x80);

        CommandBlockWrapper cbw;

        cbw.dCBWSignature = MASS_CBW_SIGNATURE;
        cbw.dCBWTag = ++dCBWTag;
        cbw.dCBWDataTransferLength = bsize;
        cbw.bmCBWFlags = MASS_CMD_DIR_IN,
                cbw.bmCBWLUN = lun;
        cbw.bmCBWCBLength = 10;

        for (uint8_t i = 0; i < 16; i++)
                cbw.CBWCB[i] = 0;

        cbw.CBWCB[0] = SCSI_CMD_READ_CAPACITY_10;
        //cbw.CBWCB[4] = bsize;

        return HandleSCSIError(Transaction(&cbw, bsize, buf, 0));
}

uint8_t BulkOnly::TestUnitReady(uint8_t lun) {
        if (!bAddress) // || !bPollEnable)
                return MASS_ERR_UNIT_NOT_READY;

        Notify(PSTR("\r\nTestUnitReady\r\n"), 0x80);
        Notify(PSTR("-----------------\r\n"), 0x80);

        CommandBlockWrapper cbw;

        cbw.dCBWSignature = MASS_CBW_SIGNATURE;
        cbw.dCBWTag = ++dCBWTag;
        cbw.dCBWDataTransferLength = 0;
        cbw.bmCBWFlags = MASS_CMD_DIR_OUT,
                cbw.bmCBWLUN = lun;
        cbw.bmCBWCBLength = 6;

        for (uint8_t i = 0; i < 16; i++)
                cbw.CBWCB[i] = 0;

        cbw.CBWCB[0] = SCSI_CMD_TEST_UNIT_READY;

        return HandleSCSIError(Transaction(&cbw, 0, NULL, 0));
}

/* Media control: 0x00 Stop Motor, 0x01 Start Motor, 0x02 Eject Media, 0x03 Load Media */
uint8_t BulkOnly::MediaCTL(uint8_t lun, uint8_t ctl) {
        uint8_t rcode = MASS_ERR_UNIT_NOT_READY;
        if (bAddress) {
                CommandBlockWrapper cbw;

                cbw.dCBWSignature = MASS_CBW_SIGNATURE;
                cbw.dCBWTag = ++dCBWTag;
                cbw.dCBWDataTransferLength = 0;
                cbw.bmCBWFlags = MASS_CMD_DIR_OUT,
                        cbw.bmCBWLUN = lun;
                cbw.bmCBWCBLength = 6;

                for (uint8_t i = 0; i < 16; i++)
                        cbw.CBWCB[i] = 0;

                cbw.CBWCB[0] = SCSI_CMD_START_STOP_UNIT;
                cbw.CBWCB[4] = ctl & 0x03;

                rcode = HandleSCSIError(Transaction(&cbw, 0, NULL, 0));
        }
        return rcode;
}

uint8_t BulkOnly::Read(uint8_t lun, uint32_t addr, uint16_t bsize, uint8_t blocks, uint8_t *buf) {
        Notify(PSTR("\r\nRead\r\n"), 0x80);
        Notify(PSTR("---------\r\n"), 0x80);

        CommandBlockWrapper cbw;

        cbw.dCBWSignature = MASS_CBW_SIGNATURE;
        cbw.dCBWTag = ++dCBWTag;
        cbw.dCBWDataTransferLength = ((uint32_t)bsize * blocks);
        cbw.bmCBWFlags = MASS_CMD_DIR_IN,
                cbw.bmCBWLUN = lun;
        cbw.bmCBWCBLength = 10;

        for (uint8_t i = 0; i < 16; i++)
                cbw.CBWCB[i] = 0;

        cbw.CBWCB[0] = SCSI_CMD_READ_10;
        cbw.CBWCB[8] = blocks;
        cbw.CBWCB[2] = ((addr >> 24) & 0xff);
        cbw.CBWCB[3] = ((addr >> 16) & 0xff);
        cbw.CBWCB[4] = ((addr >> 8) & 0xff);
        cbw.CBWCB[5] = (addr & 0xff);
        return HandleSCSIError(Transaction(&cbw, bsize, buf, 0));
}

/* We won't be needing this... */
uint8_t BulkOnly::Read(uint8_t lun, uint32_t addr, uint16_t bsize, uint8_t blocks, USBReadParser *prs) {
#if 0
        Notify(PSTR("\r\nRead (With parser)\r\n"), 0x80);
        Notify(PSTR("---------\r\n"), 0x80);

        CommandBlockWrapper cbw;

        cbw.dCBWSignature = MASS_CBW_SIGNATURE;
        cbw.dCBWTag = ++dCBWTag;
        cbw.dCBWDataTransferLength = ((uint32_t)bsize * blocks);
        cbw.bmCBWFlags = MASS_CMD_DIR_IN,
                cbw.bmCBWLUN = lun;
        cbw.bmCBWCBLength = 10;

        for (uint8_t i = 0; i < 16; i++)
                cbw.CBWCB[i] = 0;

        cbw.CBWCB[0] = SCSI_CMD_READ_10;
        cbw.CBWCB[8] = blocks;
        cbw.CBWCB[2] = ((addr >> 24) & 0xff);
        cbw.CBWCB[3] = ((addr >> 16) & 0xff);
        cbw.CBWCB[4] = ((addr >> 8) & 0xff);
        cbw.CBWCB[5] = (addr & 0xff);

        return HandleSCSIError(Transaction(&cbw, bsize, prs, 1));
#endif
}

uint8_t BulkOnly::Write(uint8_t lun, uint32_t addr, uint16_t bsize, uint8_t blocks, const uint8_t *buf) {
        Notify(PSTR("\r\nWrite\r\n"), 0x80);
        Notify(PSTR("---------\r\n"), 0x80);

        //MediaCTL(lun, 0x01);
        CommandBlockWrapper cbw;

        cbw.dCBWSignature = MASS_CBW_SIGNATURE;
        cbw.dCBWTag = ++dCBWTag;
        cbw.dCBWDataTransferLength = ((uint32_t)bsize * blocks);
        cbw.bmCBWFlags = MASS_CMD_DIR_OUT,
                cbw.bmCBWLUN = lun;
        cbw.bmCBWCBLength = 10;

        for (uint8_t i = 0; i < 16; i++)
                cbw.CBWCB[i] = 0;

        cbw.CBWCB[0] = SCSI_CMD_WRITE_10;
        cbw.CBWCB[8] = 1;
        cbw.CBWCB[5] = (addr & 0xff);
        cbw.CBWCB[4] = ((addr >> 8) & 0xff);
        cbw.CBWCB[3] = ((addr >> 16) & 0xff);
        cbw.CBWCB[2] = ((addr >> 24) & 0xff);

        return HandleSCSIError(Transaction(&cbw, bsize, (void*)buf, 0));
}

uint8_t BulkOnly::ModeSense(uint8_t lun, uint8_t pc, uint8_t page, uint8_t subpage, uint8_t len, uint8_t *pbuf) {
        Notify(PSTR("\r\rModeSense\r\n"), 0x80);
        Notify(PSTR("------------\r\n"), 0x80);

        CommandBlockWrapper cbw;

        cbw.dCBWSignature = MASS_CBW_SIGNATURE;
        cbw.dCBWTag = ++dCBWTag;
        cbw.dCBWDataTransferLength = len;
        cbw.bmCBWFlags = MASS_CMD_DIR_IN,
                cbw.bmCBWLUN = lun;
        cbw.bmCBWCBLength = 6;

        for (uint8_t i = 0; i < 16; i++)
                cbw.CBWCB[i] = 0;

        cbw.CBWCB[0] = SCSI_CMD_MODE_SENSE_6;
        cbw.CBWCB[2] = ((pc << 6) | page);
        cbw.CBWCB[3] = subpage;
        cbw.CBWCB[4] = len;

        return HandleSCSIError(Transaction(&cbw, 512, pbuf, 0));
}

uint8_t BulkOnly::Transaction(CommandBlockWrapper *pcbw, uint16_t buf_size, void *buf, uint8_t flags) {
        uint16_t read;
        uint8_t ret = 0;

        ErrorMessage<uint8_t > (PSTR("CBW.dCBWTag"), pcbw->dCBWTag);

        ret = HandleUsbError(pUsb->outTransfer(bAddress, epInfo[epDataOutIndex].epAddr, sizeof (CommandBlockWrapper), (uint8_t*)pcbw), epDataOutIndex);

        if (ret) {
                ErrorMessage<uint8_t > (PSTR("CBW"), ret);
                return ret;
        }
        Notify(PSTR("CBW:\t\tOK\r\n"), 0x80);

        ret = 0;

        read = (pcbw->dCBWDataTransferLength > buf_size) ? buf_size : pcbw->dCBWDataTransferLength;

        if (read) {
                if (pcbw->bmCBWFlags & MASS_CMD_DIR_IN) {
                        if ((flags & MASS_TRANS_FLG_CALLBACK) == MASS_TRANS_FLG_CALLBACK) {
                                uint8_t rbuf[read];
                                uint8_t err = 0;
                                ret = pUsb->inTransfer(bAddress, epInfo[epDataInIndex].epAddr, &read, rbuf);
                                if (ret == hrSUCCESS) ((USBReadParser*)buf)->Parse(read, rbuf, 0);
                                if (ret == hrSTALL) err = ClearEpHalt(epDataInIndex);
                                if (ret) {
                                        ErrorMessage<uint8_t > (PSTR("RDR"), err);
                                        return MASS_ERR_GENERAL_USB_ERROR;
                                }
                        } else
                                ret = pUsb->inTransfer(bAddress, epInfo[epDataInIndex].epAddr, &read, (uint8_t*)buf);
                } else
                        ret = pUsb->outTransfer(bAddress, epInfo[epDataOutIndex].epAddr, read, (uint8_t*)buf);

                ret = HandleUsbError(ret, (pcbw->bmCBWFlags & MASS_CMD_DIR_IN) ? epDataInIndex : epDataOutIndex);

                if (ret) {
                        ErrorMessage<uint8_t > (PSTR("DAT"), ret);
                        return MASS_ERR_GENERAL_USB_ERROR;
                }

                Notify(PSTR("Data Stage:\tOK\r\n"), 0x80);
        }

        uint8_t count = 2;

        while (count) {
                CommandStatusWrapper csw;
                read = sizeof (CommandStatusWrapper);

                ret = HandleUsbError(pUsb->inTransfer(bAddress, epInfo[epDataInIndex].epAddr, &read, (uint8_t*) & csw), epDataInIndex);

                if (ret) {
                        ErrorMessage<uint8_t > (PSTR("CSW"), ret);
                        count--;
                        continue; //return ret;
                }
                if (IsValidCSW(&csw, pcbw)) {
                        ErrorMessage<uint8_t > (PSTR("CSW.dCBWTag"), csw.dCSWTag);
                        ErrorMessage<uint8_t > (PSTR("bCSWStatus"), csw.bCSWStatus);
                        ErrorMessage<uint8_t > (PSTR("dCSWDataResidue"), csw.dCSWDataResidue);
                        Notify(PSTR("CSW:\t\tOK\r\n\r\n"), 0x80);
                        return csw.bCSWStatus;
                } else {
                        Notify(PSTR("Invalid CSW\r\n"), 0x80);
                        ResetRecovery();
                        return MASS_ERR_SUCCESS; //MASS_ERR_INVALID_CSW;
                }
                count--;
        }
        if (count)
                ResetRecovery();

        return MASS_ERR_SUCCESS;
}

uint8_t BulkOnly::SetCurLUN(uint8_t lun) {
        if (lun > bMaxLUN)
                return MASS_ERR_INVALID_LUN;

        bTheLUN = lun;
        return MASS_ERR_SUCCESS;
};

uint8_t BulkOnly::HandleSCSIError(uint8_t status) {
        uint8_t ret = 0;

        switch (status) {
                case 0: return MASS_ERR_SUCCESS;
                        //case 4: return MASS_ERR_UNIT_BUSY;
                case 2:
                        ErrorMessage<uint8_t > (PSTR("Phase"), status);
                        ResetRecovery();
                        return MASS_ERR_GENERAL_SCSI_ERROR;
                case 1:
                        ErrorMessage<uint8_t > (PSTR("SCSI Error"), status);
                        RequestSenseResponce rsp;

                        ret = RequestSense(bTheLUN, sizeof (RequestSenseResponce), (uint8_t*) & rsp);

                        if (ret)
                                return MASS_ERR_GENERAL_SCSI_ERROR;

                        ErrorMessage<uint8_t > (PSTR("Response Code"), rsp.bResponseCode);
                        ErrorMessage<uint8_t > (PSTR("Sense Key"), rsp.bmSenseKey);
                        ErrorMessage<uint8_t > (PSTR("Add Sense Code"), rsp.bAdditionalSenseCode);
                        ErrorMessage<uint8_t > (PSTR("Add Sense Qual"), rsp.bAdditionalSenseQualifier);
                        switch (rsp.bmSenseKey) {
                                case 0:
                                        return MASS_ERR_SUCCESS;
                                case SCSI_S_NOT_READY:
                                        switch (rsp.bAdditionalSenseCode) {
                                                case SCSI_ASC_MEDIUM_NOT_PRESENT:
                                                        return MASS_ERR_NO_MEDIA;
                                                default:
                                                        return MASS_ERR_UNIT_NOT_READY;
                                        }
                                case SCSI_S_ILLEGAL_REQUEST:
                                        switch (rsp.bAdditionalSenseCode) {
                                                case SCSI_ASC_LBA_OUT_OF_RANGE:
                                                        return MASS_ERR_BAD_LBA;
                                                default:
                                                        return MASS_ERR_CMD_NOT_SUPPORTED;
                                        }
                                default: return MASS_ERR_GENERAL_SCSI_ERROR;
                        }

                default:
                        Reset();
                        ErrorMessage<uint8_t > (PSTR("Gen SCSI Err"), status);
                        return status; //MASS_ERR_GENERAL_SCSI_ERROR;
        } // switch
}

void BulkOnly::PrintEndpointDescriptor(const USB_ENDPOINT_DESCRIPTOR* ep_ptr) {
        Notify(PSTR("Endpoint descriptor:"), 0x80);
        Notify(PSTR("\r\nLength:\t\t"), 0x80);
        PrintHex<uint8_t > (ep_ptr->bLength, 0x80);
        Notify(PSTR("\r\nType:\t\t"), 0x80);
        PrintHex<uint8_t > (ep_ptr->bDescriptorType, 0x80);
        Notify(PSTR("\r\nAddress:\t"), 0x80);
        PrintHex<uint8_t > (ep_ptr->bEndpointAddress, 0x80);
        Notify(PSTR("\r\nAttributes:\t"), 0x80);
        PrintHex<uint8_t > (ep_ptr->bmAttributes, 0x80);
        Notify(PSTR("\r\nMaxPktSize:\t"), 0x80);
        PrintHex<uint16_t > (ep_ptr->wMaxPacketSize, 0x80);
        Notify(PSTR("\r\nPoll Intrv:\t"), 0x80);
        PrintHex<uint8_t > (ep_ptr->bInterval, 0x80);
        Notify(PSTR("\r\n"), 0x80);
}
