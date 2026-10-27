/* Exercise the actual production CF builder, not a separate JSON model. */
#define main golden_static_main_not_run
#include "test_scene_contract.original.c"
#undef main

static void write_position(const char *root,int pos,unsigned generation){
 char path[512],body[128];FILE *f;int n;
 snprintf(path,sizeof(path),"%s/navigation-card.state",root);
 n=snprintf(body,sizeof(body),"format=1\nposition=%d\ngeneration=%u\n",pos,generation);
 f=fopen(path,"wb");assert(f);assert(fwrite(body,1,(size_t)n,f)==(size_t)n);
 assert(fprintf(f,"crc32=%x\n",(unsigned)cpmap_card_crc((const unsigned char *)body,(size_t)n))>0);
 assert(!fclose(f));
}
int main(void){
 char root[]="/tmp/cpmap-card-scene-test-XXXXXX";int pos,i,j;Obj *d,*views;
 static const int x[]={202,18,386,325,42},y[]={16,75,75,75,16},w[]={606,606,606,360,606},h[]={344,284,284,284,344};
 assert(mkdtemp(root));assert(!setenv("CPMAP_CARD_STATE_DIR",root,1));
 assert(!setenv("CARPLAY_ALTSCREEN_LOG","/dev/null",1));
 for(pos=0;pos<4;pos++){
  write_position(root,pos,(unsigned)pos+1);d=create_alt_display();assert(d);
  assert(!strcmp(string(d,"initialURL"),cpmap_card_url_for(pos)));
  assert(number(d,"type")==111&&number(d,"maxFPS")==60);
  assert(number(d,"widthPixels")==1010&&number(d,"heightPixels")==376);
  assert(number(d,"widthPhysical")==202&&number(d,"heightPhysical")==75);
  assert(number(d,"features")==10&&number(d,"primaryInputDevice")==3);
  assert(number(d,"initialViewArea")== (pos==0?3:0));
  views=lookup(d,"viewAreas");assert(views&&views->count==5);
  for(i=0;i<5;i++){
   Obj *view=views->items[i],*safe=lookup(view,"safeArea"),*adj=lookup(view,"adjacentViewAreas");
   assert(number(view,"originXPixels")==0&&number(view,"originYPixels")==0);
   assert(number(view,"widthPixels")==1010&&number(view,"heightPixels")==376);
   assert(number(safe,"originXPixels")==x[i]&&number(safe,"originYPixels")==y[i]);
   assert(number(safe,"widthPixels")==w[i]&&number(safe,"heightPixels")==h[i]);
   assert(x[i]>=0&&x[i]+w[i]<=1010&&y[i]>=0&&y[i]+h[i]<=376);
   assert(adj&&adj->count==4);for(j=0;j<4;j++)assert(adj->items[j]->number>=0&&adj->items[j]->number<5&&adj->items[j]->number!=i);
  }
  if(pos==CPMAP_CARD_CENTER)assert(strstr(string(d,"initialURL"),"maneuverLayout=topAligned")&&w[3]<w[1]);
  if(pos==CPMAP_CARD_OFF)assert(strstr(string(d,"initialURL"),"instrumentcluster/map?")&&strstr(string(d,"initialURL"),"maneuverLayout=no"));
  assert(cpmap_card_view_for(pos,0)==(pos==0?3:0));
  assert(cpmap_card_view_for(pos,1)==(pos==0?3:pos==1?1:pos==2?2:4));
  drop(d);assert(live==0);
 }
 fail_number=1;assert(!create_alt_display());assert(live==0);fail_number=0;
 printf("AID10_5_RUNTIME_BUILDER_PASS positions=4 views=5 iphone17_navigation_center=1 left_right_unchanged=1 encoded=1010x376 leaks=0\n");
 return 0;
}
