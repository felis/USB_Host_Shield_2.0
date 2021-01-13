/* Copyright (C) 2011 Circuits At Home, LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").

Contact information
-------------------

Circuits At Home, LTD
Web      :  http://www.circuitsathome.com
e-mail   :  support@circuitsathome.com
 */

#if !defined(__HIDUNIVERSAL_H__)
#define __HIDUNIVERSAL_H__

#include "hidcomposite.h"

class HIDUniversal : public HIDComposite {

        bool SelectInterface(uint8_t iface __attribute__((unused)), uint8_t proto __attribute__((unused))) final {
                // the original HIDUniversal didn't have this at all so make it a no-op
                // (and made it final so users don't override this - if they want to use
                // SelectInterface() they should be deriving from HIDComposite directly)
                return true;
        }

        void ParseHIDData(USBHID *hid, uint8_t ep __attribute__((unused)), bool is_rpt_id, uint8_t len, uint8_t *buf) final {
                // override the HIDComposite version of this method to call the HIDUniversal version
                // (which doesn't use the endpoint), made it final to make sure users
                // of HIDUniversal override the right version of ParseHIDData() (the other one, below)
                ParseHIDData(hid, is_rpt_id, len, buf);
        }

protected:
        virtual void ParseHIDData(USBHID *hid __attribute__((unused)), bool is_rpt_id __attribute__((unused)), uint8_t len __attribute__((unused)), uint8_t *buf __attribute__((unused))) {
                return;
        }

public:
        HIDUniversal(USB *p) : HIDComposite(p) {}

        uint8_t Poll() override;

        // UsbConfigXtracter implementation
        void EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR *ep) override
        {
                // If the first configuration satisfies, the others are not considered.
                if(bNumEP > 1 && conf != bConfNum)
                        return;
                // otherwise HIDComposite does what HIDUniversal needs
                HIDComposite::EndpointXtract(conf, iface, alt, proto, ep);
        }
};

#endif // __HIDUNIVERSAL_H__
