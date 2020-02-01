#include "cache.h"

ofstream Log;

Cache::Cache(sc_module_name name_, int id_) : sc_module(name_), id(id_)
{
    initialize_cache_arrays();
    SC_THREAD(snoop);
    sensitive << port_clk.pos();
    dont_initialize();
}

Cache::~Cache() 
{
    for(int index = 0; index < CACHE_NUMBER_OF_SETS; index++)
    {
        delete[] cache[index];
        delete[] tags[index];
    }

    delete[] lru;
    delete[] cache;
    delete[] tags;   
}

void Cache::initialize_cache_arrays()
{
    cache                   = new int*[CACHE_NUMBER_OF_SETS];
    tags                    = new int*[CACHE_NUMBER_OF_SETS]; 
    cache_status            = new LineState*[CACHE_NUMBER_OF_SETS];
    lru                     = new u_int8_t[CACHE_NUMBER_OF_SETS];

    for(int i = 0; i < CACHE_NUMBER_OF_SETS; i++)
    {
        cache[i]                  = new int[CACHE_NUMBER_OF_LINES_IN_SET * CACHE_LINE_SIZE_BYTES];
        tags[i]                   = new int[CACHE_NUMBER_OF_LINES_IN_SET];
        cache_status[i]           = new LineState[CACHE_NUMBER_OF_LINES_IN_SET];

        for(int j = 0; j < CACHE_NUMBER_OF_LINES_IN_SET; j++)
        {
            tags[i][j]         = -1;
            cache_status[i][j] = LineState::STATE_INVALID; 
        }
    }
}

int Cache::create_mask(int start_bit, int end_bit)
{
    int mask = 0;
    for(int i  = start_bit; i <= end_bit; i++)
    {
        mask |= 1 << i;
    }
    return mask;
}

int Cache::get_index_of_line_in_set(int set_index, int tag)
{
    for(int index = 0; index < CACHE_NUMBER_OF_LINES_IN_SET; index++)
    {
        if(cache_status[set_index][index] != LineState::STATE_INVALID && tags[set_index][index] == tag)
        {
            return index;
        }
    }
    return -1;
}

void Cache::update_lru(int set_address, int line_in_set_index)
{
    switch(line_in_set_index)
    {
        case 0:
            lru[set_address] = lru[set_address] | 11;
            break;
        case 1:
            lru[set_address] = lru[set_address] | 3;
            lru[set_address] = lru[set_address] & 119;
            break;
        case 2:
            lru[set_address] = lru[set_address] | 17;
            lru[set_address] = lru[set_address] & 125;
            break;
        case 3:
            lru[set_address] = lru[set_address] | 1;
            lru[set_address] = lru[set_address] & 109;
            break;
        case 4:
            lru[set_address] = lru[set_address] | 36;
            lru[set_address] = lru[set_address] & 126;
            break;                   
        case 5:
            lru[set_address] = lru[set_address] | 4;
            lru[set_address] = lru[set_address] & 94;
            break;
        case 6:
            lru[set_address] = lru[set_address] | 64;
            lru[set_address] = lru[set_address] & 122;
            break;
        case 7:
            lru[set_address] = lru[set_address] & 58;
            break;
        default:
            break;
    }
}

int Cache::get_lru_line(int set_address)
{ 
    for(int i = 0; i < CACHE_NUMBER_OF_LINES_IN_SET; i++)
    {
        if(cache_status[set_address][i] == LineState::STATE_INVALID)
        {
            return i;
        }
    }

    u_int8_t lru_val = lru[set_address];
    int line_index;
    LineState lru_line_state;

    if( (lru_val & 11) == 0 )
    {
        line_index = 0;
    }
    else if( (lru_val & 11) == 8 )
    {
        line_index = 1;
    }
    else if( (lru_val & 19) == 2 )
    {
        line_index = 2;
    }
    else if( (lru_val & 19) == 18 )
    {
        line_index = 3;
    }
    else if( (lru_val & 37) == 1 )
    {
        line_index = 4;
    }
    else if( (lru_val & 37) == 33 )
    {
        line_index = 5;
    }
    else if( (lru_val & 69) == 5 )
    {
        line_index = 6;
    }
    else if( (lru_val & 69) == 69 )
    {
        line_index = 7;
    }
    else
    {
        line_index = -1;
    }

    lru_line_state = cache_status[set_address][line_index];
    
    if(lru_line_state == LineState::STATE_MODIFIED || lru_line_state == LineState::STATE_OWNED || lru_line_state == STATE_EXCLUSIVE)
    {
        wait(100); //Simulate write to main memory
    }

    return line_index;
}

void Cache::extract_address_components(int addr, int *byte_in_line, int *set_address, int *tag)
{
    *byte_in_line = (addr & bit_mask_byte_in_line);     // Obtaining value for bits 0 - 4, no shifting required
    *set_address  = (addr & bit_mask_set_address) >> 5; // Shifting to right to obtain value for bits 5  - 11
    *tag          = (addr & bit_mask_tag)         >> 12;  
}

int Cache::read_value_from_another_cache_or_memory(bool does_copy_exist, int address)
{
    int retrieved_data;
    
    if(port_cache_to_cache.read().to_int() != -1 && does_copy_exist && port_cache_to_cache.read() != "ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ")
    {
        cache_to_cache_transfers++;
        retrieved_data = port_cache_to_cache.read().to_int();
        bus->release_bus_mutex();
        wait();
    }
    else
    {
        bus->release_bus_mutex();
        wait(100); //Simulate memory read
        retrieved_data = rand() % 100;
    }

    return retrieved_data;
}

int Cache::cpu_read(uint32_t addr)
{
    int retrieved_data, byte_in_line, set_address, tag;
    bool does_copy_exist;

    extract_address_components(addr, &byte_in_line, &set_address, &tag);
    int line_in_set_index = get_index_of_line_in_set(set_address, tag);
            
    if(line_in_set_index == -1)
    {
        log(name(), "read miss on address", addr);
        stats_readmiss(id);
                
        does_copy_exist = bus->read_probe(id, addr);
        retrieved_data  = read_value_from_another_cache_or_memory(does_copy_exist, addr);
        
        line_in_set_index = get_lru_line(set_address);
        tags[set_address][line_in_set_index] = tag;
        cache[set_address][line_in_set_index * CACHE_LINE_SIZE_BYTES + byte_in_line] = retrieved_data;
        
        if(does_copy_exist)
        {
            cache_status[set_address][line_in_set_index] = LineState::STATE_SHARED;
        }
        else
        {
            cache_status[set_address][line_in_set_index] = LineState::STATE_EXCLUSIVE;
        }
    }
    else
    {
        log(name(), "read hit on address", addr);
        update_lru(set_address, line_in_set_index);
        stats_readhit(id);
    }
    
    update_lru(set_address, line_in_set_index);
    wait();
    return 0;
}

int Cache::cpu_write(uint32_t addr, uint32_t data)
{
    int byte_in_line, set_address, tag, retrieved_data;
    bool does_copy_exist;
    
    extract_address_components(addr, &byte_in_line, &set_address, &tag);

    int line_in_set_index = get_index_of_line_in_set(set_address, tag);

    if(line_in_set_index == -1)
    {
        log(name(), "write miss on address", addr);
        stats_writemiss(id);

        does_copy_exist = bus->write_probe(id, addr, true);

        retrieved_data = read_value_from_another_cache_or_memory(does_copy_exist, addr);

        line_in_set_index = get_lru_line(set_address);
        cache_status[set_address][line_in_set_index] = LineState::STATE_MODIFIED;
        update_lru(set_address, line_in_set_index);
        tags[set_address][line_in_set_index] = tag;
        cache[set_address][line_in_set_index * CACHE_NUMBER_OF_LINES_IN_SET + byte_in_line] = retrieved_data;
    }
    else
    {
        log(name(), "write hit on address", addr);
        stats_writehit(id);
    
        if(cache_status[set_address][line_in_set_index] == LineState::STATE_SHARED || 
           cache_status[set_address][line_in_set_index] == LineState::STATE_OWNED)
        {
            bus->write_probe(id, addr, false);
        }
        
        cache_status[set_address][line_in_set_index] = LineState::STATE_MODIFIED;
        update_lru(set_address, line_in_set_index);
        cache[set_address][line_in_set_index * CACHE_NUMBER_OF_LINES_IN_SET + byte_in_line] = data;
    }

    wait();

    return 0;
}

void Cache::snoop()
{
    while(can_snoop)
    {
        wait(port_bus_valid.value_changed_event());
        
        int snooped_address     = port_bus_addr.read().to_int();
        BusRequest request_type = port_bus_valid.read();
        int proc_index          = port_bus_proc.read();

        port_cache_to_cache.write("ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ");
        port_do_i_have.write("Z");
        port_provider.write("ZZZ");

        handle_snooped_value(snooped_address, request_type, proc_index); 
    }
}

void Cache::handle_snooped_value(int snooped_address, BusRequest request_type, int proc_index)
{

    DataLookup matched_data;

    matched_data.data       = -1;
    matched_data.line_state = LineState::STATE_INVALID;

    switch(request_type)
    {   
        case READ_PROBE:
        case WRITE_PROBE_HIT:
        case WRITE_PROBE_MISS:
            if(proc_index != id)
            {
                matched_data = coherence_lookup(snooped_address, request_type);
            }
            break;
        case INVALID:
            break;
        default:
            break;
    }

    if(matched_data.data != -1)
    {
        if(matched_data.line_state == LineState::STATE_MODIFIED || 
           matched_data.line_state == LineState::STATE_OWNED || 
           matched_data.line_state == LineState::STATE_EXCLUSIVE)
        {
            port_cache_to_cache.write(matched_data.data);
            port_do_i_have.write(1);
            port_provider.write(id);
        }
        else if(matched_data.line_state == LineState::STATE_SHARED)
        {
            port_do_i_have.write(1);
        }
    }

}

DataLookup Cache::coherence_lookup(int snooped_address, BusRequest bus_request)
{
    int byte_in_line, set_address, tag;
    extract_address_components(snooped_address, &byte_in_line, &set_address, &tag);

    bool is_found = false;

    DataLookup found_data;

    found_data.address    = -1;
    found_data.data       = -1;
    found_data.line_state = LineState::STATE_INVALID;

    for(int line_index = 0; line_index < CACHE_NUMBER_OF_LINES_IN_SET; line_index++)
    {
        if(cache_status[set_address][line_index] != LineState::STATE_INVALID && tags[set_address][line_index] == tag && is_found == false)
        {
            found_data.line_state = cache_status[set_address][line_index];
            found_data.data       = cache[set_address][line_index * CACHE_NUMBER_OF_LINES_IN_SET + byte_in_line];
            found_data.address    = snooped_address;

            switch(bus_request)
            {
                case BusRequest::READ_PROBE:
                    if(found_data.line_state == LineState::STATE_MODIFIED || found_data.line_state == LineState::STATE_EXCLUSIVE)
                    {
                        cache_status[set_address][line_index] = LineState::STATE_OWNED;
                    }
                    break;
                case BusRequest::WRITE_PROBE_MISS:
                    cache_status[set_address][line_index] = LineState::STATE_INVALID;
                    break;
                case BusRequest::WRITE_PROBE_HIT:
                    cache_status[set_address][line_index] = LineState::STATE_INVALID;
                    found_data.data = -1;
                    break;
                case BusRequest::INVALID:
                    break;
            }

            is_found = true;   
        }
    }

    return found_data;

}