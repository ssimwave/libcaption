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
#ifndef LIBCAPTION_H
#define LIBCAPTION_H
#ifdef __cplusplus
extern "C" {
#endif

#include "eia608.h"
#include "utf8.h"
#include "xds_data.h"
#include "dtvcc.h"

// ssize_t is POSIX and does not exist on Windows
#if defined(_MSC_VER)
#if defined(_WIN64)
typedef signed long ssize_t;
#else
typedef signed int ssize_t;
#endif
#endif

typedef enum {
    LIBCAPTION_ERROR = 0,
    LIBCAPTION_OK = 1,
    LIBCAPTION_READY = 2
} libcaption_status_t;

static inline libcaption_status_t libcaption_status_update(libcaption_status_t old_stat, libcaption_status_t new_stat)
{
    return (LIBCAPTION_ERROR == old_stat || LIBCAPTION_ERROR == new_stat) ? LIBCAPTION_ERROR : (LIBCAPTION_READY == old_stat) ? LIBCAPTION_READY : new_stat;
}

#define SCREEN_ROWS 15
#define SCREEN_COLS 32

typedef struct {
    unsigned int uln : 1; //< underline
    unsigned int sty : 3; //< style
    utf8_char_t data[5]; //< 4 byte utf8 values plus null term
} caption_frame_cell_t;

typedef struct {
    caption_frame_cell_t cell[SCREEN_ROWS][SCREEN_COLS];
} caption_frame_buffer_t;

typedef struct {
    unsigned int uln : 1; //< underline
    unsigned int sty : 3; //< style
    unsigned int rup : 2; //< roll-up line count minus 1
    int8_t row, col;
    uint16_t cc_data;
    dtvcc_packet_t dtvcc_packet;
} caption_frame_state_t;

typedef enum {
    cc_type_ntsc_cc_field_1 = 0,
    cc_type_ntsc_cc_field_2 = 1,
    cc_type_dtvcc_packet_data = 2,
    cc_type_dtvcc_packet_header = 3,
} cea708_cc_type_t;


typedef enum {
  // 608 and common
  LIBCAPTION_DETAIL_OFF_SCREEN              = 1 << 1,
  LIBCAPTION_DETAIL_DUPLICATE_CONTROL       = 1 << 2,
  LIBCAPTION_DETAIL_UNKNOWN_COMMAND         = 1 << 3,
  LIBCAPTION_DETAIL_INVALID_CHARACTER       = 1 << 4,
  LIBCAPTION_DETAIL_PARITY_ERROR            = 1 << 5,
  LIBCAPTION_DETAIL_ABNORMAL_PACKET         = 1 << 6,
  LIBCAPTION_DETAIL_UNKNOWN_TEXT_ATTRIBUTE  = 1 << 7,
  LIBCAPTION_DETAIL_INVALID_EXT_CHARACTER   = 1 << 8,
  LIBCAPTION_DETAIL_ROLLUP_OOS_ERROR        = 1 << 9,
  LIBCAPTION_DETAIL_ROLLUP_MISSING_ERROR    = 1 << 10,
  LIBCAPTION_DETAIL_ROLLUP_ERROR            = 1 << 11,
  LIBCAPTION_DETAIL_POPON_OOS_ERROR         = 1 << 12,
  LIBCAPTION_DETAIL_POPON_MISSING_ERROR     = 1 << 13,
  LIBCAPTION_DETAIL_POPON_ERROR             = 1 << 14,

  LIBCAPTION_XDS_INVALID_CHARACTERS         = 1 << 15,
  LIBCAPTION_XDS_CHECKSUM_ERROR             = 1 << 16,
  LIBCAPTION_XDS_INVALID_PKT_STRUCTURE      = 1 << 17,

  // 708
  LIBCAPTION_DETAIL_SEQUENCE_DISCONTINUITY  = 1 << 18,
  LIBCAPTION_DETAIL_ABNORMAL_SERVICE_BLOCK  = 1 << 19,
  LIBCAPTION_DETAIL_ABNORMAL_CONTROL_CODE   = 1 << 20,
  LIBCAPTION_DETAIL_ABNORMAL_WINDOW_POSITION= 1 << 21,
  LIBCAPTION_DETAIL_ABNORMAL_WINDOW_SIZE    = 1 << 22,
  LIBCAPTION_DETAIL_ABNORMAL_CHARACTER      = 1 << 23,
  LIBCAPTION_DETAIL_DTVCC_PACKING_MISMATCH  = 1 << 24

} caption_frame_status_detail_type;

typedef struct {
  int types;
  int num_services_708;
  int packetErrors;
  int packetLoss;
  unsigned int hasCEA608 : 1;
  unsigned int hasCEA708 : 1;
} caption_frame_status_detail_t;

static inline int status_detail_is_set(const caption_frame_status_detail_t* d, const caption_frame_status_detail_type t) {
  return d->types & t;
}

static inline void status_detail_set(caption_frame_status_detail_t* d, const caption_frame_status_detail_type t) {
  d->types |= t;
}

void status_detail_init(caption_frame_status_detail_t* d);

// timestamp and duration are in seconds
typedef struct {
    double timestamp;
    xds_t xds;
    caption_frame_state_t state;
    caption_frame_buffer_t front;
    caption_frame_buffer_t back;
    caption_frame_buffer_t* write;
    libcaption_status_t status;
    caption_frame_status_detail_t detail;
} caption_frame_t;

/*!
    \brief Initializes an allocated caption_frame_t instance
    \param frame Pointer to prealocated caption_frame_t object
*/
void caption_frame_init(caption_frame_t* frame);
/*! \brief
    \param
*/
static inline int caption_frame_popon(caption_frame_t* frame) { return (frame->write == &frame->back) ? 1 : 0; }
/*! \brief
    \param
*/
static inline int caption_frame_painton(caption_frame_t* frame) { return (frame->write == &frame->front) ? 1 : 0; }
/*! \brief
    \param
*/
const static int _caption_frame_rollup[] = { 0, 2, 3, 4 };
static inline int caption_frame_rollup(caption_frame_t* frame) { return _caption_frame_rollup[frame->state.rup]; }
/*! \brief
    \param
*/
static inline double caption_frame_timestamp(caption_frame_t* frame) { return frame->timestamp; }
/*! \brief Writes a single charcter to a caption_frame_t object
    \param frame A pointer to an allocted and initialized caption_frame_t object
    \param row Row position to write charcter, must be between 0 and SCREEN_ROWS-1
    \param col Column position to write charcter, must be between 0 and SCREEN_ROWS-1
    \param style Style to apply to charcter
    \param underline Set underline attribute, 0 = off any other value = on
    \param c pointer to a single valid utf8 charcter. Bytes are automatically determined, and a NULL terminator is not required
*/
int caption_frame_write_char(caption_frame_t* frame, int row, int col, eia608_style_t style, int underline, const utf8_char_t* c);
/*! \brief
    \param
*/
const utf8_char_t* caption_frame_read_char(caption_frame_t* frame, int row, int col, eia608_style_t* style, int* underline);

enum POPON_CMD_SEQ {
    RCL = 1,
    ENM,
    PAC,
    TOFF,
    EDM,
    EOC
};

enum ROLLUP_CMD_SEQ {
    RU123 = 1,
    CR,
    PACR
};

/**
 * Pop-on captions are delivered through a sequence of commands.
 * The state machine, through state transitions, captures whether a
 * command is out of sequence or is missing.
 * A missing command automatically leads to out of sequence command error.
 */
typedef struct popon_state_machine popon_state_machine;
struct popon_state_machine {
    int cur_state;
    int next_state;
    int rcl;
    int enm;
    int pac;
    int toff;
    int edm;
    int eoc;
    int oos_error;
    int missing_error;
};

typedef struct rollup_state_machine rollup_state_machine;
struct rollup_state_machine {
    int cur_state;
    int next_state;
    int ru123;
    int cr;
    int pac;
    int oos_error;
    int missing_error;
};

void init_rsm(rollup_state_machine *rsm);
void update_rsm(caption_frame_status_detail_t* details, eia608_control_t cmd, int pac,
                rollup_state_machine *rsm);
void init_psm(popon_state_machine *psm);
void update_psm(caption_frame_status_detail_t* details, eia608_control_t cmd, int pac,
                popon_state_machine *psm);
/*! \brief
    \param
*/
libcaption_status_t caption_frame_decode(caption_frame_t* frame, uint16_t cc_data, double timestamp,
                                         rollup_state_machine* rsm, popon_state_machine* psm,
                                         cea708_cc_type_t type);

/*! \brief
    \param
*/
libcaption_status_t caption_frame_decode_dtvcc(caption_frame_t* frame, uint16_t cc_data, double timestamp, cea708_cc_type_t type);
/*! \brief
    \param
*/
int caption_frame_from_text(caption_frame_t* frame, const utf8_char_t* data);
/*! \brief
    \param
*/
#define CAPTION_FRAME_TEXT_BYTES (4 * ((SCREEN_COLS + 2) * SCREEN_ROWS) + 1)
size_t caption_frame_to_text(caption_frame_t* frame, utf8_char_t* data);
/*! \brief
    \param
*/
#define CAPTION_FRAME_DUMP_BUF_SIZE 8192
size_t caption_frame_dump_buffer(caption_frame_t* frame, utf8_char_t* buf);
void caption_frame_dump(caption_frame_t* frame);

#ifdef __cplusplus
}
#endif
#endif
