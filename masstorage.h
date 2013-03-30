#if !defined(__MASSTORAGE_H__)
#define __MASSTORAGE_H__

#define DEBUG

#include <inttypes.h>
#include "avrpins.h"
#include <avr/pgmspace.h>
#include "max3421e.h"
#include "usbhost.h"
#include "usb_ch9.h"
#include "Usb.h"

#if defined(ARDUINO) && ARDUINO >=100
#include "Arduino.h"
#else
#include <WProgram.h>
#endif

#include <confdescparser.h>

#define SWAP(a, b) (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b)))

#define bmREQ_MASSOUT       USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_INTERFACE
#define bmREQ_MASSIN        USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_INTERFACE

// Mass Storage Subclass Constants
#define MASS_SUBCLASS_SCSI_NOT_REPORTED 0x00	// De facto use
#define MASS_SUBCLASS_RBC		0x01
#define MASS_SUBCLASS_ATAPI		0x02	// MMC-5 (ATAPI)
#define MASS_SUBCLASS_OBSOLETE1		0x03	// Was QIC-157
#define MASS_SUBCLASS_UFI		0x04	// Specifies how to interface Floppy Disk Drives to USB
#define MASS_SUBCLASS_OBSOLETE2		0x05	// Was SFF-8070i
#define MASS_SUBCLASS_SCSI		0x06	// SCSI Transparent Command Set
#define MASS_SUBCLASS_LSDFS		0x07	// Specifies how host has to negotiate access before trying SCSI
#define MASS_SUBCLASS_IEEE1667		0x08

// Mass Storage Class Protocols
#define MASS_PROTO_CBI			0x00	// CBI (with command completion interrupt)
#define MASS_PROTO_CBI_NO_INT		0x01	// CBI (without command completion interrupt)
#define MASS_PROTO_OBSOLETE		0x02
#define MASS_PROTO_BBB			0x50	// Bulk Only Transport
#define MASS_PROTO_UAS			0x62

// Request Codes
#define MASS_REQ_ADSC			0x00
#define MASS_REQ_GET			0xFC
#define MASS_REQ_PUT			0xFD
#define MASS_REQ_GET_MAX_LUN		0xFE
#define MASS_REQ_BOMSR			0xFF	// Bulk-Only Mass Storage Reset

#define MASS_CBW_SIGNATURE		0x43425355
#define MASS_CSW_SIGNATURE		0x53425355

#define MASS_CMD_DIR_OUT                (0 << 7)
#define MASS_CMD_DIR_IN			(1 << 7)

#define SCSI_CMD_INQUIRY		0x12
#define SCSI_CMD_REPORT_LUNS		0xA0
#define SCSI_CMD_REQUEST_SENSE		0x03
#define SCSI_CMD_FORMAT_UNIT		0x04
#define SCSI_CMD_READ_6			0x08
#define SCSI_CMD_READ_10		0x28
#define SCSI_CMD_READ_CAPACITY_10	0x25
#define SCSI_CMD_TEST_UNIT_READY	0x00
#define SCSI_CMD_WRITE_6		0x0A
#define SCSI_CMD_WRITE_10		0x2A
#define SCSI_CMD_MODE_SENSE_6		0x1A
#define SCSI_CMD_MODE_SENSE_10		0x5A
#define SCSI_CMD_START_STOP_UNIT	0x1B

#define SCSI_S_NOT_READY		0x02
#define SCSI_S_MEDIUM_ERROR		0x03
#define SCSI_S_ILLEGAL_REQUEST		0x05
#define SCSI_S_UNIT_ATTENTION		0x06

#define SCSI_ASC_MEDIUM_NOT_PRESENT     0x3A
#define SCSI_ASC_LBA_OUT_OF_RANGE       0x21


#define MASS_ERR_SUCCESS		0x00
#define MASS_ERR_PHASE_ERROR		0x02
#define MASS_ERR_UNIT_NOT_READY		0x03
#define MASS_ERR_UNIT_BUSY		0x04
#define MASS_ERR_STALL			0x05
#define MASS_ERR_CMD_NOT_SUPPORTED	0x06
#define MASS_ERR_INVALID_CSW		0x07
#define MASS_ERR_NO_MEDIA		0x08
#define MASS_ERR_BAD_LBA                0x09
#define MASS_ERR_DEVICE_DISCONNECTED    0x11
#define MASS_ERR_UNABLE_TO_RECOVER	0x12	// Reset recovery error
#define MASS_ERR_INVALID_LUN		0x13
#define MASS_ERR_GENERAL_SCSI_ERROR	0xFE
#define MASS_ERR_GENERAL_USB_ERROR	0xFF
#define MASS_ERR_USER			0xA0	// For subclasses to define their own error codes

#define MASS_TRANS_FLG_CALLBACK         0x01	// Callback is involved
#define MASS_TRANS_FLG_NO_STALL_CHECK   0x02	// STALL condition is not checked
#define MASS_TRANS_FLG_NO_PHASE_CHECK   0x04	// PHASE_ERROR is not checked

struct Capacity {
        uint8_t data[8];
        //uint32_t dwBlockAddress;
        //uint32_t dwBlockLength;
} __attribute__((packed));

struct InquiryResponse {
        uint8_t DeviceType : 5;
        uint8_t PeripheralQualifier : 3;

        unsigned Reserved : 7;
        unsigned Removable : 1;

        uint8_t Version;

        unsigned ResponseDataFormat : 4;
        unsigned Reserved2 : 1;
        unsigned NormACA : 1;
        unsigned TrmTsk : 1;
        unsigned AERC : 1;

        uint8_t AdditionalLength;
        uint8_t Reserved3[2];

        unsigned SoftReset : 1;
        unsigned CmdQue : 1;
        unsigned Reserved4 : 1;
        unsigned Linked : 1;
        unsigned Sync : 1;
        unsigned WideBus16Bit : 1;
        unsigned WideBus32Bit : 1;
        unsigned RelAddr : 1;

        uint8_t VendorID[8];
        uint8_t ProductID[16];
        uint8_t RevisionID[4];
} __attribute__((packed));

struct CommandBlockWrapperBase {
        uint32_t dCBWSignature;
        uint32_t dCBWTag;
        uint32_t dCBWDataTransferLength;
        uint8_t bmCBWFlags;
} __attribute__((packed));

struct CommandBlockWrapper : public CommandBlockWrapperBase {

        struct {
                uint8_t bmCBWLUN : 4;
                uint8_t bmReserved1 : 4;
        };

        struct {
                uint8_t bmCBWCBLength : 4;
                uint8_t bmReserved2 : 4;
        };

        uint8_t CBWCB[16];
} __attribute__((packed));

struct CommandStatusWrapper {
        uint32_t dCSWSignature;
        uint32_t dCSWTag;
        uint32_t dCSWDataResidue;
        uint8_t bCSWStatus;
} __attribute__((packed));

struct RequestSenseResponce {
        uint8_t bResponseCode;
        uint8_t bSegmentNumber;

        uint8_t bmSenseKey : 4;
        uint8_t bmReserved : 1;
        uint8_t bmILI : 1;
        uint8_t bmEOM : 1;
        uint8_t bmFileMark : 1;

        uint8_t Information[4];
        uint8_t bAdditionalLength;
        uint8_t CmdSpecificInformation[4];
        uint8_t bAdditionalSenseCode;
        uint8_t bAdditionalSenseQualifier;
        uint8_t bFieldReplaceableUnitCode;
        uint8_t SenseKeySpecific[3];
} __attribute__((packed));

#define MASS_MAX_ENDPOINTS		3

class BulkOnly : public USBDeviceConfig, public UsbConfigXtracter {
protected:
        static const uint8_t epDataInIndex; // DataIn endpoint index
        static const uint8_t epDataOutIndex; // DataOUT endpoint index
        static const uint8_t epInterruptInIndex; // InterruptIN  endpoint index

        USB *pUsb;
        uint8_t bAddress;
        uint8_t bConfNum; // configuration number
        uint8_t bIface; // interface value
        uint8_t bNumEP; // total number of EP in the configuration
        uint32_t qNextPollTime; // next poll time
        bool bPollEnable; // poll enable flag

        EpInfo epInfo[MASS_MAX_ENDPOINTS];

        uint32_t dCBWTag; // Tag
        uint32_t dCBWDataTransferLength; // Data Transfer Length
        uint8_t bLastUsbError; // Last USB error
        uint8_t bMaxLUN; // Max LUN
        uint8_t bTheLUN; // Active LUN

protected:
        void PrintEndpointDescriptor(const USB_ENDPOINT_DESCRIPTOR* ep_ptr);

        bool IsValidCBW(uint8_t size, uint8_t *pcbw);
        bool IsMeaningfulCBW(uint8_t size, uint8_t *pcbw);

        bool IsValidCSW(CommandStatusWrapper *pcsw, CommandBlockWrapperBase *pcbw);

        uint8_t ClearEpHalt(uint8_t index);
        uint8_t Transaction(CommandBlockWrapper *cbw, uint16_t bsize, void *buf, uint8_t flags);
        uint8_t HandleUsbError(uint8_t error, uint8_t index);
        uint8_t HandleSCSIError(uint8_t status);

public:
        BulkOnly(USB *p);

        uint8_t GetLastUsbError() {
                return bLastUsbError;
        };

        uint8_t GetbMaxLUN() {
                return bMaxLUN; // Max LUN
        }

        uint8_t GetbTheLUN() {
                return bTheLUN; // Active LUN
        }

        uint8_t Reset();
        uint8_t GetMaxLUN(uint8_t *max_lun);
        uint8_t SetCurLUN(uint8_t lun);

        uint8_t ResetRecovery();
        uint8_t Inquiry(uint8_t lun, uint16_t size, uint8_t *buf);
        uint8_t TestUnitReady(uint8_t lun);
        uint8_t ReadCapacity(uint8_t lun, uint16_t size, uint8_t *buf);
        uint8_t RequestSense(uint8_t lun, uint16_t size, uint8_t *buf);
        uint8_t ModeSense(uint8_t lun, uint8_t pc, uint8_t page, uint8_t subpage, uint8_t len, uint8_t *buf);
        uint8_t MediaCTL(uint8_t lun, uint8_t ctl);
        uint8_t Read(uint8_t lun, uint32_t addr, uint16_t bsize, uint8_t blocks, uint8_t *buf);
        uint8_t Read(uint8_t lun, uint32_t addr, uint16_t bsize, uint8_t blocks, USBReadParser *prs);
        uint8_t Write(uint8_t lun, uint32_t addr, uint16_t bsize, uint8_t blocks, const uint8_t *buf);

        // USBDeviceConfig implementation
        virtual uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed);
        virtual uint8_t Release();
        virtual uint8_t Poll();

        virtual uint8_t GetAddress() {
                return bAddress;
        };

        // UsbConfigXtracter implementation
        virtual void EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR *ep);

protected:
        // Additional Initialization Method for Subclasses

        virtual uint8_t OnInit() {
                return 0;
        };
};

#endif // __MASSTORAGE_H__
