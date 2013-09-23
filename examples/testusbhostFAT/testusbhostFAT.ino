/*
 * Mega + USB storage + optional DS1307 + optional expansion RAM + funky status LED,
 * Includes interactive debug level setting, and supports hot-plug.
 *
 * IMPORTANT! PLEASE USE Arduino 1.0.5 or better!
 * Older versions HAVE MAJOR BUGS AND WILL NOT WORK AT ALL!
 * Use of gcc-avr and lib-c that is newer than the Arduino version is even better.
 * If you experience random crashes, use make.
 * The options that the IDE use can generate bad code and cause the AVR to crash.
 *
 * This sketch requires the following libraries:
 * https://github.com/felis/USB_Host_Shield_2.0 Install as 'USB_Host_Shield_2_0'
 * https://github.com/xxxajk/xmem2 Install as 'xmem', provides memory services.
 * https://github.com/xxxajk/generic_storage provides access to FAT file system.
 * https://github.com/xxxajk/RTClib provides access to DS1307, or fake clock.
 *
 * Optional, to use the Makefile (Recommended! See above!):
 * https://github.com/xxxajk/Arduino_Makefile_master
 *
 */

// You can set this to 0 if you are not using a USB hub.
// It will save a little bit of flash and RAM.
// Set to 1 if you want to use a hub.
#define WANT_HUB_TEST 0


/////////////////////////////////////////////////////////////
// Please Note: This section is for Arduino IDE ONLY.      //
// Use of Make creates a flash image that is 3.3KB smaller //
/////////////////////////////////////////////////////////////
#ifndef USING_MAKEFILE
// Uncomment to enable debugging
//#define DEBUG_USB_HOST
// This is where stderr/USB debugging goes to
#define USB_HOST_SERIAL Serial3
// If you have external memory, setting this to 0 enables FAT table caches.
// The 0 setting is recommended only if you have external memory.
#define _FS_TINY 1

// These you can safely leave alone.
#define _USE_LFN 3
#define EXT_RAM_STACK 1
#define EXT_RAM_HEAP 1
#define _MAX_SS 512
#endif
/////////////////////////////////////////////////////////////
// End of Arduino IDE specific hacks                       //
/////////////////////////////////////////////////////////////

#include <xmem.h>
#if WANT_HUB_TEST
#include <usbhub.h>
#else
#include <Usb.h>
#endif
#include <masstorage.h>
#include <Storage.h>
#include <PCpartition/PCPartition.h>
#include <avr/interrupt.h>
#include <FAT/FAT.h>
#include <Wire.h>
#include <RTClib.h>

static FILE tty_stdio;
static FILE tty_stderr;
USB Usb;

#define LED 13 // the pin that the LED is attached to

volatile int brightness = 0; // how bright the LED is
volatile int fadeAmount = 80; // how many points to fade the LED by
volatile uint8_t current_state = 1;
volatile uint32_t LEDnext_time; // fade timeout
volatile uint8_t last_state = 0;
volatile boolean fatready = false;
volatile boolean partsready = false;
volatile boolean notified = false;
volatile uint32_t HEAPnext_time; // when to print out next heap report
volatile boolean runtest = false;
volatile boolean usbon = false;
volatile uint32_t usbon_time;
volatile boolean change = false;
volatile boolean reportlvl = false;
int cpart = 0;
PCPartition *PT;

#if WANT_HUB_TEST
#define MAX_HUBS 1
USBHub *Hubs[MAX_HUBS];
#endif

static PFAT *Fats[_VOLUMES];
static part_t parts[_VOLUMES];
static storage_t sto[_VOLUMES];

/*make sure this is a power of two. */
#define mbxs 128
static uint8_t My_Buff_x[mbxs]; /* File read buffer */


#define prescale1       ((1 << WGM12) | (1 << CS10))
#define prescale8       ((1 << WGM12) | (1 << CS11))
#define prescale64      ((1 << WGM12) | (1 << CS10) | (1 << CS11))
#define prescale256     ((1 << WGM12) | (1 << CS12))
#define prescale1024    ((1 << WGM12) | (1 << CS12) | (1 << CS10))

extern "C" unsigned int freeHeap();

static int tty_stderr_putc(char c, FILE *t) {
        USB_HOST_SERIAL.write(c);
        return 0;
}

static int tty_std_putc(char c, FILE *t) {
        Serial.write(c);
        return 0;
}

static int tty_std_getc(FILE *t) {
        while (!Serial.available());
        return Serial.read();
}

void setup() {
        boolean serr = false;
        for (int i = 0; i < _VOLUMES; i++) {
                Fats[i] = NULL;
                sto[i].private_data = new pvt_t;
                ((pvt_t *)sto[i].private_data)->B = 255; // impossible
        }
        // Set this to higher values to enable more debug information
        // minimum 0x00, maximum 0xff
        UsbDEBUGlvl = 0x51;
        // make LED pin as an output:
        pinMode(LED, OUTPUT);
        pinMode(2, OUTPUT);
        // Ensure TX is off
        _SFR_BYTE(UCSR0B) &= ~_BV(TXEN0);
        // Initialize 'debug' serial port
        USB_HOST_SERIAL.begin(115200);
        // Do not start primary Serial port if already started.
        if (bit_is_clear(UCSR0B, TXEN0)) {
                Serial.begin(115200);
                serr = true;
        }

        // Set up stdio/stderr
        tty_stdio.put = tty_std_putc;
        tty_stdio.get = tty_std_getc;
        tty_stdio.flags = _FDEV_SETUP_RW;
        tty_stdio.udata = 0;
        stdout = &tty_stdio;
        stdin = &tty_stdio;

        tty_stderr.put = tty_stderr_putc;
        tty_stderr.get = NULL;
        tty_stderr.flags = _FDEV_SETUP_WRITE;
        tty_stderr.udata = 0;
        stderr = &tty_stderr;

        // Blink LED
        delay(500);
        analogWrite(LED, 255);
        delay(500);
        analogWrite(LED, 0);
        delay(500);
        printf_P(PSTR("\r\n\n\n\n\n\n\n\n\n\n\n\n\n\n\nStart\r\n"));
        printf_P(PSTR("Current UsbDEBUGlvl %02x\r\n"), UsbDEBUGlvl);
        printf_P(PSTR("'+' and '-' increase/decrease by 0x01\r\n"));
        printf_P(PSTR("'.' and ',' increase/decrease by 0x10\r\n"));
        printf_P(PSTR("'t' will run a 10MB write/read test and print out the time it took.\r\n"));
        printf_P(PSTR("'e' will toggle vbus off for a few moments.\r\n\r\n"));
        printf_P(PSTR("Long filename support: "
#if _USE_LFN
                "Enabled"
#else
                "Disabled"
#endif
                "\r\n"));
        if (serr) {
                fprintf_P(stderr, PSTR("\r\n\n\n\n\n\n\n\n\n\n\n\n\n\n\nStart\r\n"));
                fprintf_P(stderr, PSTR("Current UsbDEBUGlvl %02x\r\n"), UsbDEBUGlvl);
                fprintf_P(stderr, PSTR("Long filename support: "
#if _USE_LFN
                        "Enabled"
#else
                        "Disabled"
#endif
                        "\r\n"));
        }
        analogWrite(LED, 255);
        delay(500);
        analogWrite(LED, 0);
        delay(500);
        analogWrite(LED, 255);
        delay(500);
        analogWrite(LED, 0);
        delay(500);
        analogWrite(LED, 255);
        delay(500);
        analogWrite(LED, 0);
        delay(500);

        LEDnext_time = millis() + 1;
#ifdef EXT_RAM
        printf_P(PSTR("Total EXT RAM banks %i\r\n"), xmem::getTotalBanks());
#endif
        printf_P(PSTR("Available heap: %u Bytes\r\n"), freeHeap());
        printf_P(PSTR("SP %x\r\n"), (uint8_t *)(SP));

        // Even though I'm not going to actually be deleting,
        // I want to be able to have slightly more control.
        // Besides, it is easier to initialize stuff...
#if WANT_HUB_TEST
        for (int i = 0; i < MAX_HUBS; i++) {
                Hubs[i] = new USBHub(&Usb);
                printf_P(PSTR("Available heap: %u Bytes\r\n"), freeHeap());
        }
#endif
        // Initialize generic storage. This must be done before USB starts.
        InitStorage();

        while (Usb.Init(1000) == -1) {
                printf_P(PSTR("No USB HOST Shield?\r\n"));
                Notify(PSTR("OSC did not start."), 0x40);
        }
        // usb VBUS _OFF_
        //Usb.gpioWr(0x00);
        //digitalWrite(2, 0);
        //usbon_time = millis() + 2000;
        cli();
        TCCR3A = 0;
        TCCR3B = 0;
        // (0.01/(1/((16 *(10^6)) / 8))) - 1 = 19999
        OCR3A = 19999;
        TCCR3B |= prescale8;
        TIMSK3 |= (1 << OCIE1A);
        sei();

        HEAPnext_time = millis() + 10000;
}

void serialEvent() {
        // Adjust UsbDEBUGlvl level on-the-fly.
        // + to increase, - to decrease, * to display current level.
        // . to increase by 16, , to decrease by 16
        // e to flick VBUS
        // * to report debug level
        if (Serial.available()) {
                int inByte = Serial.read();
                switch (inByte) {
                        case '+':
                                if (UsbDEBUGlvl < 0xff) UsbDEBUGlvl++;
                                reportlvl = true;
                                break;
                        case '-':
                                if (UsbDEBUGlvl > 0x00) UsbDEBUGlvl--;
                                reportlvl = true;
                                break;
                        case '.':
                                if (UsbDEBUGlvl < 0xf0) UsbDEBUGlvl += 16;
                                reportlvl = true;
                                break;
                        case ',':
                                if (UsbDEBUGlvl > 0x0f) UsbDEBUGlvl -= 16;
                                reportlvl = true;
                                break;
                        case '*':
                                reportlvl = true;
                                break;
                        case 't':
                                runtest = true;
                                break;
                        case 'e':
                                change = true;
                                usbon = false;
                                break;
                }
        }
}

ISR(TIMER3_COMPA_vect) {
        if (millis() >= LEDnext_time) {
                LEDnext_time = millis() + 30;

                // set the brightness of LED
                analogWrite(LED, brightness);

                // change the brightness for next time through the loop:
                brightness = brightness + fadeAmount;

                // reverse the direction of the fading at the ends of the fade:
                if (brightness <= 0) {
                        brightness = 0;
                        fadeAmount = -fadeAmount;
                }
                if (brightness >= 255) {
                        brightness = 255;
                        fadeAmount = -fadeAmount;
                }
        }
}

bool isfat(uint8_t t) {
        return (t == 0x01 || t == 0x04 || t == 0x06 || t == 0x0b || t == 0x0c || t == 0x0e || t == 0x1);
}

void die(FRESULT rc) {
        printf_P(PSTR("Failed with rc=%u.\r\n"), rc);
        //for (;;);
}

void loop() {
        FIL My_File_Object_x; /* File object */

        // Print a heap status report about every 10 seconds.
        if (millis() >= HEAPnext_time) {
                if (UsbDEBUGlvl > 0x50) {
                        printf_P(PSTR("Available heap: %u Bytes\r\n"), freeHeap());
                }
                HEAPnext_time = millis() + 10000;
        }

        // Horrid! This sort of thing really belongs in an ISR, not here!
        // We also will be needing to test each hub port, we don't do this yet!
        if (!change && !usbon && millis() >= usbon_time) {
                change = true;
                usbon = true;
        }

        if (change) {
                change = false;
                if (usbon) {
                        Usb.vbusPower(vbus_on);
                        printf_P(PSTR("VBUS on\r\n"));
                } else {
                        Usb.vbusPower(vbus_off);
                        usbon_time = millis() + 2000;
                }
        }
        Usb.Task();
        current_state = Usb.getUsbTaskState();
        if (current_state != last_state) {
                if (UsbDEBUGlvl > 0x50)
                        printf_P(PSTR("USB state = %x\r\n"), current_state);
                if (current_state == USB_STATE_RUNNING) {
                        fadeAmount = 30;
                }
                if (current_state == USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE) {
                        fadeAmount = 80;
                        partsready = false;
                        for (int i = 0; i < cpart; i++) {
                                if (Fats[i] != NULL)
                                        delete Fats[i];
                                Fats[i] = NULL;
                        }
                        fatready = false;
                        notified = false;
                        cpart = 0;
                }
                last_state = current_state;
        }

        // only do any of this if usb is on
        if (usbon) {
                if (partsready && !fatready) {
                        if (cpart > 0) fatready = true;
                }

                // This is horrible, and needs to be moved elsewhere!
                for (int B = 0; B < MAX_USB_MS_DRIVERS; B++) {
                        if (!partsready && Bulk[B]->GetAddress() != NULL) {
                                // Build a list.
                                int ML = Bulk[B]->GetbMaxLUN();
                                //printf("MAXLUN = %i\r\n", ML);
                                ML++;
                                for (int i = 0; i < ML; i++) {
                                        if (Bulk[B]->LUNIsGood(i)) {
                                                partsready = true;
                                                ((pvt_t *)sto[i].private_data)->lun = i;
                                                ((pvt_t *)sto[i].private_data)->B = B;
                                                sto[i].Read = *PRead;
                                                sto[i].Write = *PWrite;
                                                sto[i].Reads = *PReads;
                                                sto[i].Writes = *PWrites;
                                                sto[i].Status = *PStatus;
                                                sto[i].TotalSectors = Bulk[B]->GetCapacity(i);
                                                sto[i].SectorSize = Bulk[B]->GetSectorSize(i);
                                                printf_P(PSTR("LUN:\t\t%u\r\n"), i);
                                                printf_P(PSTR("Total Sectors:\t%08lx\t%lu\r\n"), sto[i].TotalSectors, sto[i].TotalSectors);
                                                printf_P(PSTR("Sector Size:\t%04x\t\t%u\r\n"), sto[i].SectorSize, sto[i].SectorSize);
                                                // get the partition data...
                                                PT = new PCPartition;

                                                if (!PT->Init(&sto[i])) {
                                                        part_t *apart;
                                                        for (int j = 0; j < 4; j++) {
                                                                apart = PT->GetPart(j);
                                                                if (apart != NULL && apart->type != 0x00) {
                                                                        memcpy(&(parts[cpart]), apart, sizeof (part_t));
                                                                        printf_P(PSTR("Partition %u type %#02x\r\n"), j, parts[cpart].type);
                                                                        // for now
                                                                        if (isfat(parts[cpart].type)) {
                                                                                Fats[cpart] = new PFAT(&sto[i], cpart, parts[cpart].firstSector);
                                                                                //int r = Fats[cpart]->Good();
                                                                                if (Fats[cpart]->Good()) {
                                                                                        delete Fats[cpart];
                                                                                        Fats[cpart] = NULL;
                                                                                } else cpart++;
                                                                        }
                                                                }
                                                        }
                                                } else {
                                                        // try superblock
                                                        Fats[cpart] = new PFAT(&sto[i], cpart, 0);
                                                        //int r = Fats[cpart]->Good();
                                                        if (Fats[cpart]->Good()) {
                                                                //printf_P(PSTR("Superblock error %x\r\n"), r);
                                                                delete Fats[cpart];
                                                                Fats[cpart] = NULL;
                                                        } else cpart++;

                                                }
                                                delete PT;
                                        } else {
                                                sto[i].Read = NULL;
                                                sto[i].Write = NULL;
                                                sto[i].Writes = NULL;
                                                sto[i].Reads = NULL;
                                                sto[i].TotalSectors = 0UL;
                                                sto[i].SectorSize = 0;
                                        }
                                }

                        }
                }

                if (fatready) {
                        if (Fats[0] != NULL) {
                                struct Pvt * p;
                                p = ((struct Pvt *)(Fats[0]->storage->private_data));
                                if (!Bulk[p->B]->LUNIsGood(p->lun)) {
                                        // media change
                                        fadeAmount = 80;
                                        partsready = false;
                                        for (int i = 0; i < cpart; i++) {
                                                if (Fats[i] != NULL)
                                                        delete Fats[i];
                                                Fats[cpart] = NULL;
                                        }
                                        fatready = false;
                                        notified = false;
                                        cpart = 0;
                                }

                        }
                }
                if (fatready) {
                        FRESULT rc; /* Result code */
                        UINT bw, br, i;

                        if (!notified) {
                                fadeAmount = 5;
                                notified = true;
                                printf_P(PSTR("\r\nOpen an existing file (message.txt).\r\n"));
                                rc = f_open(&My_File_Object_x, "0:/MESSAGE.TXT", FA_READ);
                                if (rc) printf_P(PSTR("Error %i, message.txt not found.\r\n"));
                                else {
                                        printf_P(PSTR("\r\nType the file content.\r\n"));
                                        for (;;) {
                                                rc = f_read(&My_File_Object_x, My_Buff_x, mbxs, &br); /* Read a chunk of file */
                                                if (rc || !br) break; /* Error or end of file */
                                                for (i = 0; i < br; i++) {
                                                        /* Type the data */
                                                        if (My_Buff_x[i] == '\n')
                                                                Serial.write('\r');
                                                        if (My_Buff_x[i] != '\r')
                                                                Serial.write(My_Buff_x[i]);
                                                        Serial.flush();
                                                }
                                        }
                                        if (rc) {
                                                f_close(&My_File_Object_x);
                                                goto out;
                                        }

                                        printf_P(PSTR("\r\nClose the file.\r\n"));
                                        rc = f_close(&My_File_Object_x);
                                        if (rc) goto out;
                                }
                                printf_P(PSTR("\r\nCreate a new file (hello.txt).\r\n"));
                                rc = f_open(&My_File_Object_x, "0:/Hello.TxT", FA_WRITE | FA_CREATE_ALWAYS);
                                if (rc) {
                                        die(rc);
                                        goto outdir;
                                }
                                printf_P(PSTR("\r\nWrite a text data. (Hello world!)\r\n"));
                                rc = f_write(&My_File_Object_x, "Hello world!\r\n", 14, &bw);
                                if (rc) {
                                        goto out;
                                }
                                printf_P(PSTR("%u bytes written.\r\n"), bw);

                                printf_P(PSTR("\r\nClose the file.\r\n"));
                                rc = f_close(&My_File_Object_x);
                                if (rc) {
                                        die(rc);
                                        goto out;
                                }
outdir:
                                {
#if _USE_LFN
                                        char lfn[_MAX_LFN + 1];
                                        FILINFO My_File_Info_Object_x; /* File information object */
                                        My_File_Info_Object_x.lfname = lfn;
#endif
                                        DIR My_Dir_Object_x; /* Directory object */
                                        printf_P(PSTR("\r\nOpen root directory.\r\n"));
                                        rc = f_opendir(&My_Dir_Object_x, "0:/");
                                        if (rc) {
                                                die(rc);
                                                goto out;
                                        }

                                        printf_P(PSTR("\r\nDirectory listing...\r\n"));
                                        printf_P(PSTR("Available heap: %u Bytes\r\n"), freeHeap());
                                        for (;;) {
#if _USE_LFN
                                                My_File_Info_Object_x.lfsize = _MAX_LFN;
#endif

                                                rc = f_readdir(&My_Dir_Object_x, &My_File_Info_Object_x); /* Read a directory item */
                                                if (rc || !My_File_Info_Object_x.fname[0]) break; /* Error or end of dir */

                                                if (My_File_Info_Object_x.fattrib & AM_DIR) {
                                                        Serial.write('d');
                                                } else {
                                                        Serial.write('-');
                                                }
                                                Serial.write('r');

                                                if (My_File_Info_Object_x.fattrib & AM_RDO) {
                                                        Serial.write('-');
                                                } else {
                                                        Serial.write('w');
                                                }
                                                if (My_File_Info_Object_x.fattrib & AM_HID) {
                                                        Serial.write('h');
                                                } else {
                                                        Serial.write('-');
                                                }

                                                if (My_File_Info_Object_x.fattrib & AM_SYS) {
                                                        Serial.write('s');
                                                } else {
                                                        Serial.write('-');
                                                }

                                                if (My_File_Info_Object_x.fattrib & AM_ARC) {
                                                        Serial.write('a');
                                                } else {
                                                        Serial.write('-');
                                                }

#if _USE_LFN
                                                if (*My_File_Info_Object_x.lfname)
                                                        printf_P(PSTR(" %8lu  %s (%s)\r\n"), My_File_Info_Object_x.fsize, My_File_Info_Object_x.fname, My_File_Info_Object_x.lfname);
                                                else
#endif
                                                        printf_P(PSTR(" %8lu  %s\r\n"), My_File_Info_Object_x.fsize, &(My_File_Info_Object_x.fname[0]));
                                        }
                                }
out:
                                if (rc) die(rc);
                                printf_P(PSTR("\r\nTest completed.\r\n"));

                        }

                        if (runtest) {
                                ULONG ii, wt, rt, start, end;
                                runtest = false;
                                f_unlink("0:/10MB.bin");
                                printf_P(PSTR("\r\nCreate a new 10MB test file (10MB.bin).\r\n"));
                                rc = f_open(&My_File_Object_x, "0:/10MB.bin", FA_WRITE | FA_CREATE_ALWAYS);
                                if (rc) goto failed;
                                for (bw = 0; bw < mbxs; bw++) My_Buff_x[bw] = bw & 0xff;
                                start = millis();
                                for (ii = 10485760LU / mbxs; ii > 0LU; ii--) {
                                        rc = f_write(&My_File_Object_x, My_Buff_x, mbxs, &bw);
                                        if (rc || !bw) goto failed;
                                }
                                rc = f_close(&My_File_Object_x);
                                if (rc) goto failed;
                                end = millis();
                                wt = end - start;
                                printf_P(PSTR("Time to write 10485760 bytes: %lu ms (%lu sec) \r\n"), wt, (500 + wt) / 1000UL);
                                rc = f_open(&My_File_Object_x, "0:/10MB.bin", FA_READ);
                                start = millis();
                                if (rc) goto failed;
                                for (;;) {
                                        rc = f_read(&My_File_Object_x, My_Buff_x, mbxs, &bw); /* Read a chunk of file */
                                        if (rc || !bw) break; /* Error or end of file */
                                }
                                end = millis();
                                if (rc) goto failed;
                                rc = f_close(&My_File_Object_x);
                                if (rc) goto failed;
                                rt = end - start;
                                printf_P(PSTR("Time to read 10485760 bytes: %lu ms (%lu sec)\r\nDelete test file\r\n"), rt, (500 + rt) / 1000UL);
failed:
                                if (rc) die(rc);
                                printf_P(PSTR("10MB timing test finished.\r\n"));
                        }
                }
        }
}
