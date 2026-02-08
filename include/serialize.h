#ifndef __SERIALIZE__
#define __SERIALIZE__

#include <stdint.h>
#include <stdlib.h>

#include "taxicab.h"
#include "types.h"

void get_file_name_identifier(char* const buff, size_t buff_sz, const char* const prefix, const char* const suffix);
void save_taxicabs(const char* const base_file_name, taxicab a, const char* const a_name, taxicab b, const char* const b_name);
void save_pow_m_sqr(const char* const base_file_name, pow_m_sqr M, const char* const M_name);
void save_rels(const char* const base_file_name, da_sets rels, const char* const rels_name);
void save_latin_square(const char* const base_file_name, latin_square P, const char* const P_name);
void save_latin_squares(const char*const base_file_name, latin_square* P, uint32_t r, latin_square* Q, uint32_t s, const char* const name);

void load_taxicabs(const char* const base_file_name, taxicab* a, const char* const a_name, taxicab* b, const char* const b_name);

#endif // __SERIALIZE__

