#ifndef DATATYPES_H
#define DATATYPES_H

enum BusRequest
{
    INVALID,
    READ,
    WRITE,
    READX
};

enum LineState
{
    STATE_MODIFIED,
    STATE_OWNED,
    STATE_EXCLUSIVE,
    STATE_SHARED,
    STATE_INVALID
};

typedef struct
{
    sc_mutex   access_mutex;
    int        address;
    BusRequest request_type;
} RequestContent;

#endif

