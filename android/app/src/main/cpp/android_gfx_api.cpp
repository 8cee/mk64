#include "portable/gfx/gfx_rendering_api.h"
#include "portable/gfx/gfx_cc.h"
#include <GLES2/gl2.h>
#include <android/log.h>
#include <unordered_map>

struct ShaderProgram {
    GLuint program = 0;
    uint32_t id = 0;
    uint8_t num_inputs = 0;
    bool used_textures[2]{false,false};
};

static std::unordered_map<uint32_t, ShaderProgram> gShaders;
static ShaderProgram* gCurrent = nullptr;

static bool z01(){ return true; }
static void unload(ShaderProgram*){ glUseProgram(0); gCurrent=nullptr; }
static void load(ShaderProgram* p){ if(p){ glUseProgram(p->program); gCurrent=p; } }
static ShaderProgram* lookup(uint32_t id){ auto i=gShaders.find(id); return i==gShaders.end()?nullptr:&i->second; }
static ShaderProgram* create(uint32_t id){
    auto &p=gShaders[id]; p.id=id;
    CCFeatures f{}; gfx_cc_get_features(id,&f); p.num_inputs=(uint8_t)f.num_inputs;
    p.used_textures[0]=f.used_textures[0]; p.used_textures[1]=f.used_textures[1];
    // Shader generation is filled by the next renderer step; retain decoded
    // combiner metadata now so the F3DEX frontend and GLES backend share ABI.
    return &p;
}
static void info(ShaderProgram* p,uint8_t*n,bool t[2]){ if(!p)return; *n=p->num_inputs;t[0]=p->used_textures[0];t[1]=p->used_textures[1]; }
static uint32_t newtex(){ GLuint t;glGenTextures(1,&t);return t; }
static void seltex(int tile,uint32_t id){glActiveTexture(GL_TEXTURE0+tile);glBindTexture(GL_TEXTURE_2D,id);}
static void upload(const uint8_t*b,int w,int h){glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,b);}
static void sampler(int,bool linear,uint32_t,uint32_t){glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,linear?GL_LINEAR:GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,linear?GL_LINEAR:GL_NEAREST);}
static void depth(bool v){v?glEnable(GL_DEPTH_TEST):glDisable(GL_DEPTH_TEST);}
static void dmask(bool v){glDepthMask(v?GL_TRUE:GL_FALSE);}
static void decal(bool){}
static void viewport(int x,int y,int w,int h){glViewport(x,y,w,h);}
static void scissor(int x,int y,int w,int h){glEnable(GL_SCISSOR_TEST);glScissor(x,y,w,h);}
static void alpha(bool v){v?glEnable(GL_BLEND):glDisable(GL_BLEND);}
static void draw(float*,size_t,size_t){}
static void init(){}
static void resize(){}
static void start(){}
static void end(){}
static void finish(){glFinish();}

extern "C" GfxRenderingAPI mk64_android_gfx_api = {
 z01,unload,load,create,lookup,info,newtex,seltex,upload,sampler,depth,dmask,decal,viewport,scissor,alpha,draw,init,resize,start,end,finish
};
