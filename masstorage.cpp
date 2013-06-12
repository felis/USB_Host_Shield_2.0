#include "masstorage.h"

const uint8_t BulkOnly::epDataInIndex = 1;
const uint8_t BulkOnly::epDataOutIndex = 2;
const uint8_t BulkOnly::epInterruptInIndex = 3;

BulkOnly::BulkOnly(USB *p) :
pUsb(p),
bAddress(0),
bIface(0),
bNumEP(1),
qNextPollTime(0),
bPollEnable(false),
dCBWTag(0),
bLastUsbError(0) {
        ClearAllEP();
        dCBWTag = 0;
        if (pUsb)
                pUsb->RegisterDeviceClass(this);
}

void BulkOnly::ClearAllEP() {
        for (uint8_t i = 0; i < MASS_MAX_ENDPOINTS; i++) {
                epInfo[i].epAddr = 0;
                epInfo[i].maxPktSize = (i) ? 0 : 8;
                epInfo[i].epAttribs = 0;

                epInfo[i].bmNakPower = USB_NAK_DEFAULT; //0; //USB_NAK_DEFAULT;
        }
        // clear all LUN data as well
        for (uint8_t i = 0; i < MASS_MAX_SUPPORTED_LUN; i++) LUNOk[i] = false;
        bIface = 0;
        bNumEP = 1;

        bAddress = 0;
        qNextPollTime = 0;
        bPollEnable = false;
        bLastUsbError = 0;
        bMaxLUN = 0;
        bTheLUN = 0;
        //dCBWTag = 100;
        //dCBWTag = 1073741823;
}

boolean BulkOnly::CheckLUN(uint8_t lun) {
        uint8_t rcode;
        Capacity capacity;
        for (uint8_t i = 0; i<sizeof (Capacity); i++) capacity.data[i] = 0;

        rcode = ReadCapacity(lun, sizeof (Capacity), (uint8_t*) & capacity);
        if (rcode) {
                //printf(">>>>>>>>>>>>>>>>ReadCapacity returned %i\r\n", rcode);
                return false;
        }
        ErrorMessage<uint8_t > (PSTR(">>>>>>>>>>>>>>>>CAPACITY OK ON LUN"), lun);
        for (uint8_t i = 0; i<sizeof (Capacity); i++)
                PrintHex<uint8_t > (capacity.data[i], 0x80);
        Notify(PSTR("\r\n\r\n"), 0x80);
        // Only 512/1024/2048/4096 are valid values!
        uint32_t c = ((uint32_t)capacity.data[4] << 24) + ((uint32_t)capacity.data[5] << 16) + ((uint32_t)capacity.data[6] << 8) + (uint32_t)capacity.data[7];
        if (c != 0x0200LU && c != 0x0400LU && c != 0x0800LU && c != 0x1000LU) {
                return false;
        }
        // Store capacity information.
        CurrentSectorSize[lun] = (uint16_t)(c & 0xFFFF);
        CurrentCapacity[lun] = ((uint32_t)capacity.data[0] << 24) + ((uint32_t)capacity.data[1] << 16) + ((uint32_t)capacity.data[2] << 8) + (uint32_t)capacity.data[3];
        if (CurrentCapacity[lun] == 0xffffffffLU || CurrentCapacity[lun] == 0x00LU) {
                // Buggy firmware will report 0xffffffff or 0 for no media
                if (CurrentCapacity[lun])
                        ErrorMessage<uint8_t > (PSTR(">>>>>>>>>>>>>>>>BUGGY FIRMWARE. CAPACITY FAIL ON LUN"), lun);
                return false;
        }
        //rcode = TestUnitReady(lun);
        //if (!rcode)
        delay(20);
        Page3F(lun);
        //if (!Page3F(lun))
        //if (!Page3F(lun))
        if (!TestUnitReady(lun)) return true;
        //LockMedia(lun, 1);
        //MediaCTL(lun, 1);
        //if (!TestUnitReady(lun)) return true;
        return false;
}
// Scan for media change
// @Oleg -- should we scan ALL LUN, or just one at a time?

void BulkOnly::CheckMedia() {
        for (uint8_t lun = 0; lun <= bMaxLUN; lun++) {
                if (TestUnitReady(lun)) {
                        LUNOk[lun] = false;
                        continue;
                }
                if (!LUNOk[lun])
                        LUNOk[lun] = CheckLUN(lun);
        }
#if 0
        printf("}}}}}}}}}}}}}}}}STATUS ");
        for (uint8_t lun = 0; lun <= bMaxLUN; lun++) {
                if (LUNOk[lun])
                        printf("#");
                else printf(".");
        }
        printf("\r\n");
#endif
        qNextPollTime = millis() + 2000;
}

/*
 * USB_ERROR_CONFIG_REQUIRES_ADDITIONAL_RESET == success
 * We need to standardize either the rcode, or change the API to return values
 * so a signal that additional actions are required can be produced.
 * Some of these codes do exist already.
 *
 * TECHNICAL: We could do most of this code elsewhere, with the exception of checking the class instance.
 * Doing so would save some program memory when using multiple drivers.
 */
uint8_t BulkOnly::ConfigureDevice(uint8_t parent, uint8_t port, bool lowspeed) {

        const uint8_t constBufSize = sizeof (USB_DEVICE_DESCRIPTOR);

        uint8_t buf[constBufSize];
        uint8_t rcode;
        UsbDevice *p = NULL;
        EpInfo *oldep_ptr = NULL;
        USBTRACE("MS ConfigureDevice\r\n");
        ClearAllEP();
        delay(2000);
        AddressPool &addrPool = pUsb->GetAddressPool();


        if (bAddress)
                return USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE;

        // <TECHNICAL>
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
        // Steal and abuse from epInfo structure to save on memory.
        epInfo[1].epAddr = ((USB_DEVICE_DESCRIPTOR*)buf)->bNumConfigurations;
        // </TECHNICAL>
        return USB_ERROR_CONFIG_REQUIRES_ADDITIONAL_RESET;

FailGetDevDescr:
#ifdef DEBUG_USB_HOST
        NotifyFailGetDevDescr(rcode);
#endif
        rcode = USB_ERROR_FailGetDevDescr;

Fail:
        Release();
        return rcode;
};

boolean BulkOnly::WriteProtected(uint8_t lun) {
        return WriteOk[lun];
}

// Check for write protect.

uint8_t BulkOnly::Page3F(uint8_t lun) {
        uint8_t buf[192];
        for (int i = 0; i < 192; i++) {
                buf[i] = 0x00;
        }
        WriteOk[lun] = true;
        uint8_t rc = ModeSense(lun, 0, 0x3f, 0, 192, buf);
        //if (rc) rc = ModeSense(lun, 0, 0x00, 0, 4, buf);
        //if (rc) rc = ModeSense(lun, 0, 0x3f, 0, 192, buf);
        if (!rc) {
                WriteOk[lun] = ((buf[2] & 0x80) == 0);
                Notify(PSTR("Mode Sense: "), 0x80);
                for (int i = 0; i < 4; i++) {
                        PrintHex<uint8_t > (buf[i], 0x80);
                        Notify(PSTR(" "), 0x80);
                }
#if 0
                if (WriteOk[lun]) {
                        Notify(PSTR(" Writes Allowed"), 0x80);
                } else {
                        Notify(PSTR(" Writes Denied"), 0x80);
                }
#endif
                Notify(PSTR("\r\n"), 0x80);
        }
        return rc;
}
// Bottom half of init. 0 == success.

uint8_t BulkOnly::Init(uint8_t parent, uint8_t port, bool lowspeed) {
        uint8_t rcode;
        uint8_t num_of_conf = epInfo[1].epAddr; // number of configurations
        epInfo[1].epAddr = 0;
        USBTRACE("MS Init\r\n");

        AddressPool &addrPool = pUsb->GetAddressPool();
        UsbDevice *p = addrPool.GetUsbDevicePtr(bAddress);

        if (!p)
                return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

        // Assign new address to the device
        delay(2000);
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
        pUsb->setEpInfoEntry(bAddress, bNumEP, epInfo);

        USBTRACE("MS ConfigureDevice\r\n");
        USBTRACE2("Conf:", bConfNum);

        // Set Configuration Value
        rcode = pUsb->setConf(bAddress, 0, bConfNum);

        if (rcode)
                goto FailSetConfDescr;

        //Linux does a 1sec delay after this.
        delay(1000);

        rcode = GetMaxLUN(&bMaxLUN);
        if (rcode)
                goto FailGetMaxLUN;

        if (bMaxLUN >= MASS_MAX_SUPPORTED_LUN) bMaxLUN = MASS_MAX_SUPPORTED_LUN - 1;
        ErrorMessage<uint8_t > (PSTR("MaxLUN"), bMaxLUN);

        delay(1000); // Delay a bit for slow firmware.

        //bTheLUN = bMaxLUN;

        for (uint8_t lun = 0; lun <= bMaxLUN; lun++) {
                InquiryResponse response;
                rcode = Inquiry(lun, sizeof (InquiryResponse), (uint8_t*) & response);
                if (rcode) {
                        ErrorMessage<uint8_t > (PSTR("Inquiry"), rcode);
                } else {
                        uint8_t tries = 0xf0;
                        while (rcode = TestUnitReady(lun)) {
                                if (rcode == 0x08) break; // break on no media, this is OK to do.
                                // try to lock media and spin up
                                if (tries < 14) {
                                        LockMedia(lun, 1);
                                        MediaCTL(lun, 1); // I actually have a USB stick that needs this!
                                } else delay(2 * (tries + 1));
                                tries++;
                                if (!tries) break;
                        }
                        if (!rcode) {
                                delay(1000);
                                LUNOk[lun] = CheckLUN(lun);
                                if (!LUNOk[lun]) LUNOk[lun] = CheckLUN(lun);
                        }
                }
        }


#if 0
        {
                bool good;
                for (uint8_t i = 1; i == 0; i++) {
                        good = false;
                        CheckMedia();
                        for (uint8_t lun = 0; lun <= bMaxLUN; lun++) good |= LUNOk[lun];
                        if (good) break;
                        delay(118); // 255 loops =~ 30 seconds to allow for spin up, as per SCSI spec.
                }
        }
#else
        CheckMedia();
#endif

        rcode = OnInit();

        if (rcode)
                goto FailOnInit;

        USBTRACE("MS configured\r\n\r\n");

        bPollEnable = true;

        //USBTRACE("Poll enabled\r\n");
        return 0;

FailSetConfDescr:
#ifdef DEBUG_USB_HOST
        NotifyFailSetConfDescr();
        goto Fail;
#endif

FailOnInit:
#ifdef DEBUG_USB_HOST
        USBTRACE("OnInit:");
        goto Fail;
#endif

FailGetMaxLUN:
#ifdef DEBUG_USB_HOST
        USBTRACE("GetMaxLUN:");
        goto Fail;
#endif

FailInvalidSectorSize:
#ifdef DEBUG_USB_HOST
        USBTRACE("Sector Size is NOT VALID: ");
        goto Fail;
#endif

FailSetDevTblEntry:
#ifdef DEBUG_USB_HOST
        NotifyFailSetDevTblEntry();
        goto Fail;
#endif

FailGetConfDescr:
#ifdef DEBUG_USB_HOST
        NotifyFailGetConfDescr();
#endif

Fail:
#ifdef DEBUG_USB_HOST
        NotifyFail(rcode);
#endif
        Release();
        return rcode;
}

uint32_t BulkOnly::GetCapacity(uint8_t lun) {
        if (LUNOk[lun])
                return CurrentCapacity[lun];
        return 0LU;
}

uint16_t BulkOnly::GetSectorSize(uint8_t lun) {
        if (LUNOk[lun])
                return CurrentSectorSize[lun];
        return 0U;
}

bool BulkOnly::LUNIsGood(uint8_t lun) {
        return LUNOk[lun];
}

void BulkOnly::EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR * pep) {
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
        ClearAllEP();
        pUsb->GetAddressPool().FreeAddress(bAddress);
        return 0;
}

uint8_t BulkOnly::Poll() {
        uint8_t rcode = 0;

        if (!bPollEnable)
                return 0;

        if (qNextPollTime <= millis()) {
                CheckMedia();
        }
        rcode = 0;

        return rcode;
}

uint8_t BulkOnly::GetMaxLUN(uint8_t *plun) {
        uint8_t ret = pUsb->ctrlReq(bAddress, 0, bmREQ_MASSIN, MASS_REQ_GET_MAX_LUN, 0, 0, bIface, 1, 1, plun, NULL);

        if (ret == hrSTALL)
                *plun = 0;

        return 0;
}

uint8_t BulkOnly::ClearEpHalt(uint8_t index) {
        if (index == 0)
                return 0;

        uint8_t ret = 0;

        while (ret = (pUsb->ctrlReq(bAddress, 0, USB_SETUP_HOST_TO_DEVICE | USB_SETUP_TYPE_STANDARD | USB_SETUP_RECIPIENT_ENDPOINT,
                USB_REQUEST_CLEAR_FEATURE, USB_FEATURE_ENDPOINT_HALT, 0, ((index == epDataInIndex) ? (0x80 | epInfo[index].epAddr) : epInfo[index].epAddr), 0, 0, NULL, NULL))
                == 0x01) delay(6);

        if (ret) {
                ErrorMessage<uint8_t > (PSTR("ClearEpHalt"), ret);
                ErrorMessage<uint8_t > (PSTR("EP"), ((index == epDataInIndex) ? (0x80 | epInfo[index].epAddr) : epInfo[index].epAddr));
                return ret;
        }
        epInfo[index].bmSndToggle = 0;
        epInfo[index].bmRcvToggle = 0;
        // epAttribs = 0;
        return 0;
}

uint8_t BulkOnly::Reset() {
        while (pUsb->ctrlReq(bAddress, 0, bmREQ_MASSOUT, MASS_REQ_BOMSR, 0, 0, bIface, 0, 0, NULL, NULL) == 0x01) delay(6);
        return 0;
}

uint8_t BulkOnly::ResetRecovery() {
        Notify(PSTR("\r\nResetRecovery\r\n"), 0x80);
        Notify(PSTR("-----------------\r\n"), 0x80);

#if 0
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

#else
        delay(6);
        Reset();
        delay(6);
        ClearEpHalt(epDataInIndex);
        delay(6);
        bLastUsbError = ClearEpHalt(epDataOutIndex);
#endif
        delay(6);
        return bLastUsbError;
}



#if 0
// TO-DO: Unify CBW creation as much as possible.
// Make and submit CBW.
// if stalled, delay retry
// exit on 100 retries, or anything except stall.

uint8_t SubmitCBW(uint8_t cmd, uint8_t cmdsz, uint8_t lun, uint16_t bsize, uint8_t *buf, uint8_t flags) {
        CommandBlockWrapper cbw;
        SetCurLUN(lun);
        cbw.dCBWSignature = MASS_CBW_SIGNATURE;
        cbw.dCBWTag = ++dCBWTag;
        cbw.dCBWDataTransferLength = bsize;
        cbw.bmCBWFlags = flags;
        cbw.bmCBWLUN = lun;
        cbw.bmCBWCBLength = cmdsz;
        for (uint8_t i = 0; i < 16; i++) cbw.CBWCB[i] = 0;
        cbw.CBWCB[0] = cmd;
        cbw.CBWCB[1] = lun << 5;
        cbw.CBWCB[4] = bsize;
}
#endif


// don't test if OK

uint8_t BulkOnly::Inquiry(uint8_t lun, uint16_t bsize, uint8_t *buf) {
        Notify(PSTR("\r\nInquiry\r\n"), 0x80);
        Notify(PSTR("---------\r\n"), 0x80);

        CommandBlockWrapper cbw;
        SetCurLUN(lun);
        cbw.dCBWSignature = MASS_CBW_SIGNATURE;
        cbw.dCBWTag = ++dCBWTag;
        cbw.dCBWDataTransferLength = bsize;
        cbw.bmCBWFlags = MASS_CMD_DIR_IN;
        cbw.bmCBWLUN = lun;
        cbw.bmCBWCBLength = 6;

        for (uint8_t i = 0; i < 16; i++)
                cbw.CBWCB[i] = 0;

        cbw.CBWCB[0] = SCSI_CMD_INQUIRY;
        cbw.CBWCB[1] = lun << 5;
        cbw.CBWCB[4] = bsize;

        uint8_t rc = HandleSCSIError(Transaction(&cbw, bsize, buf, 0));
#if 0
        if (!rc) {
                printf("LUN %i `", lun);
                for (int i = 8; i < 36; i++) printf("%c", buf[i]);
                printf("'\r\nQualifier %1.1X ", (buf[0]&0xE0) >> 5);
                printf("Device type %2.2X ", buf[0]&0x1f);
                printf("RMB %1.1X ", buf[1]&0x80 >> 7);
                printf("SSCS% 1.1X ", buf[5]&0x80 >> 7);
                uint8_t sv = buf[2];
                printf("SCSI version %2.2X\r\nDevice conforms to ", sv);
                switch (sv) {
                        case 0:
                                printf("No specific");
                                break;
                                /*
                                case 1:
                                        printf("");
                                        break;
                                 */
                        case 2:
                                printf("ANSI 2");
                                break;
                        case 3:
                                printf("ANSI INCITS 301-1997 (SPC)");
                                break;
                        case 4:
                                printf("ANSI INCITS 351-2001 (SPC-2)");
                                break;
                        case 5:
                                printf("ANSI INCITS 408-2005 (SPC-4)");
                                break;
                        case 6:
                                printf("T10/1731-D (SPC-4)");
                                break;
                        default:
                                printf("unknown");
                }
                printf(" standards.\r\n");
        }
#endif
        return rc;
}

uint8_t BulkOnly::LockMedia(uint8_t lun, uint8_t lock) {
        Notify(PSTR("\r\nLockMedia\r\n"), 0x80);
        Notify(PSTR("---------\r\n"), 0x80);

        CommandBlockWrapper cbw;
        SetCurLUN(lun);
        cbw.dCBWSignature = MASS_CBW_SIGNATURE;
        cbw.dCBWTag = ++dCBWTag;
        cbw.dCBWDataTransferLength = 0;
        cbw.bmCBWFlags = MASS_CMD_DIR_IN;
        cbw.bmCBWLUN = lun;
        cbw.bmCBWCBLength = 6;

        for (uint8_t i = 0; i < 16; i++)
                cbw.CBWCB[i] = 0;

        cbw.CBWCB[0] = SCSI_CMD_PREVENT_REMOVAL;
        //cbw.CBWCB[1] = lun << 5;
        cbw.CBWCB[4] = lock;

        return (HandleSCSIError(Transaction(&cbw, 0, NULL, 0)));

}

// don't test if OK, only for use internally.

uint8_t BulkOnly::RequestSense(uint8_t lun, uint16_t size, uint8_t *buf) {
        Notify(PSTR("\r\nRequestSense\r\n"), 0x80);
        Notify(PSTR("----------------\r\n"), 0x80);

        CommandBlockWrapper cbw;
        SetCurLUN(lun);

        cbw.dCBWSignature = MASS_CBW_SIGNATURE;
        cbw.dCBWTag = ++dCBWTag;
        cbw.dCBWDataTransferLength = size;
        cbw.bmCBWFlags = MASS_CMD_DIR_IN;
        cbw.bmCBWLUN = lun;
        cbw.bmCBWCBLength = 6;

        for (uint8_t i = 0; i < 16; i++)
                cbw.CBWCB[i] = 0;

        cbw.CBWCB[0] = SCSI_CMD_REQUEST_SENSE;
        cbw.CBWCB[1] = lun << 5;
        cbw.CBWCB[4] = size;

        return Transaction(&cbw, size, buf, 0);
}

uint8_t BulkOnly::ReadCapacity(uint8_t lun, uint16_t bsize, uint8_t *buf) {
        Notify(PSTR("\r\nReadCapacity\r\n"), 0x80);
        Notify(PSTR("---------------\r\n"), 0x80);
        CommandBlockWrapper cbw;

        SetCurLUN(lun);
        cbw.dCBWSignature = MASS_CBW_SIGNATURE;
        cbw.dCBWTag = ++dCBWTag;
        cbw.dCBWDataTransferLength = bsize;
        cbw.bmCBWFlags = MASS_CMD_DIR_IN;
        cbw.bmCBWLUN = lun;
        cbw.bmCBWCBLength = 10;

        for (uint8_t i = 0; i < 16; i++)
                cbw.CBWCB[i] = 0;

        cbw.CBWCB[0] = SCSI_CMD_READ_CAPACITY_10;
        cbw.CBWCB[1] = lun << 5;

        return HandleSCSIError(Transaction(&cbw, bsize, buf, 0));
}

// don't test if OK

uint8_t BulkOnly::TestUnitReady(uint8_t lun) {
        SetCurLUN(lun);
        if (!bAddress)
                return MASS_ERR_UNIT_NOT_READY;

        Notify(PSTR("\r\nTestUnitReady\r\n"), 0x80);
        Notify(PSTR("-----------------\r\n"), 0x80);

        CommandBlockWrapper cbw;
        uint8_t rc;

        cbw.dCBWSignature = MASS_CBW_SIGNATURE;
        cbw.dCBWTag = ++dCBWTag;
        cbw.dCBWDataTransferLength = 0;
        cbw.bmCBWFlags = MASS_CMD_DIR_OUT;
        cbw.bmCBWLUN = lun;
        cbw.bmCBWCBLength = 6;

        for (uint8_t i = 0; i < 16; i++)
                cbw.CBWCB[i] = 0;

        cbw.CBWCB[0] = SCSI_CMD_TEST_UNIT_READY;
        cbw.CBWCB[1] = lun;
        rc = HandleSCSIError(Transaction(&cbw, 0, NULL, 0));
        return (rc);
}

/* Media control: 0x00 Stop Motor, 0x01 Start Motor, 0x02 Eject Media, 0x03 Load Media */
// don't test if OK

uint8_t BulkOnly::MediaCTL(uint8_t lun, uint8_t ctl) {
        Notify(PSTR("\r\nMediaCTL\r\n"), 0x80);
        Notify(PSTR("-----------------\r\n"), 0x80);
        SetCurLUN(lun);
        uint8_t rcode = MASS_ERR_UNIT_NOT_READY;
        if (bAddress) {
                CommandBlockWrapper cbw;

                cbw.dCBWSignature = MASS_CBW_SIGNATURE;
                cbw.dCBWTag = ++dCBWTag;
                cbw.dCBWDataTransferLength = 0;
                cbw.bmCBWFlags = MASS_CMD_DIR_OUT;
                cbw.bmCBWLUN = lun;
                cbw.bmCBWCBLength = 6;

                for (uint8_t i = 0; i < 16; i++)
                        cbw.CBWCB[i] = 0;

                cbw.CBWCB[0] = SCSI_CMD_START_STOP_UNIT;
                cbw.CBWCB[1] = lun << 5;
                cbw.CBWCB[4] = ctl & 0x03;

                rcode = HandleSCSIError(Transaction(&cbw, 0, NULL, 0));
        }
        return rcode;
}

uint8_t BulkOnly::Read(uint8_t lun, uint32_t addr, uint16_t bsize, uint8_t blocks, uint8_t *buf) {
        if (!LUNOk[lun]) return MASS_ERR_NO_MEDIA;
        Notify(PSTR("\r\nRead LUN:\t"), 0x80);
        PrintHex<uint8_t > (lun, 0x90);
        //printf("LUN=%i LBA=%8.8X BLOCKS=%i SIZE=%i\r\n", lun, addr, blocks, bsize);
        Notify(PSTR("\r\nLBA:\t\t"), 0x90);
        PrintHex<uint32_t > (addr, 0x90);
        Notify(PSTR("\r\nblocks:\t\t"), 0x90);
        PrintHex<uint8_t > (blocks, 0x90);
        Notify(PSTR("\r\nblock size:\t"), 0x90);
        PrintHex<uint16_t > (bsize, 0x90);
        Notify(PSTR("\r\n---------\r\n"), 0x80);
        CommandBlockWrapper cbw;

again:
        cbw.dCBWSignature = MASS_CBW_SIGNATURE;
        cbw.dCBWDataTransferLength = ((uint32_t)bsize * blocks);
        cbw.bmCBWFlags = MASS_CMD_DIR_IN;
        cbw.bmCBWLUN = lun;
        cbw.bmCBWCBLength = 10;

        for (uint8_t i = 0; i < 16; i++)
                cbw.CBWCB[i] = 0;

        cbw.CBWCB[0] = SCSI_CMD_READ_10;
        cbw.CBWCB[1] = lun << 5;
        cbw.CBWCB[2] = ((addr >> 24) & 0xff);
        cbw.CBWCB[3] = ((addr >> 16) & 0xff);
        cbw.CBWCB[4] = ((addr >> 8) & 0xff);
        cbw.CBWCB[5] = (addr & 0xff);
        cbw.CBWCB[8] = blocks;
        cbw.dCBWTag = ++dCBWTag;
        SetCurLUN(lun);
        uint8_t er = HandleSCSIError(Transaction(&cbw, bsize, buf, 0));
        if (er == MASS_ERR_STALL) {
                MediaCTL(lun, 1);
                delay(150);
                if (!TestUnitReady(lun)) goto again;
        }
        return er;
}

/* We won't be needing this... */
uint8_t BulkOnly::Read(uint8_t lun, uint32_t addr, uint16_t bsize, uint8_t blocks, USBReadParser * prs) {
        if (!LUNOk[lun]) return MASS_ERR_NO_MEDIA;
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

uint8_t BulkOnly::Write(uint8_t lun, uint32_t addr, uint16_t bsize, uint8_t blocks, const uint8_t * buf) {
        if (!LUNOk[lun]) return MASS_ERR_NO_MEDIA;
        if (!WriteOk[lun]) return MASS_ERR_WRITE_PROTECTED;
        Notify(PSTR("\r\nWrite LUN:\t"), 0x80);
        PrintHex<uint8_t > (lun, 0x90);
        //printf("LUN=%i LBA=%8.8X BLOCKS=%i SIZE=%i\r\n", lun, addr, blocks, bsize);
        Notify(PSTR("\r\nLBA:\t\t"), 0x90);
        PrintHex<uint32_t > (addr, 0x90);
        Notify(PSTR("\r\nblocks:\t\t"), 0x90);
        PrintHex<uint8_t > (blocks, 0x90);
        Notify(PSTR("\r\nblock size:\t"), 0x90);
        PrintHex<uint16_t > (bsize, 0x90);
        Notify(PSTR("\r\n---------\r\n"), 0x80);
        //MediaCTL(lun, 0x01);
        CommandBlockWrapper cbw;

again:
        cbw.dCBWSignature = MASS_CBW_SIGNATURE;
        cbw.dCBWTag = ++dCBWTag;
        cbw.dCBWDataTransferLength = ((uint32_t)bsize * blocks);
        cbw.bmCBWFlags = MASS_CMD_DIR_OUT;
        cbw.bmCBWLUN = lun;
        cbw.bmCBWCBLength = 10;

        for (uint8_t i = 0; i < 16; i++)
                cbw.CBWCB[i] = 0;

        cbw.CBWCB[0] = SCSI_CMD_WRITE_10;
        cbw.CBWCB[1] = lun << 5;
        cbw.CBWCB[2] = ((addr >> 24) & 0xff);
        cbw.CBWCB[3] = ((addr >> 16) & 0xff);
        cbw.CBWCB[4] = ((addr >> 8) & 0xff);
        cbw.CBWCB[5] = (addr & 0xff);
        cbw.CBWCB[8] = 1;

        SetCurLUN(lun);
        uint8_t er = HandleSCSIError(Transaction(&cbw, bsize, (void*)buf, 0));
        if (er == MASS_ERR_WRITE_STALL) {
                MediaCTL(lun, 1);
                delay(150);
                if (!TestUnitReady(lun)) goto again;
        }
        return er;
}

// don't test if OK

uint8_t BulkOnly::ModeSense(uint8_t lun, uint8_t pc, uint8_t page, uint8_t subpage, uint8_t len, uint8_t * pbuf) {
        Notify(PSTR("\r\rModeSense\r\n"), 0x80);
        Notify(PSTR("------------\r\n"), 0x80);

        CommandBlockWrapper cbw;
        SetCurLUN(lun);

        cbw.dCBWSignature = MASS_CBW_SIGNATURE;
        cbw.dCBWTag = ++dCBWTag;
        cbw.dCBWDataTransferLength = ((uint32_t)len);
        cbw.bmCBWFlags = MASS_CMD_DIR_IN;
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

bool BulkOnly::IsValidCSW(CommandStatusWrapper *pcsw, CommandBlockWrapperBase *pcbw) {
        if (pcsw->dCSWSignature != MASS_CSW_SIGNATURE) {
                Notify(PSTR("CSW:Sig error\r\n"), 0x80);
                //printf("%lx != %lx\r\n", MASS_CSW_SIGNATURE, pcsw->dCSWSignature);
                return false;
        }
        if (pcsw->dCSWTag != pcbw->dCBWTag) {
                Notify(PSTR("CSW:Wrong tag\r\n"), 0x80);
                //printf("%lx != %lx\r\n", pcsw->dCSWTag, pcbw->dCBWTag);
                return false;
        }
        return true;
}

uint8_t BulkOnly::Transaction(CommandBlockWrapper *pcbw, uint16_t buf_size, void *buf, uint8_t flags) {
        uint16_t bytes = (pcbw->dCBWDataTransferLength > buf_size) ? buf_size : pcbw->dCBWDataTransferLength;
        boolean write = (pcbw->bmCBWFlags & MASS_CMD_DIR_IN) != MASS_CMD_DIR_IN;
        boolean callback = (flags & MASS_TRANS_FLG_CALLBACK) == MASS_TRANS_FLG_CALLBACK;
        uint8_t ret = 0;
        uint8_t usberr;
        CommandStatusWrapper csw; // up here, we allocate ahead to save cpu cycles.
        // Fix reserved bits.
        pcbw->bmReserved1 = 0;
        pcbw->bmReserved2 = 0;
        ErrorMessage<uint32_t > (PSTR("CBW.dCBWTag"), pcbw->dCBWTag);

        while ((usberr = pUsb->outTransfer(bAddress, epInfo[epDataOutIndex].epAddr, sizeof (CommandBlockWrapper), (uint8_t*)pcbw)) == hrBUSY) delay(1);

        ret = HandleUsbError(usberr, epDataOutIndex);
        //ret = HandleUsbError(pUsb->outTransfer(bAddress, epInfo[epDataOutIndex].epAddr, sizeof (CommandBlockWrapper), (uint8_t*)pcbw), epDataOutIndex);
        if (ret) {
                ErrorMessage<uint8_t > (PSTR("============================ CBW"), ret);
        } else {
                if (bytes) {
                        if (!write) {
                                if (callback) {
                                        uint8_t rbuf[bytes];
                                        while ((usberr = pUsb->inTransfer(bAddress, epInfo[epDataInIndex].epAddr, &bytes, rbuf)) == hrBUSY) delay(1);
                                        if (usberr == hrSUCCESS) ((USBReadParser*)buf)->Parse(bytes, rbuf, 0);
                                } else {
                                        while ((usberr = pUsb->inTransfer(bAddress, epInfo[epDataInIndex].epAddr, &bytes, (uint8_t*)buf)) == hrBUSY) delay(1);
                                }
                                ret = HandleUsbError(usberr, epDataInIndex);
                        } else {
                                while ((usberr = pUsb->outTransfer(bAddress, epInfo[epDataOutIndex].epAddr, bytes, (uint8_t*)buf)) == hrBUSY) delay(1);
                                ret = HandleUsbError(usberr, epDataOutIndex);
                        }
                        if (ret) {
                                ErrorMessage<uint8_t > (PSTR("============================ DAT"), ret);
                        }
                }
        }

        //if (!ret || ret == MASS_ERR_WRITE_STALL || ret == MASS_ERR_STALL) {
        {
                bytes = sizeof (CommandStatusWrapper);
                int tries = 2;
                while (tries--) {
                        while ((usberr = pUsb->inTransfer(bAddress, epInfo[epDataInIndex].epAddr, &bytes, (uint8_t*) & csw)) == hrBUSY) delay(1);
                        if (!usberr) break;
                        ClearEpHalt(epDataInIndex);
                        //HandleUsbError(usberr, epDataInIndex);
                        if (tries) ResetRecovery();
                }
                if (!ret) {
                        Notify(PSTR("CBW:\t\tOK\r\n"), 0x80);
                        Notify(PSTR("Data Stage:\tOK\r\n"), 0x80);
                } else {
                        // Throw away csw, IT IS NOT OF ANY USE.
                        //HandleUsbError(usberr, epDataInIndex);
                        ResetRecovery();
                        return ret;
                }
                ret = HandleUsbError(usberr, epDataInIndex);
                if (ret) {
                        ErrorMessage<uint8_t > (PSTR("============================ CSW"), ret);
                }
                if (usberr == hrSUCCESS) {
                        if (IsValidCSW(&csw, pcbw)) {
                                //ErrorMessage<uint32_t > (PSTR("CSW.dCBWTag"), csw.dCSWTag);
                                //ErrorMessage<uint8_t > (PSTR("bCSWStatus"), csw.bCSWStatus);
                                //ErrorMessage<uint32_t > (PSTR("dCSWDataResidue"), csw.dCSWDataResidue);
                                Notify(PSTR("CSW:\t\tOK\r\n\r\n"), 0x80);
                                return csw.bCSWStatus;
                        } else {
                                Notify(PSTR("Invalid CSW\r\n"), 0x80);
                                ResetRecovery();
                                //return MASS_ERR_SUCCESS;
                                return MASS_ERR_INVALID_CSW;
                        }
                }

        }
        return ret;
}

uint8_t BulkOnly::SetCurLUN(uint8_t lun) {
        if (lun > bMaxLUN)
                return MASS_ERR_INVALID_LUN;
        bTheLUN = lun;
        return MASS_ERR_SUCCESS;
};

uint8_t BulkOnly::HandleUsbError(uint8_t error, uint8_t index) {
        uint8_t count = 3;

        bLastUsbError = error;
        //if (error)
        //ClearEpHalt(index);
        while (error && count) {
                if (error != hrSUCCESS) {
                        ErrorMessage<uint8_t > (PSTR("USB Error"), error);
                        ErrorMessage<uint8_t > (PSTR("Index"), index);
                }
                switch (error) {
                                // case hrWRONGPID:
                        case hrSUCCESS:
                                return MASS_ERR_SUCCESS;
                        case hrBUSY:
                                // SIE is busy, just hang out and try again.
                                return MASS_ERR_UNIT_BUSY;
                        case hrTIMEOUT:
                        case hrJERR: return MASS_ERR_DEVICE_DISCONNECTED;
                        case hrSTALL:
                                if (index == 0)
                                        return MASS_ERR_STALL;
                                ClearEpHalt(index);
                                if (index != epDataInIndex)
                                        return MASS_ERR_WRITE_STALL;
                                return MASS_ERR_STALL;

                        case hrNAK:
                                if (index == 0)
                                        return MASS_ERR_UNIT_BUSY;
                                return MASS_ERR_UNIT_BUSY;
                                //ClearEpHalt(index);
                                //ResetRecovery();
                                //if (index != epDataInIndex)
                                //        return MASS_ERR_WRITE_NAKS;
                                //return MASS_ERR_READ_NAKS;
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

uint8_t BulkOnly::HandleSCSIError(uint8_t status) {
        uint8_t ret = 0;

        switch (status) {
                case 0: return MASS_ERR_SUCCESS;
                        //case 4: return MASS_ERR_UNIT_BUSY; // Busy means retry later.
                        // case 0x05/0x14: we stalled out
                        // case 0x15/0x16: we naked out.
                case 2:
                        ErrorMessage<uint8_t > (PSTR("Phase Error"), status);
                        ErrorMessage<uint8_t > (PSTR("LUN"), bTheLUN);
                        ResetRecovery();
                        return MASS_ERR_GENERAL_SCSI_ERROR;
                case 1:
                        ErrorMessage<uint8_t > (PSTR("SCSI Error"), status);
                        ErrorMessage<uint8_t > (PSTR("LUN"), bTheLUN);
                        RequestSenseResponce rsp;

                        ret = RequestSense(bTheLUN, sizeof (RequestSenseResponce), (uint8_t*) & rsp);

                        if (ret) {
                                //ResetRecovery();
                                return MASS_ERR_GENERAL_SCSI_ERROR;
                        }
                        ErrorMessage<uint8_t > (PSTR("Response Code"), rsp.bResponseCode);
                        if (rsp.bResponseCode & 0x80) {
                                Notify(PSTR("Information field: "), 0x80);
                                for (int i = 0; i < 4; i++) {
                                        PrintHex<uint8_t > (rsp.CmdSpecificInformation[i], 0x80);
                                        Notify(PSTR(" "), 0x80);
                                }
                                Notify(PSTR("\r\n"), 0x80);
                        }
                        ErrorMessage<uint8_t > (PSTR("Sense Key"), rsp.bmSenseKey);
                        ErrorMessage<uint8_t > (PSTR("Add Sense Code"), rsp.bAdditionalSenseCode);
                        ErrorMessage<uint8_t > (PSTR("Add Sense Qual"), rsp.bAdditionalSenseQualifier);
                        // warning, this is not testing ASQ, only SK and ASC.
                        switch (rsp.bmSenseKey) {
                                        /* bug...
                                        case 0:
                                                return MASS_ERR_SUCCESS;
                                         */
                                case SCSI_S_UNIT_ATTENTION:
                                        switch (rsp.bAdditionalSenseCode) {
                                                case SCSI_ASC_MEDIA_CHANGED:
                                                        return MASS_ERR_MEDIA_CHANGED;
                                                default:
                                                        return MASS_ERR_UNIT_NOT_READY;
                                        }
                                case SCSI_S_NOT_READY:
                                        switch (rsp.bAdditionalSenseCode) {
                                                case SCSI_ASC_MEDIUM_NOT_PRESENT:
                                                        return MASS_ERR_NO_MEDIA;
                                                        //return MASS_ERR_SUCCESS;
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
                                default:
                                        return MASS_ERR_GENERAL_SCSI_ERROR;
                        }

                default:
                        // Should have been handled already in HandleUsbError.
                        // ResetRecovery();
                        ErrorMessage<uint8_t > (PSTR("Gen SCSI Err"), status);
                        ErrorMessage<uint8_t > (PSTR("LUN"), bTheLUN);
                        return status; //MASS_ERR_GENERAL_SCSI_ERROR;
        } // switch
}

void BulkOnly::PrintEndpointDescriptor(const USB_ENDPOINT_DESCRIPTOR * ep_ptr) {
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
