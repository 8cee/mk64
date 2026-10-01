#include <android/log.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

extern "C" {
#include <ultra64.h>
void mio0decode(u8* in, u8* out);
void displaylist_unpack(uintptr_t* data, uintptr_t finalDisplaylistOffset, u32 arg2);
extern uintptr_t gHeapEndPtr;
extern unsigned char __assets_start[];
extern unsigned char __assets_end[];
extern const uint8_t gAndroidMk64RecipeBlob[];
extern const size_t gAndroidMk64RecipeBlobSize;
bool mk64_android_pi_read(uintptr_t romAddress, void* destination, size_t size);
}

#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, "MK64Assets", __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, "MK64Assets", __VA_ARGS__)

namespace {
enum { PA_RAW=1, PA_MIO0=2, PA_LITERAL=3, PA_RELOCS=4, PA_UNPACK=5 };
enum { PA_XF_ID=0, PA_XF_SW16=1, PA_XF_SW32=2, PA_XF_PATTERN=16 };
enum { PA_SHAPE_4=0, PA_SHAPE_22=1, PA_SHAPE_211=2, PA_SHAPE_112=3 };

#pragma pack(push,1)
struct Header {
    char magic[8];
    uint32_t region_size, data_crc, recipe_count, literal_size, pattern_count;
    uint32_t course_count, block_count, reloc_count, max_block, max_unpacked;
};
struct Recipe { uint32_t dst,size; uint16_t kind,xform; uint32_t src,extra; };
struct Pattern { uint32_t nwords; uint8_t shapes[16]; };
struct Course { uint32_t course,unpacked_len,rom_off,rom_len,packed_off; };
struct Block { uint32_t rom_off,rom_len,decomp_len; };
struct Reloc { uint32_t off,target; };
#pragma pack(pop)

void sw16(uint8_t* b,size_t n){for(size_t i=0;i+1<n;i+=2)std::swap(b[i],b[i+1]);}
void sw32(uint8_t* b,size_t n){for(size_t i=0;i+3<n;i+=4){std::swap(b[i],b[i+3]);std::swap(b[i+1],b[i+2]);}}
void pattern(uint8_t* b,size_t n,const Pattern& p){
    if(!p.nwords) return;
    for(size_t w=0;w<n/4;w++){
        uint8_t* q=b+w*4; uint8_t t;
        switch(p.shapes[w%p.nwords]){
            case PA_SHAPE_4: t=q[0];q[0]=q[3];q[3]=t;t=q[1];q[1]=q[2];q[2]=t;break;
            case PA_SHAPE_22: t=q[0];q[0]=q[1];q[1]=t;t=q[2];q[2]=q[3];q[3]=t;break;
            case PA_SHAPE_211: t=q[0];q[0]=q[1];q[1]=t;break;
            case PA_SHAPE_112: t=q[2];q[2]=q[3];q[3]=t;break;
            default: break;
        }
    }
}
void apply(uint8_t* b,size_t n,uint16_t xf,const Pattern* pats,uint32_t np){
    if(xf==PA_XF_SW16) sw16(b,n);
    else if(xf==PA_XF_SW32) sw32(b,n);
    else if(xf>=PA_XF_PATTERN){
        uint32_t i=xf-PA_XF_PATTERN;
        if(i<np) pattern(b,n,pats[i]);
    }
}
bool read_rom(uint32_t off,void* dst,size_t n){return mk64_android_pi_read(off,dst,n);}
}

extern "C" bool mk64_android_load_assets() {
    if (gAndroidMk64RecipeBlobSize < sizeof(Header)) return false;
    const auto* h=reinterpret_cast<const Header*>(gAndroidMk64RecipeBlob);
    if(std::memcmp(h->magic,"MK64RCP1",8)!=0){ALOGE("bad recipe magic");return false;}
    const size_t regionSize=static_cast<size_t>(__assets_end-__assets_start);
    if(regionSize!=h->region_size){ALOGE("region mismatch %zu vs %u",regionSize,h->region_size);return false;}

    const uint8_t* p=gAndroidMk64RecipeBlob+sizeof(Header);
    const auto* recs=reinterpret_cast<const Recipe*>(p); p+=h->recipe_count*sizeof(Recipe);
    const uint8_t* literals=p; p+=(h->literal_size+3u)&~3u;
    const auto* pats=reinterpret_cast<const Pattern*>(p); p+=h->pattern_count*sizeof(Pattern);
    const auto* courses=reinterpret_cast<const Course*>(p); p+=h->course_count*sizeof(Course);
    const auto* blocks=reinterpret_cast<const Block*>(p); p+=h->block_count*sizeof(Block);
    const auto* relocs=reinterpret_cast<const Reloc*>(p); p+=h->reloc_count*sizeof(Reloc);
    if(p>gAndroidMk64RecipeBlob+gAndroidMk64RecipeBlobSize){ALOGE("recipe truncated");return false;}

    std::memset(__assets_start,0,regionSize);
    std::vector<uint8_t> comp,dec,tmp;
    uint32_t cachedBlock=0xffffffffu;

    auto loadBlock=[&](uint32_t rom)->bool{
        if(cachedBlock==rom) return true;
        const Block* b=nullptr;
        for(uint32_t i=0;i<h->block_count;i++) if(blocks[i].rom_off==rom){b=&blocks[i];break;}
        if(!b){ALOGE("unknown MIO0 block %08x",rom);return false;}
        comp.resize(b->rom_len);
        dec.resize(b->decomp_len+64);
        if(!read_rom(b->rom_off,comp.data(),comp.size())) return false;
        if(comp.size()<4 || std::memcmp(comp.data(),"MIO0",4)!=0){ALOGE("bad MIO0 block %08x",rom);return false;}
        mio0decode(comp.data(),dec.data());
        cachedBlock=rom;
        return true;
    };

    for(uint32_t i=0;i<h->recipe_count;i++){
        const Recipe& r=recs[i];
        if((uint64_t)r.dst+r.size>regionSize){ALOGE("recipe dst out of range");return false;}
        uint8_t* dst=__assets_start+r.dst;
        if(r.kind==PA_RAW){
            if(r.xform==PA_XF_ID){ if(!read_rom(r.src,dst,r.size)) return false; }
            else {
                size_t n=(r.size+3u)&~3u; tmp.resize(n);
                if(!read_rom(r.src,tmp.data(),n)) return false;
                apply(tmp.data(),n,r.xform,pats,h->pattern_count);
                std::memcpy(dst,tmp.data(),r.size);
            }
        } else if(r.kind==PA_MIO0){
            if(!loadBlock(r.src)) return false;
            if((uint64_t)r.extra+r.size>dec.size()) return false;
            size_t n=(r.size+3u)&~3u; tmp.resize(n);
            std::memcpy(tmp.data(),dec.data()+r.extra,n);
            apply(tmp.data(),n,r.xform,pats,h->pattern_count);
            std::memcpy(dst,tmp.data(),r.size);
        } else if(r.kind==PA_LITERAL){
            if((uint64_t)r.src+r.size>h->literal_size) return false;
            std::memcpy(dst,literals+r.src,r.size);
        }
    }

    uintptr_t savedHeap=gHeapEndPtr;
    for(uint32_t ci=0;ci<h->course_count;ci++){
        const Course& c=courses[ci];
        comp.resize(c.rom_len);
        if(!read_rom(c.rom_off,comp.data(),comp.size())){gHeapEndPtr=savedHeap;return false;}
        std::vector<uint8_t> unpack(((c.unpacked_len+15u)&~15u)+72u);
        gHeapEndPtr=reinterpret_cast<uintptr_t>(unpack.data())+((c.unpacked_len+15u)&~15u)+8u;
        displaylist_unpack(reinterpret_cast<uintptr_t*>(comp.data()+c.packed_off),c.unpacked_len,0);
        for(uint32_t i=0;i<h->recipe_count;i++){
            const Recipe& r=recs[i];
            if(r.kind==PA_UNPACK && r.src==c.course){
                if((uint64_t)r.extra+r.size>unpack.size()){gHeapEndPtr=savedHeap;return false;}
                std::memcpy(__assets_start+r.dst,unpack.data()+r.extra,r.size);
            }
        }
    }
    gHeapEndPtr=savedHeap;

    for(uint32_t i=0;i<h->reloc_count;i++){
        uint32_t off=relocs[i].off;
        if(off&0x80000000u){
            // All external relocs are gCourseTable in the PSP build; Android
            // compiles courses/courseTable.c natively, so these bytes are unused.
            continue;
        }
        if(off+4u>regionSize || relocs[i].target>=regionSize) return false;
        uintptr_t v=reinterpret_cast<uintptr_t>(__assets_start+relocs[i].target);
        *reinterpret_cast<uint32_t*>(__assets_start+off)=static_cast<uint32_t>(v);
    }

    ALOGI("loaded %u recipes, %u relocs, %u courses",h->recipe_count,h->reloc_count,h->course_count);
    return true;
}
