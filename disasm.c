#include "dbg/dbg.h"
#include "mmu/mmu.h"

#include <stdio.h>
#include <stdlib.h>

/* Standalone disassembler */
#if (defined _WIN32) || (defined WIN32) || (defined __CYGWIN__)
#include "bufferfile_win32.c"
#else
#include "bufferfile_unix.c"
#endif


int main(int argc, char **argv){
    int size;
    const void *data;
    
    int print_addresses = 1;
    int print_raw = 1;
    const char *path = NULL;
    
    for(size = 1; size < argc; size++){
        if(argv[size][0] == '-'){
            int i = 1;
            char c;
            if(argv[size][1] == 0){
                puts("Empty option");
                return 1;
            }
            
            while((c = argv[size][i++]) != 0){
                switch(c){
                    case 'a':
                        print_addresses = 0;
                        break;
                    case 'r':
                        print_raw = 0;
                        break;
                    default:
                        printf("Unknown option %c\n", c);
                        return 1;
                }
            }
        }
        else{
            if(path != NULL){
                puts("Too many files");
                return 1;
            }
            path = argv[size];
        }
    }
    
    if(path == NULL){
        puts("Usage: gg_disasm <file> [-a] [-r]");
        puts("    -a  Do not print addresses");
        puts("    -r  Do not print raw bytes");
        return 1;
    }
    
    data = BufferFile(path, &size);
    
    if(data == NULL || size == 0){
        printf("Could not open file %s\n", path);
        return 1;
    }
    
    {
        char buffer[80];
        unsigned address = 0;
        GG_MMU *mmu = GG_CreateMMU();
        GG_SetMMURom(mmu, data, size);
        
        do{
            unsigned start_address = address;
            const char *line = GG_DebugDisassemble(mmu, &address, buffer);
            int p = 0;
            
            if(print_addresses)
                p = printf("0x%0.4X ", start_address);
            
            p += printf("%s", line);
            
            if(print_raw){
                while(p++ < 31){
                    printf(" ");
                }
                do{
                    printf(" 0x%0.2X", GG_Read8MMU(mmu, start_address++));
                }while(start_address < address);
            }
            puts(" ");
        }while(address < size);
        
    }
    
    return 0;
}
