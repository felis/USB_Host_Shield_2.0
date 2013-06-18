

/*
 * Mega + USB storage + optional expansion RAM + funky status LED,
 * Includes interactive debug level setting, and supports emulated hot-plug.
 *
 * IMPORTANT! PLEASE USE Arduino 1.0.5 or better!
 * Older versions HAVE MAJOR BUGS AND WILL NOT WORK AT ALL!
 * Use of gcc-avr and lib-c that is newer than the Arduino version is even better.
 *
 */

#define WANT_HUB_TEST 1
#ifndef HAVE_XMEM
// Set this to zero to disable xmem
#define HAVE_XMEM 1
#endif


#include <avr/pgmspace.h>
#if HAVE_XMEM
#include <xmem.h>
#endif
#include <avrpins.h>
#include <max3421e.h>
#include <usbhost.h>
#include <usb_ch9.h>
#include <address.h>
#include <Usb.h>
#if WANT_HUB_TEST
#include <usbhub.h>
#endif
#include <masstorage.h>
#include <message.h>
#include <avr/interrupt.h>
#include <PCpartition/PCPartition.h>
#include <Storage.h>
#include <FAT/FAT.h>

// Warning! Do not use this unless you are aware of what it does!
#if HAVE_XMEM
#define GOD_MODE 0
#endif
static FILE mystdout;

int led = 13; // the pin that the LED is attached to
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
#define MAX_HUBS 2
static USBHub *Hubs[MAX_HUBS];
#endif

static PFAT *Fats[_VOLUMES];
static part_t parts[_VOLUMES];
static storage_t sto[_VOLUMES];

#define prescale1       ((1 << WGM12) | (1 << CS10))
#define prescale8       ((1 << WGM12) | (1 << CS11))
#define prescale64      ((1 << WGM12) | (1 << CS10) | (1 << CS11))
#define prescale256     ((1 << WGM12) | (1 << CS12))
#define prescale1024    ((1 << WGM12) | (1 << CS12) | (1 << CS10))

extern "C" unsigned int freeHeap();

/*
unsigned int getHeapend(){
        extern unsigned int __heap_start;

        if ((unsigned int)__brkval == 0) {
                return (unsigned int)&__heap_start;
        } else {
                return (unsigned int)__brkval;
        }
}

unsigned int freeHeap() {
        if (SP < (unsigned int)__malloc_heap_start) {
                return ((unsigned int)__malloc_heap_end - getHeapend());
        } else {
                return (SP - getHeapend());
        }
}
 */
static int my_putc(char c, FILE *t) {
        Serial.write(c);
}

void setup() {
        for (int i = 0; i < _VOLUMES; i++) {
                Fats[i] = NULL;
        }
        // Set this to higher values to enable more debug information
        // minimum 0x00, maximum 0xff
        UsbDEBUGlvl = 0x51;
        // declare pin 9 to be an output:
        pinMode(led, OUTPUT);
        pinMode(2, OUTPUT);
        // Initialize 'debug' serial port
        Serial.begin(115200);

        //fdevopen(&my_putc, 0);
        // too bad we can't tinker with iob directly, oh well.
        mystdout.put = my_putc;
        mystdout.get = NULL;
        mystdout.flags = _FDEV_SETUP_WRITE;
        mystdout.udata = 0;
        stdout = &mystdout;

        // Blink pin 9:
        delay(500);
        analogWrite(led, 255);
        delay(500);
        analogWrite(led, 0);
        delay(500);
        analogWrite(led, 255);
        delay(500);
        analogWrite(led, 0);
        delay(500);
        analogWrite(led, 255);
        delay(500);
        analogWrite(led, 0);
        printf_P(PSTR("\r\n\n\n\n\n\n\n\n\n\n\n\n\n\n\nStart\r\n"));
        printf_P(PSTR("Current UsbDEBUGlvl %02x\r\n"), UsbDEBUGlvl);
        printf_P(PSTR("'+' and '-' increase/decrease by 0x01\r\n"));
        printf_P(PSTR("'.' and ',' increase/decrease by 0x10\r\n"));
        printf_P(PSTR("'t' will run a 10MB write/read test and print out the time it took.\r\n"));
        printf_P(PSTR("'e' will toggle vbus off for a few moments.\r\n"));
        printf_P(PSTR("\r\n\r\nLong filename support: "
#if _USE_LFN
                "Enabled"
#else
                "Disabled"
#endif
                "\r\n"));
        analogWrite(led, 255);
        delay(500);
        analogWrite(led, 0);
        delay(500);

        delay(100);
        analogWrite(led, 255);
        delay(100);
        analogWrite(led, 0);
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
        while (Usb.Init() == -1) {
                printf_P(PSTR("No\r\n"));
                Notify(PSTR("OSC did not start."), 0x40);
        }
        // usb VBUS _OFF_
        Usb.gpioWr(0x00);
        digitalWrite(2, 0);
        usbon_time = millis() + 2000;
        cli();
        TCCR3A = 0;
        TCCR3B = 0;
        // (0.01/(1/((16 *(10^6)) / 8))) - 1 = 19999
#if GOD_MODE
        OCR3A = 10;
#else
        OCR3A = 19999;
#endif
        TCCR3B |= prescale8;
        TIMSK3 |= (1 << OCIE1A);
        sei();

        HEAPnext_time = millis() + 10000;
}

#if GOD_MODE
volatile uint16_t *foof = reinterpret_cast<uint16_t *>(0x2200);
#endif

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
#if GOD_MODE
                        case 'z':
                                cli();
                                *foof = 0xffff;
                                *(foof + 2) = 0x0000;
                                sei();
                                break;
#endif
                }
        }
}

ISR(TIMER3_COMPA_vect) {
#if GOD_MODE
#if !EXT_RAM_HEAP && !EXT_RAM_STACK
        // Super cool debug feature to detect stack and heap collisions.
        // We can use this in conjuction with the AVR dragon
        // to check max depth of the stack and height of heap.
        if (*foof >= (uint16_t)(SP)) {
                *foof = (uint16_t)(SP);
                *(foof + 2) = (uint16_t)__brkval;
        }
#endif
#endif
        if (millis() >= LEDnext_time) {
                LEDnext_time = millis() + 30;

                // set the brightness of pin 9:
                analogWrite(led, brightness);

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

/*make sure this is a power of two. */
#define mbxs 128

void loop() {
        uint8_t My_Buff_x[mbxs]; /* File read buffer */
        FIL My_File_Object_x; /* File object */
        DIR My_Dir_Object_x; /* Directory object */
        FILINFO My_File_Info_Object_x; /* File information object */

        // Print a heap status report about every 10 seconds.
        if (millis() >= HEAPnext_time) {
                if (UsbDEBUGlvl > 0x50) {
                        printf_P(PSTR("Available heap: %u Bytes\r\n"), freeHeap());
#if GOD_MODE
                        cli();
                        uint16_t p = *foof;
                        sei();
                        printf_P(PSTR("MAXSP %4.4x, current %4.4x\r\n"), p, SP);
#endif
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
                        printf_P(PSTR("VBUS on\r\n"));
                        Usb.gpioWr(0xFF);
                        digitalWrite(2, 1);
                } else {
                        Usb.gpioWr(0x00);
                        digitalWrite(2, 0);
                        usbon = false;
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
                        /*
                        partsready = false;
                        for (int i = 0; i < cpart; i++) {
                                if (Fats[i] != NULL)
                                        delete Fats[i];
                        }
                        fatready = false;
                        notified = false;
                        cpart = 0;
                         */
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
#if 0
                if (current_state == 0xa0) {
                        printf_P(PSTR("VBUS off\r\n"));
                        // safe to do here
                        Usb.gpioWr(0x00);
                        digitalWrite(2, 0);
                        usbon = false;
                        usbon_time = millis() + 2000;
                        change = false;
                }
#endif
                last_state = current_state;
        }

        // only do any of this if usb is on
        if (usbon) {
                if (partsready && !fatready) {
                        if (cpart > 0) fatready = true;
                }

                // This is horrible, and needs to be moved elsewhere!
                for (int B = 0; B < MAX_DRIVERS; B++) {
                        if (!partsready && Bulk[B]->GetAddress() != NULL) {
                                // Build a list.
                                int ML = Bulk[B]->GetbMaxLUN();
                                //printf("MAXLUN = %i\r\n", ML);
                                ML++;
                                for (int i = 0; i < ML; i++) {
                                        if (Bulk[B]->LUNIsGood(i)) {
                                                partsready = true;
                                                sto[i].private_data = &info[i];
                                                info[i].lun = i;
                                                info[i].B = B;
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
                                                                                Fats[cpart] = new PFAT;
                                                                                int r = Fats[cpart]->Init(&sto[i], cpart, parts[cpart].firstSector);
                                                                                if (r) {
                                                                                        delete Fats[cpart];
                                                                                        Fats[cpart] = NULL;
                                                                                } else cpart++;
                                                                        }
                                                                }
                                                        }
                                                } else {
                                                        // try superblock
                                                        Fats[cpart] = new PFAT;
                                                        int r = Fats[cpart]->Init(&sto[i], cpart, 0);
                                                        if (r) {
                                                                printf_P(PSTR("Superblock error %x\r\n"), r);
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
                        ULONG ii, wt, rt, start, end;

                        if (!notified) {
                                fadeAmount = 5;
                                notified = true;
                                printf_P(PSTR("\r\nOpen an existing file (message.txt).\r\n"));
                                rc = f_open(&My_File_Object_x, "0:/MESSAGE.TXT", FA_READ);
                                if (rc) printf_P(PSTR("Error %i, message.txt not found.\r\n"));
                                else {
                                        printf_P(PSTR("\r\nType the file content.\r\n"));
                                        for (;;) {
                                                rc = f_read(&My_File_Object_x, &(My_Buff_x[0]), mbxs, &br); /* Read a chunk of file */
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
                                printf_P(PSTR("\r\nOpen root directory.\r\n"));
                                rc = f_opendir(&My_Dir_Object_x, "0:/");
                                if (rc) {
                                        die(rc);
                                        goto out;
                                }

                                printf_P(PSTR("\r\nDirectory listing...\r\n"));
                                printf_P(PSTR("Available heap: %u Bytes\r\n"), freeHeap());
                                for (;;) {
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
out:
                                if (rc) die(rc);
                                printf_P(PSTR("\r\nTest completed.\r\n"));

                        }




                        if (runtest) {
                                runtest = false;
                                printf_P(PSTR("\r\nCreate a new 10MB test file (10MB.bin).\r\n"));
                                for (bw = 0; bw < mbxs; bw++) My_Buff_x[bw] = bw & 0xff;
                                rc = f_open(&My_File_Object_x, "0:/10MB.bin", FA_WRITE | FA_CREATE_ALWAYS);
                                if (rc) goto failed;
                                start = millis();
                                for (ii = 10485760 / mbxs; ii > 0; ii--) {
                                        rc = f_write(&My_File_Object_x, &My_Buff_x[0], mbxs, &bw);
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
                                        rc = f_read(&My_File_Object_x, &My_Buff_x[0], mbxs, &bw); /* Read a chunk of file */
                                        if (rc || !bw) break; /* Error or end of file */
                                }
                                end = millis();
                                if (rc) goto failed;
                                rc = f_close(&My_File_Object_x);
                                if (rc) goto failed;
                                rt = end - start;
                                printf_P(PSTR("Time to read 10485760 bytes: %lu ms (%lu sec)\r\nDelete test file\r\n"), rt, (500 + rt) / 1000UL);
failed:
                                rc = f_unlink("0:/10MB.bin");
                                if (rc) die(rc);
                                printf_P(PSTR("10MB timing test finished.\r\n"));
                        }
                }
        }
}
