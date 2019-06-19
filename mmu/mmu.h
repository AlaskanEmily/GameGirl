#ifndef GG_MMU_H
#define GG_MMU_H

#if (defined __GNUC__) || (defined __TINYC__)
#define GG_STDCALL(T) __attribute__((stdcall)) T
#elif (defined _MSC_VER) || (defined __WATCOMC__)
#define GG_STDCALL(T) T __stdcall
#else
#error Add stdcall for your compiler here.
#endif

#ifdef __cplusplus
#define GG_MMU_FUNC(T) extern "C" GG_STDCALL(T)
#else
#define GG_MMU_FUNC GG_STDCALL
#endif

union GG_MMU_u;
typedef union GG_MMU_u GG_MMU;
typedef GG_MMU *GG_MMU_ptr;

#ifdef __cplusplus
extern "C" {
#endif
extern const unsigned gg_mmu_struct_size;
extern const unsigned _gg_mmu_struct_size;
#ifdef __cplusplus
} // extern "C"
#endif

/* Equivalent of GG_InitMMU(malloc(gg_mmu_struct_size))
 * This may use a different allocator though, so it should ONLY ever
 * be paired with GG_DestroyMMU
 */
GG_MMU_FUNC(GG_MMU_ptr) GG_CreateMMU(void);

/* Equivalent of free(GG_FiniMMU(mmuptr))
 * This may use a different allocator though, so it should ONLY ever
 * be paired with GG_CreateMMU
 */
GG_MMU_FUNC(void) GG_DestroyMMU(GG_MMU *);

/* Initializes an MMU struct. */
GG_MMU_FUNC(GG_MMU_ptr) GG_InitMMU(GG_MMU *);

/* Finalizes an MMU struct. */
GG_MMU_FUNC(GG_MMU_ptr) GG_FiniMMU(GG_MMU *);

GG_MMU_FUNC(void) GG_SetMMURom(GG_MMU *, const void *rom, unsigned len);

GG_MMU_FUNC(unsigned) GG_Read8MMU(const GG_MMU *mmu, unsigned i);
GG_MMU_FUNC(unsigned) GG_Read16MMU(const GG_MMU *mmu, unsigned i);

GG_MMU_FUNC(unsigned) GG_Inc8MMU(GG_MMU *mmu, unsigned i);
GG_MMU_FUNC(unsigned) GG_Dec8MMU(GG_MMU *mmu, unsigned i);

GG_MMU_FUNC(void) GG_Write8MMU(GG_MMU *mmu, unsigned i, unsigned val);
GG_MMU_FUNC(void) GG_Write16MMU(GG_MMU *mmu, unsigned i, unsigned val);

#endif /* GG_MMU_H */
