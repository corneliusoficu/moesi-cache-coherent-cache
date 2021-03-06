#ifndef CACHE_H
#define CACHE_H

#include <systemc.h>

#include "psa.h"
#include "cache_if.h"
#include "bus_if.h"
#include "helpers.h"

#define CACHE_LINE_SIZE_BYTES        32
#define CACHE_NUMBER_OF_LINES_IN_SET 8
#define CACHE_NUMBER_OF_SETS         128

class Cache : public cache_if, public sc_module
{

public:
    static long cache_to_cache_transfers;
    static long memory_read_accesses;
    static long memory_write_accesses;

    sc_in<bool>       port_clk;
    sc_port<Bus_if>   bus;
    sc_in_rv<32>      port_bus_addr;
    sc_in<int>        port_bus_proc;
    sc_in<BusRequest> port_bus_valid;
    sc_inout_rv<32>   port_cache_to_cache;
    sc_inout_rv<1>    port_do_i_have;
    sc_out_rv<3>      port_provider;

    bool can_snoop;

    const char *line_state_names[5] = {"MODIFIED", "OWNED", "EXCLUSIVE", "SHARED", "INVALID"};
    
    Cache(sc_module_name, int);
    ~Cache();

    SC_HAS_PROCESS(Cache);

    int cpu_read(uint32_t addr);
    int cpu_write(uint32_t addr, uint32_t data);

private:
    int id;

    int       **cache;
    int       **tags;
    LineState **cache_status;
    u_int8_t   *lru;

    int bit_mask_byte_in_line = create_mask(0,  4);
    int bit_mask_set_address  = create_mask(5, 11);
    int bit_mask_tag          = create_mask(12, 31);

    void extract_address_components(int, int*, int*, int*);
    void initialize_cache_arrays();
    int  create_mask(int, int);
    int  get_index_of_line_in_set(int, int);
    void update_lru(int, int);
    int  get_lru_line(int);
    void handle_cache_read(int, int, int);
    void handle_cache_write(int, int, int, int, int);
    void execute();
    void snoop();
    void handle_snooped_value(int, BusRequest, int);
    void invalidate_cache_copy(int);

    DataLookup coherence_lookup(int, BusRequest);
    int  read_value_from_another_cache_or_memory(bool, int);
};

#endif