#ifndef BUS_H
#define BUS_H

#include <systemc.h>

#include "bus_if.h"
#include "cache.h"

class Bus : public Bus_if, public sc_module
{
    public:
        long waits;
        long read_probes_miss;
        long write_probes_hit;
        long write_probes_miss;

        sc_in<bool>        port_clk;        
        sc_signal_rv<32>   port_bus_addr;
        sc_out<BusRequest> port_bus_valid;
        sc_out<int>        port_bus_proc;
        sc_in_rv<1>        port_do_i_have;

        Bus(sc_module_name, int, int);
        ~Bus();

        virtual bool read_probe(int, int);
        virtual bool write_probe(int, int, bool);

        virtual bool release_bus_mutex();
    
    private:
        int id;
        int num_cpus;

        sc_mutex bus_mutex;
};

#endif