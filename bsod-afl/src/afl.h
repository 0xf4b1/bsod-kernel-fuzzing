#ifndef FORKSERVER_H
#define FORKSERVER_H

void afl_setup(void);
void afl_rewind(unsigned long start);
void afl_wait(void);
void afl_report(bool crash);
void afl_instrument_location(unsigned long cur_loc);
void afl_instrument_location_block(unsigned long cur_loc);
void afl_instrument_location_edge(unsigned long prev_loc, unsigned long cur_loc);

#endif
