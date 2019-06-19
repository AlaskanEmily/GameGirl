#include "mmu.h"

#include <string.h>

#ifndef NDEBUG
#include <stdio.h>
#define DEBUG_ONLY(X) X
#else
#define DEBUG_ONLY(X)
#endif

union GG_MMU_u {
    unsigned char mem[0x10000];
    struct {
        const char rom[0x8000];
        char ram[0x8000];
    } rw;
    struct {
        char bank0[0x4000];
        char bank1[0x4000]; /* TODO! */
        char vram[0x2000];
        char extram[0x2000];
        char ram[0x2000]; /* MIRRORED! */
        char ram_mirror[0x1E00]; /* MIRRORED! */
        char sprites[0x100];
        char mmio[0x80];
        char zero[0x80];
    } banks;
};

#if (defined __unix) && (!defined GG_NO_MMAP)

#include <unistd.h>
#include <sys/mman.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

static union GG_MMU_u *gg_alloc_mmu(void){
    void *const r = mmap(NULL,
        sizeof(union GG_MMU_u),
        PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS,
        -1, 0);
    return r;
}

static void gg_dealloc_mmu(const union GG_MMU_u *mmu){
    munmap((void*)mmu, sizeof(union GG_MMU_u));
}

#elif (defined WIN32) || (defined _WIN32)

#include <Windows.h>

static union GG_MMU_u *gg_alloc_mmu(void){
    void *const data = VirtualAlloc(NULL,
        sizeof(union GG_MMU_u),
        MEM_COMMIT|MEM_RESERVE,
        PAGE_READWRITE);
    
    VirtualLock(data, sizeof(union GG_MMU_u));
    
    return data;
}
    
static void gg_dealloc_mmu(const union GG_MMU_u *mmu){
    VirtualFree((void*)mmu, 0, MEM_RELEASE);
}

#else

#include <stdlib.h>
static union GG_MMU_u *gg_alloc_mmu(void){
    return malloc(sizeof(union GG_MMU_u));
}

static void gg_dealloc_mmu(const union GG_MMU_u *mmu){
    free((void*)mmu);
}

#endif

void GG_SetMMURom(GG_MMU *mmu, const void *rom, unsigned len){
    if(len < 0x8000){
        memcpy(mmu->mem, rom, len);
    }
    else{
        memcpy(mmu->mem, rom, 0x8000);
    }
}

const unsigned gg_mmu_struct_size = sizeof(union GG_MMU_u);
const unsigned _gg_mmu_struct_size = sizeof(union GG_MMU_u);

GG_MMU *GG_CreateMMU(void){
    return GG_InitMMU(gg_alloc_mmu());
}

void GG_DestroyMMU(GG_MMU *mmu){
    gg_dealloc_mmu(GG_FiniMMU(mmu));
}

GG_MMU *GG_InitMMU(GG_MMU *mmu){
    (void)mmu;
    return mmu;
}

GG_MMU *GG_FiniMMU(GG_MMU *mmu){
    return mmu;
}

unsigned GG_Read8MMU(const GG_MMU *mmu, unsigned i){
    return (unsigned)(mmu->mem[i]);
}

#if (defined __i386) || (defined _M_IX86) || (defined __x86_64__)

#define GG_WRITE16(TO, VAL) do{ \
        ((unsigned short*)(TO))[0] = (VAL); \
    } while(0)

#define GG_READ16(FRM) (0+(*((unsigned short*)(FRM))))

#else

#define GG_WRITE16(TO, VAL) do{ \
        ((char*)(TO))[0] = (VAL); \
        ((char*)(TO))[1] = (VAL) << 8; \
    } while(0)

#define GG_READ16(FRM) ( \
        ((unsigned char*)(FRM))[0] | \
        ((unsigned char*)(FRM))[1] \
    )

#endif

unsigned GG_Read16MMU(const GG_MMU *mmu, unsigned i){
    const char *mem = mmu->mem;
    return GG_READ16(mem+i);
}

#define GG_RAM_IS_LO_MIRROR(SI) ((SI) >= 0x4000 && (SI) < 0x5E00)
#define GG_RAM_IS_HI_MIRROR(SI) ((SI) >= 0x6000 && (SI) < 0x7E00)
#define GG_RAM_LO(MMU, SI) ((MMU)->banks.ram[((SI) - 0x6000)])
#define GG_RAM_HI(MMU, SI) ((MMU)->banks.ram_mirror[((SI) - 0x4000)])

#define GG_RAM_WRITE8(MMU, I, VAL) do{ \
        unsigned GG_val; \
        const int GG_si = (I) - 0x8000; \
        union GG_MMU_u *const GG_mmu = (MMU); \
        \
        if(GG_si < 0) break; \
        \
        GG_val = (VAL); \
        \
        GG_mmu->rw.ram[GG_si] = GG_val; \
        if(GG_RAM_IS_LO_MIRROR(GG_si)){ \
            GG_RAM_HI(GG_mmu, GG_si) = GG_val; \
        } \
        else if(GG_RAM_IS_HI_MIRROR(GG_si)){ \
            GG_RAM_LO(GG_mmu, GG_si) = GG_val; \
        } \
    }while(0)

#define GG_RAM_WRITE16(MMU, I, VAL) do{ \
        unsigned GG_val; \
        const int GG_si = (I) - 0x8000; \
        union GG_MMU_u *const GG_mmu = (MMU); \
        \
        if(GG_si < 0) break; \
        \
        GG_val = (VAL); \
        \
        GG_mmu->rw.ram[GG_si] = GG_val; \
        GG_mmu->rw.ram[GG_si+1] = GG_val >> 8; \
        if(GG_RAM_IS_LO_MIRROR(GG_si)){ \
            GG_WRITE16(&GG_RAM_HI(GG_mmu, GG_si), GG_val); \
        } \
        else if(GG_RAM_IS_HI_MIRROR(GG_si)){ \
            GG_WRITE16(&GG_RAM_LO(GG_mmu, GG_si), GG_val); \
        } \
    }while(0)

unsigned GG_Inc8MMU(GG_MMU *mmu, unsigned i){
    unsigned result = mmu->mem[i];
    GG_RAM_WRITE8(mmu, i, ++result);
    return result;
}

unsigned GG_Dec8MMU(GG_MMU *mmu, unsigned i){
    unsigned result = mmu->mem[i];
    GG_RAM_WRITE8(mmu, i, --result);
    return result;
}

void GG_Write8MMU(GG_MMU *mmu, unsigned i, unsigned val){
    GG_RAM_WRITE8(mmu, i, val);
}

/* TODO! This needs better checks on the borders of ram areas. */
void GG_Write16MMU(GG_MMU *mmu, unsigned i, unsigned val){
    GG_RAM_WRITE8(mmu, i, val);
}

