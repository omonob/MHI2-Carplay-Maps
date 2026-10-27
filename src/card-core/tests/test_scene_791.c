/* Actual production CF declaration builder for791, no790 geometry. */
#define main golden_static_main_not_run
#include "test_scene_contract.original.c"
#undef main
static void save(const char *root,int position,unsigned generation){
 char path[512],body[128];FILE *f;int n;
 snprintf(path,sizeof(path),"%s/navigation-card.state",root);
 n=snprintf(body,sizeof(body),"format=1\nposition=%d\ngeneration=%u\n",position,generation);
 f=fopen(path,"wb");assert(f);assert(fwrite(body,1,(size_t)n,f)==(size_t)n);fprintf(f,"crc32=%x\n",(unsigned)cpmap_card_crc((const unsigned char *)body,(size_t)n));assert(!fclose(f));
}
int main(void){
 char root[]="/tmp/card791-scene-XXXXXX";int position;Obj *d,*views,*v,*safe,*adj;
 assert(mkdtemp(root));assert(!setenv("CPMAP_CARD_STATE_DIR",root,1));assert(!setenv("CARPLAY_ALTSCREEN_LOG","/dev/null",1));
 for(position=0;position<4;position++){
  save(root,position,(unsigned)position+1);d=create_alt_display();assert(d);
  assert(number(d,"widthPixels")==800&&number(d,"heightPixels")==480);
  assert(number(d,"widthPhysical")==160&&number(d,"heightPhysical")==96);
  assert(number(d,"maxFPS")==40&&number(d,"type")==111&&number(d,"features")==10);
  assert(ALTSCREEN_FORCE_KEYFRAME_INTERVAL==5);assert(number(d,"initialViewArea")==0);
  views=lookup(d,"viewAreas");assert(views&&views->count==1);v=views->items[0];safe=lookup(v,"safeArea");adj=lookup(v,"adjacentViewAreas");
  assert(number(v,"originXPixels")==0&&number(v,"originYPixels")==0&&number(v,"widthPixels")==800&&number(v,"heightPixels")==480);
  assert(number(safe,"originXPixels")==220&&number(safe,"originYPixels")==65&&number(safe,"widthPixels")==360&&number(safe,"heightPixels")==348);
  assert(number(safe,"originXPixels")+number(safe,"widthPixels")/2==400);
  assert(number(safe,"originYPixels")+number(safe,"heightPixels")/2==239);
  assert(number(safe,"originYPixels")==67-2);
  assert(number(safe,"originYPixels")+number(safe,"heightPixels")==413);
  assert(number(safe,"originYPixels")+number(safe,"heightPixels")<=480);
  assert(!adj);assert(cpmap_card_view_for(position,0)==0&&cpmap_card_view_for(position,1)==0);
  if(position==CPMAP_CARD_OFF)assert(!strcmp(string(d,"initialURL"),"maps:/car/instrumentcluster/map"));
  else assert(strstr(string(d,"initialURL"),"maneuverLayout=topAligned")&&!strstr(string(d,"initialURL"),"leftAligned")&&!strstr(string(d,"initialURL"),"rightAligned"));
  drop(d);assert(live==0);
 }
 fail_number=1;assert(!create_alt_display());assert(live==0);
 printf("VW791_PUBLIC_ACTUAL_CF_PASS encoded=800x480 physical=160x96 safe=%d,%d,%d,%d center=400,%d maxFPS=%d IDR=5 single_view=1 on_off=1 leaks=0\n",ALTSCREEN_SAFE_ORIGIN_X,ALTSCREEN_SAFE_ORIGIN_Y,ALTSCREEN_SAFE_WIDTH,ALTSCREEN_SAFE_HEIGHT,ALTSCREEN_SAFE_ORIGIN_Y+ALTSCREEN_SAFE_HEIGHT/2,ALTSCREEN_DISPLAY_MAX_FPS);return 0;
}
