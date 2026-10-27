#define _GNU_SOURCE
/* Only the renamed, unexecuted legacy790 fixture main refers to this symbol.
 * The791 production profile and its actual assertions use no790 safe area. */
#ifndef VW790_NAV_SAFE_X
#define VW790_NAV_SAFE_X 0
#endif
#include <assert.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Reference-counted CFL double. Tests invoke the real VW production builders
 * and activation function, not a second implementation of the wire format. */
enum { STRING=1, NUMBER, DICT, ARRAY };
typedef struct Obj Obj;
struct Obj {
    int kind, refs, count;
    int64_t number;
    char text[192], keys[32][80];
    Obj *items[32];
};
static int live, fail_number, fail_send, omit_send, sends, updates;
static int key_cb, val_cb, arr_cb;
static const char *expected_url;
static Obj *new_obj(int kind) {
    Obj *o=calloc(1,sizeof(*o)); assert(o); o->kind=kind; o->refs=1; ++live; return o;
}
static const void *keep(const void *p) {
    Obj *o=(Obj *)p; assert(o && o->refs>0); ++o->refs; return o;
}
static void drop(const void *p) {
    Obj *o=(Obj *)p; int i; if(!o) return; assert(o->refs>0);
    if(--o->refs) return;
    for(i=0;i<o->count;++i) drop(o->items[i]);
    --live; free(o);
}
static Obj *lookup(const void *p,const char *key) {
    const Obj *o=p; int i; assert(o && o->kind==DICT);
    for(i=0;i<o->count;++i) if(!strcmp(o->keys[i],key)) return o->items[i];
    return NULL;
}
static void put(Obj *o,const char *key,const void *value) {
    int i; assert(o && o->kind==DICT && value && strlen(key)<80);
    for(i=0;i<o->count;++i) if(!strcmp(o->keys[i],key)) break;
    if(i==o->count) { assert(o->count<32); ++o->count; strcpy(o->keys[i],key); }
    keep(value); drop(o->items[i]); o->items[i]=(Obj *)value;
}
static int64_t number(const void *p,const char *key) {
    Obj *o=lookup(p,key); assert(o && o->kind==NUMBER); return o->number;
}
static const char *string(const void *p,const char *key) {
    Obj *o=lookup(p,key); assert(o && o->kind==STRING); return o->text;
}
static void *cf_string(void *a,const char *s,uint32_t enc) {
    Obj *o=new_obj(STRING); assert(!a && enc==0x08000100U && strlen(s)<sizeof(o->text));
    strcpy(o->text,s); return o;
}
static void *cf_number(int64_t n) {
    Obj *o; if(fail_number) return NULL; o=new_obj(NUMBER); o->number=n; return o;
}
static void *cf_dict(void *a,long n,const void *k,const void *v) {
    assert(!a && !n && k==&key_cb && v==&val_cb); return new_obj(DICT);
}
static void *cf_array(void *a,long n,const void *cb) {
    assert(!a && !n && cb==&arr_cb); return new_obj(ARRAY);
}
static void cf_append(void *p,const void *v) {
    Obj *o=p; assert(o && o->kind==ARRAY && o->count<32);
    o->items[o->count++]=(Obj *)keep(v);
}
static void cf_set(void *p,const void *k,const void *v) {
    const Obj *s=k; assert(s && s->kind==STRING); put(p,s->text,v);
}
static int32_t cf_set_int(void *p,const void *k,int64_t n) {
    Obj *v=cf_number(n); if(!v) return -1; cf_set(p,k,v); drop(v); return 0;
}
static int32_t cf_set_string(void *p,const void *k,const char *s,size_t n) {
    Obj *v; assert(n==(size_t)-1); v=cf_string(NULL,s,0x08000100U);
    cf_set(p,k,v); drop(v); return 0;
}
static const void *cf_get(const void *p,const void *k) {
    const Obj *s=k; assert(s && s->kind==STRING); return lookup(p,s->text);
}
#ifdef CPMAP_CARD_TRANSPORT_TEST
static int32_t card_transport_test_send(void *,const void *,void (*)(int32_t,const void *,void *),void *);
#endif
static int32_t cf_send(void *session,const void *p,
                       void (*callback)(int32_t,const void *,void *),void *context) {
#ifdef CPMAP_CARD_TRANSPORT_TEST
    return card_transport_test_send(session,p,callback,context);
#else
    const Obj *request=p, *params=lookup(p,"params");
    assert(session && params && request->count==2);
    assert(!strcmp(string(params,"uuid"),"b7e6c5a0-2222-4000-8000-000000000002"));
    assert(!lookup(params,"displayUUID"));
    if(!strcmp(string(request,"type"),"showUI")) {
        assert(params->count==2 && !strcmp(string(params,"url"),expected_url)); ++sends;
    } else {
        const Obj *adj=lookup(params,"adjacentViewAreas");
        assert(!strcmp(string(request,"type"),"updateViewArea"));
        assert(params->count==4 && number(params,"animationDurationMillis")==0);
        assert(number(params,"viewAreaIndex")==1);
        assert(adj && adj->count==1 && adj->items[0]->number==0); ++updates;
    }
    if(fail_send) return -99;
    /* Nonzero ACK avoids the existing optional IDR thread in this test.
     * Success dispatch/real wire and callback ownership are still exercised. */
    callback(-7,NULL,context); return 0;
#endif
}
static void *symbol(void *scope,const char *name) {
    (void)scope;
#define SYM(n,v) if(!strcmp(name,n)) return (void *)(v)
    SYM("CFStringCreateWithCString",cf_string);
    SYM("CFNumberCreateInt64",cf_number);
    SYM("CFDictionaryCreateMutable",cf_dict);
    SYM("CFArrayCreateMutable",cf_array);
    SYM("CFDictionarySetValue",cf_set);
    SYM("CFDictionarySetInt64",cf_set_int);
    SYM("CFDictionarySetCString",cf_set_string);
    SYM("CFDictionaryGetValue",cf_get);
    SYM("CFArrayAppendValue",cf_append);
    SYM("CFRelease",drop); SYM("CFRetain",keep);
    SYM("kCFLDictionaryKeyCallBacksCFLTypes",&key_cb);
    SYM("kCFLDictionaryValueCallBacksCFLTypes",&val_cb);
    SYM("kCFLArrayCallBacksCFLTypes",&arr_cb);
    if(!strcmp(name,"AirPlayReceiverSessionSendCommand")) return omit_send ? NULL : (void *)cf_send;
#undef SYM
    return NULL;
}
#define dlsym symbol
#include "../source/altscreen_handshake_probe.c"
#undef dlsym

static void check_view(const Obj *v,int x,int y,int height,int adjacent) {
    const Obj *safe=lookup(v,"safeArea"), *adj=lookup(v,"adjacentViewAreas");
    assert(number(v,"originXPixels")==0 && number(v,"originYPixels")==0);
    assert(number(v,"widthPixels")==1010 && number(v,"heightPixels")==376);
    assert(number(safe,"originXPixels")==x && number(safe,"originYPixels")==y);
    assert(number(safe,"widthPixels")==606 && number(safe,"heightPixels")==height);
    assert(adj && adj->count==1 && adj->items[0]->number==adjacent);
}
int main(void) {
    Obj *display, *views, *session; AltActivationTask *task;
    expected_url = VW790_CARD_SIDE==1 ?
        "maps:/car/instrumentcluster?showETA=no&maneuverLayout=leftAligned&showSpeedLimit=no" :
        VW790_CARD_SIDE==2 ?
        "maps:/car/instrumentcluster?showETA=no&maneuverLayout=rightAligned&showSpeedLimit=no" :
        "maps:/car/instrumentcluster/map";
    assert(!setenv("CARPLAY_ALTSCREEN_LOG","/dev/null",1));
    display=create_alt_display(); assert(display);
    assert(!strcmp(string(display,"uuid"),ALT_UUID));
    assert(!strcmp(string(display,"initialURL"),expected_url));
    assert(number(display,"type")==111 && number(display,"maxFPS")==60);
    assert(number(display,"widthPixels")==1010 && number(display,"heightPixels")==376);
    assert(number(display,"widthPhysical")==202 && number(display,"heightPhysical")==75);
    assert(number(display,"initialViewArea")==0);
    views=lookup(display,"viewAreas"); assert(views && views->count==2);
    check_view(views->items[0],202,16,344,1);
    check_view(views->items[1],VW790_CARD_SIDE==2 ? 386 : 18,75,284,0);
    assert(VW790_CARD_SIDE!=2 || VW790_NAV_SAFE_X+606==1010-18);
    assert(ALTSCREEN_FORCE_KEYFRAME_INTERVAL==180);
    drop(display); assert(live==0);
    fail_number=1; assert(!create_alt_display()); assert(live==0); fail_number=0;
    session=new_obj(DICT);
    assert(!send_alt_command(session,"showUI",ALT_MAP_URL));
    assert(sends==1 && live==1 && session->refs==1);
    fail_send=1; assert(send_alt_command(session,"showUI",ALT_MAP_URL)==-99);
    assert(live==1 && session->refs==1); fail_send=0;
    omit_send=1; assert(send_alt_command(session,"showUI",ALT_MAP_URL)==-1);
    assert(live==1 && session->refs==1); omit_send=0;
    /* Actual READY worker's activation body, zero delay: same UUID + URL. */
    task=calloc(1,sizeof(*task)); assert(task); task->session=(void *)keep(session);
    assert(!delayed_alt_activation_thread(task)); assert(sends==3 && live==1);
    assert(!send_viewarea_update(session,1,0,0)); assert(updates==1 && live==1);
    assert(send_viewarea_update(session,2,0,0)==-1 && updates==1 && live==1);
    drop(session); assert(live==0);
    printf("VW790_SCENE_CONTRACT_PASS side=%d nav_safe_x=%d live_objects=%d\n",
           VW790_CARD_SIDE,VW790_NAV_SAFE_X,live);
    return 0;
}
