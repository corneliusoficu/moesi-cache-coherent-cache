#ifndef DATATYPES_H
#define DATATYPES_H

enum BusRequest
{
    INVALID,
    READ_PROBE,
    WRITE_PROBE_MISS,
    WRITE_PROBE_HIT
};

enum LineState
{
    STATE_MODIFIED  = 0,
    STATE_OWNED     = 1,
    STATE_EXCLUSIVE = 2,
    STATE_SHARED    = 3,
    STATE_INVALID   = 4
};

typedef struct
{
    int data;
    int address;
    LineState line_state;
} DataLookup;

#endif

