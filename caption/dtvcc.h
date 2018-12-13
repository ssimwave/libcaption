#ifndef LIBCAPTION_DTVCC_H
#define LIBCAPTION_DTVCC_H
#ifdef __cplusplus
extern "C" {
#endif
////////////////////////////////////////////////////////////////////////////////
// code         : the current code being processed
// bytes_left   : number of bytes left to be read
typedef struct {
    unsigned int sequence_number : 2;
    int sequence_count;
    unsigned int seen_sequences : 4;
    unsigned int packet_size : 6;
    unsigned int service_number : 6;
    unsigned int block_size : 5;
    int is_extended_header : 1;
    uint8_t code; 
    int is_ext_code : 1;
    int handle_variable_length_cmd_header : 1;
    int bytes_left;
} dtvcc_packet_t;

// #define DVTCC_SERVICE_NUMBER_UNKNOWN

static inline size_t dtvcc_packet_size_bytes(const dtvcc_packet_t *dtvcc) { return dtvcc-> packet_size == 0 ? 128 : dtvcc->packet_size * 2-1;}

////////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif
#endif
