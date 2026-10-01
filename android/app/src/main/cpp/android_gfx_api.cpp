#include "portable/gfx/gfx_rendering_api.h"
#include "portable/gfx/gfx_cc.h"
#include <PR/gbi.h>
#include <GLES2/gl2.h>
#include <android/log.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>

#define GLOGE(...) __android_log_print(ANDROID_LOG_ERROR, "MK64Gfx", __VA_ARGS__)

struct ShaderProgram {
    GLuint program = 0;
    uint32_t id = 0;
    uint8_t num_inputs = 0;
    bool used_textures[2]{false,false};
    size_t num_floats = 0;
    GLint attrib_locations[8]{};
    uint8_t attrib_sizes[8]{};
    uint8_t num_attribs = 0;
};

static std::unordered_map<uint32_t, ShaderProgram> gShaders;
static ShaderProgram* gCurrent = nullptr;
static GLuint gVbo = 0;

static bool z01(){ return false; }

static std::string item(uint32_t v, bool alpha, bool onlyAlpha, bool inputsAlpha) {
    if (onlyAlpha) {
        switch(v) {
            case SHADER_INPUT_1: return "vInput1.a";
            case SHADER_INPUT_2: return "vInput2.a";
            case SHADER_INPUT_3: return "vInput3.a";
            case SHADER_INPUT_4: return "vInput4.a";
            case SHADER_TEXEL0: case SHADER_TEXEL0A: return "texVal0.a";
            case SHADER_TEXEL1: return "texVal1.a";
            default: return "0.0";
        }
    }
    switch(v) {
        case SHADER_INPUT_1: return alpha || !inputsAlpha ? "vInput1" : "vInput1.rgb";
        case SHADER_INPUT_2: return alpha || !inputsAlpha ? "vInput2" : "vInput2.rgb";
        case SHADER_INPUT_3: return alpha || !inputsAlpha ? "vInput3" : "vInput3.rgb";
        case SHADER_INPUT_4: return alpha || !inputsAlpha ? "vInput4" : "vInput4.rgb";
        case SHADER_TEXEL0: return alpha ? "texVal0" : "texVal0.rgb";
        case SHADER_TEXEL0A: return alpha ? "vec4(texVal0.a)" : "vec3(texVal0.a)";
        case SHADER_TEXEL1: return alpha ? "texVal1" : "texVal1.rgb";
        default: return alpha ? "vec4(0.0)" : "vec3(0.0)";
    }
}

static std::string formula(const CCFeatures& f, int cycle, bool alpha, bool onlyAlpha) {
    const uint8_t* c=f.c[cycle];
    if(f.do_single[cycle]) return item(c[3],alpha,onlyAlpha,f.opt_alpha);
    if(f.do_multiply[cycle]) return item(c[0],alpha,onlyAlpha,f.opt_alpha)+" * "+item(c[2],alpha,onlyAlpha,f.opt_alpha);
    if(f.do_mix[cycle]) return "mix("+item(c[1],alpha,onlyAlpha,f.opt_alpha)+","+item(c[0],alpha,onlyAlpha,f.opt_alpha)+","+item(c[2],alpha,onlyAlpha,f.opt_alpha)+")";
    return "("+item(c[0],alpha,onlyAlpha,f.opt_alpha)+" - "+item(c[1],alpha,onlyAlpha,f.opt_alpha)+") * "+item(c[2],alpha,onlyAlpha,f.opt_alpha)+" + "+item(c[3],alpha,onlyAlpha,f.opt_alpha);
}

static GLuint compile(GLenum type,const std::string& src){
    GLuint s=glCreateShader(type); const char* p=src.c_str(); glShaderSource(s,1,&p,nullptr); glCompileShader(s);
    GLint ok=0; glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
    if(!ok){ char log[1024]={}; glGetShaderInfoLog(s,sizeof(log),nullptr,log); GLOGE("shader compile: %s",log); glDeleteShader(s); return 0; }
    return s;
}

static void attribs(ShaderProgram* p){
    if(!p) return;
    size_t off=0;
    for(int i=0;i<p->num_attribs;i++){
        glEnableVertexAttribArray(p->attrib_locations[i]);
        glVertexAttribPointer(p->attrib_locations[i],p->attrib_sizes[i],GL_FLOAT,GL_FALSE,p->num_floats*sizeof(float),(void*)(off*sizeof(float)));
        off+=p->attrib_sizes[i];
    }
}
static void unload(ShaderProgram* p){
    if(p) for(int i=0;i<p->num_attribs;i++) if(p->attrib_locations[i]>=0) glDisableVertexAttribArray(p->attrib_locations[i]);
    glUseProgram(0); gCurrent=nullptr;
}
static void load(ShaderProgram* p){ if(p){ glUseProgram(p->program); gCurrent=p; attribs(p); } }
static ShaderProgram* lookup(uint32_t id){ auto i=gShaders.find(id); return i==gShaders.end()?nullptr:&i->second; }

static ShaderProgram* create(uint32_t id){
    CCFeatures f{}; gfx_cc_get_features(id,&f);
    std::string vs="attribute vec4 aVtxPos;\n";
    std::string fs="precision mediump float;\n";
    size_t nf=4;
    if(f.used_textures[0]||f.used_textures[1]){vs+="attribute vec2 aTexCoord; varying vec2 vTexCoord;\n";fs+="varying vec2 vTexCoord;\n";nf+=2;}
    if(f.opt_fog){vs+="attribute vec4 aFog; varying vec4 vFog;\n";fs+="varying vec4 vFog;\n";nf+=4;}
    for(int i=0;i<f.num_inputs;i++){
        char b[128]; std::snprintf(b,sizeof(b),"attribute vec%d aInput%d; varying vec%d vInput%d;\n",f.opt_alpha?4:3,i+1,f.opt_alpha?4:3,i+1);
        vs+=b; fs+=std::string(b).substr(std::string(b).find("varying")); nf+=f.opt_alpha?4:3;
    }
    if(f.used_textures[0]) fs+="uniform sampler2D uTex0;\n";
    if(f.used_textures[1]) fs+="uniform sampler2D uTex1;\n";
    vs+="void main(){";
    if(f.used_textures[0]||f.used_textures[1]) vs+="vTexCoord=aTexCoord;";
    if(f.opt_fog) vs+="vFog=aFog;";
    for(int i=0;i<f.num_inputs;i++){char b[64];std::snprintf(b,sizeof(b),"vInput%d=aInput%d;",i+1,i+1);vs+=b;}
    vs+="gl_Position=aVtxPos;}\n";

    fs+="void main(){";
    if(f.used_textures[0]) fs+="vec4 texVal0=texture2D(uTex0,vTexCoord);";
    if(f.used_textures[1]) fs+="vec4 texVal1=texture2D(uTex1,vTexCoord);";
    if(f.opt_alpha){
        if(!f.color_alpha_same) fs+="vec4 texel=vec4("+formula(f,0,false,false)+","+formula(f,1,true,true)+");";
        else fs+="vec4 texel="+formula(f,0,true,false)+";";
        if(f.opt_texture_edge) fs+="if(texel.a>0.3) texel.a=1.0; else discard;";
        if(f.opt_fog) fs+="texel=vec4(mix(texel.rgb,vFog.rgb,vFog.a),texel.a);";
        fs+="gl_FragColor=texel;";
    }else{
        fs+="vec3 texel="+formula(f,0,false,false)+";";
        if(f.opt_fog) fs+="texel=mix(texel,vFog.rgb,vFog.a);";
        fs+="gl_FragColor=vec4(texel,1.0);";
    }
    fs+="}\n";

    GLuint v=compile(GL_VERTEX_SHADER,vs), q=compile(GL_FRAGMENT_SHADER,fs);
    if(!v||!q) return nullptr;
    GLuint prog=glCreateProgram(); glAttachShader(prog,v); glAttachShader(prog,q); glLinkProgram(prog);
    glDeleteShader(v); glDeleteShader(q);
    GLint ok=0; glGetProgramiv(prog,GL_LINK_STATUS,&ok);
    if(!ok){char log[1024]={};glGetProgramInfoLog(prog,sizeof(log),nullptr,log);GLOGE("program link: %s",log);glDeleteProgram(prog);return nullptr;}

    ShaderProgram p{}; p.program=prog;p.id=id;p.num_inputs=(uint8_t)f.num_inputs;p.used_textures[0]=f.used_textures[0];p.used_textures[1]=f.used_textures[1];p.num_floats=nf;
    int n=0;
    p.attrib_locations[n]=glGetAttribLocation(prog,"aVtxPos");p.attrib_sizes[n++]=4;
    if(f.used_textures[0]||f.used_textures[1]){p.attrib_locations[n]=glGetAttribLocation(prog,"aTexCoord");p.attrib_sizes[n++]=2;}
    if(f.opt_fog){p.attrib_locations[n]=glGetAttribLocation(prog,"aFog");p.attrib_sizes[n++]=4;}
    for(int i=0;i<f.num_inputs;i++){char nm[24];std::snprintf(nm,sizeof(nm),"aInput%d",i+1);p.attrib_locations[n]=glGetAttribLocation(prog,nm);p.attrib_sizes[n++]=f.opt_alpha?4:3;}
    p.num_attribs=n;
    auto [it,_]=gShaders.emplace(id,p);
    load(&it->second);
    if(f.used_textures[0]) glUniform1i(glGetUniformLocation(prog,"uTex0"),0);
    if(f.used_textures[1]) glUniform1i(glGetUniformLocation(prog,"uTex1"),1);
    return &it->second;
}
static void info(ShaderProgram* p,uint8_t*n,bool t[2]){ if(!p)return; *n=p->num_inputs;t[0]=p->used_textures[0];t[1]=p->used_textures[1]; }
static uint32_t newtex(){ GLuint t=0;glGenTextures(1,&t);return t; }
static void seltex(int tile,uint32_t id){glActiveTexture(GL_TEXTURE0+tile);glBindTexture(GL_TEXTURE_2D,id);}
static void upload(const uint8_t*b,int w,int h){glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,b);}
static GLenum wrap(uint32_t v){if(v&G_TX_CLAMP)return GL_CLAMP_TO_EDGE;return (v&G_TX_MIRROR)?GL_MIRRORED_REPEAT:GL_REPEAT;}
static void sampler(int tile,bool linear,uint32_t cms,uint32_t cmt){glActiveTexture(GL_TEXTURE0+tile);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,linear?GL_LINEAR:GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,linear?GL_LINEAR:GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,wrap(cms));glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,wrap(cmt));}
static void depth(bool v){v?glEnable(GL_DEPTH_TEST):glDisable(GL_DEPTH_TEST);}
static void dmask(bool v){glDepthMask(v?GL_TRUE:GL_FALSE);}
static void decal(bool v){if(v){glEnable(GL_POLYGON_OFFSET_FILL);glPolygonOffset(-2.f,-2.f);}else{glDisable(GL_POLYGON_OFFSET_FILL);}}
static void viewport(int x,int y,int w,int h){glViewport(x,y,w,h);}
static void scissor(int x,int y,int w,int h){glEnable(GL_SCISSOR_TEST);glScissor(x,y,w,h);}
static void alpha(bool v){v?glEnable(GL_BLEND):glDisable(GL_BLEND);}
static void draw(float* b,size_t len,size_t tris){if(!gCurrent||!gCurrent->program||!tris)return;glBindBuffer(GL_ARRAY_BUFFER,gVbo);glBufferData(GL_ARRAY_BUFFER,len*sizeof(float),b,GL_STREAM_DRAW);attribs(gCurrent);glDrawArrays(GL_TRIANGLES,0,(GLsizei)(tris*3));}
static void init(){if(!gVbo)glGenBuffers(1,&gVbo);glBindBuffer(GL_ARRAY_BUFFER,gVbo);glDepthFunc(GL_LEQUAL);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);}
static void resize(){}
static void start(){glDisable(GL_SCISSOR_TEST);glDepthMask(GL_TRUE);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glEnable(GL_SCISSOR_TEST);}
static void end(){}
static void finish(){glFinish();}

extern "C" GfxRenderingAPI mk64_android_gfx_api = {
 z01,unload,load,create,lookup,info,newtex,seltex,upload,sampler,depth,dmask,decal,viewport,scissor,alpha,draw,init,resize,start,end,finish
};
