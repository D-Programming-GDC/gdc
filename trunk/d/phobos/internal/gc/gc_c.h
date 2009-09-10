enum DataSegmentTracking {
    ExecutableOnly,
    LoadTimeLibrariesOnly,
    Dynamic
};
extern void _D3std2gc8addRangeFPvPvZv(void *, void *);
extern void _D3std2gc11removeRangeFPvZv(void *);
#define GC_add_range(x,y) (_D3std2gc8addRangeFPvPvZv((x),(y)))
#define GC_remove_range(x) (_D3std2gc11removeRangeFPvZv(x))
