#include "bus.h"

Bus::Bus(sc_module_name name_, int id_, int num_cpus_) : sc_module(name_), id(id_), num_cpus(num_cpus_)
{
    sensitive << port_clk.pos();

    port_bus_addr.write("ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ");

    waits             = 0;
    read_probes_miss  = 0;
    write_probes_hit  = 0;
    write_probes_miss = 0;
    consistency_waits = 0;
}

Bus::~Bus()
{
    
}

bool Bus::read_probe(int proc_index, int addr)
{

    while(bus_mutex.trylock() == -1)
    {
        waits++;
        wait();
    }

    read_probes_miss++;

    log(name(), "read for address", addr, "proc index", proc_index);

    port_bus_addr.write(addr);
    port_bus_proc.write(proc_index);
    port_bus_valid.write(BusRequest::READ_PROBE);

    wait();

    if(port_do_i_have.read() == 1)
    {
        return true;
    }

    return false;
}   

bool Bus::write_probe(int proc_index, int addr, bool probe_for_miss)
{
    while(bus_mutex.trylock() == -1)
    {
        waits++;
        wait();
    }

    log(name(), "readx for address", addr, "proc index", proc_index);
    
    port_bus_addr.write(addr);
    port_bus_proc.write(proc_index);
    
    if(probe_for_miss == false)
    {
        port_bus_valid.write(BusRequest::WRITE_PROBE_HIT);
        write_probes_hit++;
        return true;
    }

    port_bus_valid.write(BusRequest::WRITE_PROBE_MISS);
    write_probes_miss++;
    
    if(port_do_i_have.read() == 1)
    {
        return true;
    }

    return false;
}

bool Bus::release_bus_mutex()
{
    port_bus_valid.write(BusRequest::INVALID);
    port_bus_addr.write("ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ");

    bus_mutex.unlock();
    return true;
}