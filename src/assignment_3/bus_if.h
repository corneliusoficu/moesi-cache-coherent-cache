#include <systemc.h>

#include "datatypes.h"

#ifndef BUS_IF_H
#define BUS_IF_H

class Bus_if : public virtual sc_interface
{
    public:
        virtual bool read_probe(int proc_index, int addr) = 0;
        virtual bool write_probe(int proc_index, int addr, bool probe_for_miss) = 0;
        
        virtual bool release_bus_mutex() = 0;
};

#endif