/**********************************************************************************************/
/* The MIT License                                                                            */
/*                                                                                            */
/* Copyright 2016-2017 Twitch Interactive, Inc. or its affiliates. All Rights Reserved.       */
/*                                                                                            */
/* Permission is hereby granted, free of charge, to any person obtaining a copy               */
/* of this software and associated documentation files (the "Software"), to deal              */
/* in the Software without restriction, including without limitation the rights               */
/* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell                  */
/* copies of the Software, and to permit persons to whom the Software is                      */
/* furnished to do so, subject to the following conditions:                                   */
/*                                                                                            */
/* The above copyright notice and this permission notice shall be included in                 */
/* all copies or substantial portions of the Software.                                        */
/*                                                                                            */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR                 */
/* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,                   */
/* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE                */
/* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER                     */
/* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,              */
/* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN                  */
/* THE SOFTWARE.                                                                              */
/**********************************************************************************************/
// http://www.theneitherworld.com/mcpoodle/SCC_TOOLS/DOCS/CC_XDS.HTML#PR
#include "xds.h"
#include "caption.h"
#include <string.h>
#include <xds.h>
#include <caption.h>
#include <xds_data.h>

int is_valid_type(uint8_t class_code, uint8_t type) {
    if (type == 0) {
        return 0;
    }
    switch(class_code) {
        case CURRENT: // valid types are 0x01 to 0x17 (ANSI-CTA-608-E R-2014 9.5.1)
            return type <= 0x17;
        case FUTURE: // valid types are 0x01 to 0x17 (ANSI-CTA-608-E R-2014 9.5.2)
            return type <= 0x17;
        case CHANNEL: // valid types are 0x01 to 0x04 (ANSI-CTA-608-E R-2014 9.5.3)
            return type <= 0x04;
        case MISC: // valid types are 0x01 to 0x04, and 0x40 to 0x43 (ANSI-CTA-608-E R-2014 9.5.4)
            return type <= 0x04 || (type >= 0x40 && type <= 0x43);
        case PUBLIC_SERVICE: // valid types are 0x01 to 0x02 (ANSI-CTA-608-E R-2014 9.5.5)
            return type <= 0x02;
        case CLASS_RESERVED: // used arbitrarily by xds encoding equipment (ANSI-CTA-608-E R-2014 9.5)
            return 1;
        case PRIVATE_DATA: // used arbitrarily (ANSI-CTA-608-E R-2014 9.6)
            return 1;
        default:
            return 0;
    }
}

void clear_packet_info(xds_packet_t* packet) {
    memset(packet, 0, sizeof(xds_packet_t));
}

void xds_init(xds_t* xds)
{
    memset(xds, 0, sizeof(xds_t));
}

libcaption_status_t xds_decode(caption_frame_t* frame, uint16_t cc)
{
    xds_t* xds = &frame->xds;

    switch (xds->state) {
        default:
        case 0: // A control code was seen after a non-XDS data stream
            xds->state = 1;
            uint8_t control_code = (cc & 0x0F00) >> 8;
            uint8_t type_code = (cc & 0x007F);

            // specifications don't cover control code of 0
            // so switching state back to 0 (might be wrong behaviour)
            if (control_code == 0) {
                xds->state = 0;
                status_detail_set(&frame->detail, LIBCAPTION_XDS_INVALID_PKT_STRUCTURE);
                return LIBCAPTION_ERROR;
            }

            // if start code
            if (control_code % 2 != 0) {
                // delete old information and initialize values
                xds_packet_t *packet = &xds->packets[(control_code - 1) / 2];
                clear_packet_info(packet);
                packet->class_code = control_code;

                if (!is_valid_type(control_code, type_code)) {
                    status_detail_set(&frame->detail, LIBCAPTION_XDS_INVALID_PKT_STRUCTURE);
                    return LIBCAPTION_ERROR;
                }
                packet->type_code = type_code;
                packet->size = 0;

                xds->active_class_index = (control_code - 1) / 2;
            } else { // if continue code
                if (xds->packets[(control_code / 2) - 1].class_code == control_code + 1) {
                    // set class type
                    xds->active_class_index = (control_code / 2) - 1;
                } else {
                    // this packet class was inactive, and continue code is meaningless
                    status_detail_set(&frame->detail, LIBCAPTION_XDS_INVALID_PKT_STRUCTURE);
                    return LIBCAPTION_ERROR;
                }
            }

            return LIBCAPTION_OK;

        case 1: // in the middle of an XDS data stream
            // check if the packet is interrupted in a correct way
            if ((cc & 0xF000) == 0x1000) {
                xds->state = 0;
                return LIBCAPTION_OK;
            }

            if(xds->active_class_index > 6) {
                return LIBCAPTION_ERROR;
            }

            xds_packet_t *packet = &xds->packets[xds->active_class_index];

            //check for ending control sequence
            if ((cc & 0xFF00) == 0x8F00) {
                packet->checksum = (cc & 0x007F);
                xds->state = 0;

                //From (ANSI-CTA-608-E R-2014 8.6.3):
                //Expected checksum defined as
                //"the [7-bit] two's complement of the sum of the informational characters
                //plus the Start, Type and End characters"
                //Characters
                uint8_t calculated_checksum = packet->class_code + packet->type_code + 0xF;

                for(int i = 0; i < packet->size; i++) {
                    calculated_checksum += packet->content[i];
                }

                //convert to 2s complement by flipping all bits and adding 1 and taking 1st 7 bits
                calculated_checksum = (~calculated_checksum + 1) & 0x7f;

                if(calculated_checksum != packet->checksum) {
                    status_detail_set(&frame->detail, LIBCAPTION_XDS_CHECKSUM_ERROR);
                    return LIBCAPTION_ERROR;
                } else {
                    return LIBCAPTION_READY;
                }
            }

            //since these are not control sequences, it must be information so
            //we can discard the first byte (ANSI-CTA-608-E R-2014 8.6.1)
            uint8_t char_1 = (cc & 0x7F00) >> 8;
            uint8_t char_2 = cc & 0x007F;

            //check that the information characters are between defined limits at (ANSI-CTA-608-E R-2014 8.6.1)
            if(!((char_1 == 0 || char_1 >= 0x20) &&
                 (char_2 == 0 || char_2 >= 0x20))) {
                status_detail_set(&frame->detail, LIBCAPTION_XDS_INVALID_CHARACTERS);
                return LIBCAPTION_ERROR;
            }

            if (packet->size >= 32) { //fail because payload larger than standard allows (ANSI-CTA-608-E R-2014 8.6.1)
                status_detail_set(&frame->detail, LIBCAPTION_XDS_INVALID_PKT_STRUCTURE);
                return LIBCAPTION_ERROR;
            }

            //update position of payload and save state
            packet->content[packet->size + 0] = char_1;
            packet->content[packet->size + 1] = char_2;
            packet->size += 2;

            return LIBCAPTION_OK;
    }
}
