/* routines etc. in loader.h: */

extern bool load_aout(char* name);
extern bool load_binary(char* name, address* addr, int offset, int evenodd);
extern bool load_elf(char* name);
extern bool load_intel(char* name);
extern bool load_motorola(char* name);
extern bool load_shf(char* name);
extern bool load_tektronix(char* name);

extern bool write_aout(char* filename, address* range);
extern bool write_binary(char* filename, address* range);
extern bool write_elf(char* filename, address* range);
extern bool write_intel(char* filename, address* range);
extern bool write_motorola(char* filename, address* range);
extern bool write_shf(char* filename, address* range);
extern bool write_tektronix(char* filename, address* range);
