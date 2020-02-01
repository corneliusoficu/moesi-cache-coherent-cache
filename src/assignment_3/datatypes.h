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
    STATE_MODIFIED,
    STATE_OWNED,
    STATE_EXCLUSIVE,
    STATE_SHARED,
    STATE_INVALID
};

typedef struct
{
    int data;
    int address;
    LineState line_state;
} DataLookup;

#endif

