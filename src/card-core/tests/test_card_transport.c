/* Real CF builders, send adapter, READY body, polling body and callback body.
 * The receiver is an asynchronous reference-counted test double, not an iPhone.
 */
#define CPMAP_CARD_TRANSPORT_TEST 1
#define main golden_main_not_run
#include "test_scene_contract.original.c"
#undef main
static Obj *pending_request;
static void *pending_context;
static void (*pending_callback)(int32_t,const void *,void *);
static int pending,in_callback,wire_sends,scene_sends,view_sends,idr_sends,checks;
static void check(int ok){checks++;assert(ok);}
static int32_t card_transport_test_send(void *session,const void *request,void (*cb)(int32_t,const void *,void *),void *context){
 const char *type=string(request,"type");const Obj *params=lookup(request,"params");int i,j;
 check(session!=NULL&&params!=NULL);check(!pending&&!in_callback);
 check(!strcmp(string(params,"uuid"),ALT_UUID));check(!lookup(params,"displayUUID"));
 if(fail_send)return -99;
 if(!strcmp(type,"showUI")){
  check(params->count==2);check(strstr(string(params,"url"),"maps:/car/instrumentcluster")!=NULL);scene_sends++;
 }else if(!strcmp(type,"updateViewArea")){
  const Obj *adj=lookup(params,"adjacentViewAreas");j=(int)number(params,"viewAreaIndex");
  check(j>=0&&j<CPMAP_CARD_VIEW_COUNT&&number(params,"animationDurationMillis")==0&&params->count==4);
  check(adj&&adj->count==CPMAP_CARD_VIEW_COUNT-1);for(i=0;i<CPMAP_CARD_VIEW_COUNT-1;i++)check(adj->items[i]->number>=0&&adj->items[i]->number<CPMAP_CARD_VIEW_COUNT&&adj->items[i]->number!=j);
  view_sends++;
 }else{check(!strcmp(type,"forceKeyFrame")&&params->count==1);idr_sends++;}
 pending_request=(Obj *)keep(request);pending_callback=cb;pending_context=context;pending=1;wire_sends++;return 0;
}
static void ack(int status){
 Obj *request=pending_request;void *context=pending_context;void (*cb)(int32_t,const void *,void *)=pending_callback;
 check(pending);pending=0;pending_request=NULL;pending_context=NULL;pending_callback=NULL;
 in_callback=1;cb(status,NULL,context);in_callback=0;drop(request);
}
static void set_position(const char *root,int position,unsigned generation){
 char body[128],path[512];FILE *f;int n;
 snprintf(path,sizeof(path),"%s/navigation-card.state",root);
 n=snprintf(body,sizeof(body),"format=1\nposition=%d\ngeneration=%u\n",position,generation);
 f=fopen(path,"wb");check(f!=NULL);check(fwrite(body,1,(size_t)n,f)==(size_t)n);
 fprintf(f,"crc32=%x\n",(unsigned)cpmap_card_crc((const unsigned char *)body,(size_t)n));check(!fclose(f));
 check(card_refresh_preference()>=0);
}
static void drain(int position,int view){
 int i;for(i=0;i<10;i++){
  card_runtime_poll();if(pending)ack(0);
  if(g_card_policy.applied_position==position&&g_card_policy.applied_view==view&&!g_card_policy.need_idr&&!g_card_policy.busy)break;
 }
 check(i<10);check(g_card_policy.applied_position==position&&g_card_policy.applied_view==view&&!g_card_policy.blocked);
 card_runtime_poll();check(!pending);
}
int main(void){
 char root[]="/tmp/card-transport-test-XXXXXX",route_path[512],state_path[512];
 Obj *session,*second,*display;AltActivationTask *activation;FILE *file;int n,i;unsigned generation=1;
 check(mkdtemp(root)!=NULL);check(!setenv("CPMAP_CARD_STATE_DIR",root,1));check(!setenv("CARPLAY_ALTSCREEN_LOG","/dev/null",1));
 snprintf(route_path,sizeof(route_path),"%s/navigation.pps",root);check(!setenv("CARPLAY_NAVIGATION_STATE_PATH",route_path,1));
 set_position(root,2,generation++);display=create_alt_display();check(display!=NULL);drop(display);
 session=new_obj(DICT);g_viewarea_worker_started=1; /* explicitly poll instead of spawning a real endless worker */
 remember_viewarea_session(session);card_runtime_poll();check(!pending); /* no command before READY */
 activation=calloc(1,sizeof(*activation));check(activation!=NULL);activation->session=(void *)keep(session);
 delayed_alt_activation_thread(activation);card_runtime_poll();check(pending&&scene_sends==1&&view_sends==0);
 n=wire_sends;for(i=0;i<10;i++)card_runtime_poll();check(wire_sends==n);
 check(send_alt_command(session,"forceKeyFrame",NULL)==-EBUSY);check(wire_sends==n); /* shared wire gate */
 ack(0);check(g_card_policy.applied_position==2&&g_card_policy.applied_view==0);
 card_runtime_poll();check(pending&&view_sends==0&&idr_sends==1);ack(0);
 drain(2,0);n=wire_sends;for(i=0;i<10;i++)card_runtime_poll();check(wire_sends==n);
 /* Exercise actual PPS reader + existing 400ms navigation debounce. */
 file=fopen(route_path,"wb");check(file!=NULL);fputs("state::1\n",file);fclose(file);
 card_runtime_poll();check(!pending);usleep(450000);card_runtime_poll();check(!pending);drain(2,0); /*791 has no layout change with route state*/
 for(i=0;i<4;i++){set_position(root,i,generation++);drain(i,cpmap_card_view_for(i,1));}
 /* Latest selection wins while a previous showUI awaits ACK. */
 set_position(root,0,generation++);card_runtime_poll();check(pending);n=view_sends;
 set_position(root,1,generation++);card_runtime_poll();check(view_sends==n);ack(0);
 card_runtime_poll();check(pending&&!strcmp(string(pending_request,"type"),"showUI"));check(!strcmp(string(lookup(pending_request,"params"),"url"),cpmap_card_url_for(1)));
 ack(0);drain(1,0);
 /* Negative ACK blocks retries of the same saved generation. */
 set_position(root,3,generation++);card_runtime_poll();check(pending);ack(-7);n=wire_sends;
 for(i=0;i<20;i++){card_runtime_poll();}
 check(wire_sends==n&&g_card_policy.blocked);
 set_position(root,3,generation++);drain(3,0);
 /* Immediate dispatch failure also leaves no leaked context/wire lock. */
 set_position(root,0,generation++);fail_send=1;card_runtime_poll();fail_send=0;
 check(!pending&&g_card_policy.blocked&&g_card_wire_active==0);set_position(root,0,generation++);drain(0,0);
 omit_send=1;set_position(root,1,generation++);card_runtime_poll();omit_send=0;check(g_card_policy.blocked&&!pending);
 set_position(root,1,generation++);drain(1,0);
 /* Lost ACK: no overlap/retry, then detach and reject the old-session ACK. */
 set_position(root,0,generation++);card_runtime_poll();check(pending);n=wire_sends;
 for(i=0;i<30;i++){card_runtime_poll();}
 check(wire_sends==n&&g_card_policy.busy);
 forget_viewarea_session(session);second=new_obj(DICT);display=create_alt_display();check(display!=NULL);drop(display);
 remember_viewarea_session(second);card_signal_ready(second);ack(0);check(g_card_policy.applied_position==-1&&!g_card_policy.busy);
 drain(0,0);check(!pending);forget_viewarea_session(second);drop(second);drop(session);check(live==0);
 /* Full valid Java-compatible preference is retained on damaged read. */
 snprintf(state_path,sizeof(state_path),"%s/navigation-card.state",root);file=fopen(state_path,"wb");check(file!=NULL);fputs("invalid\n",file);fclose(file);
 check(card_refresh_preference()<0&&card_snapshot().position==0);
 check(view_sends==0);
 printf("VW791_CARD_PRODUCTION_CF_TRANSPORT_PASS checks=%d single_view=1 no_updateViewArea=1 single_inflight=1 ready=1 on_off=1 rapid_switch=1 failed_ack=1 immediate_error=1 lost_ack=1 stale_session=1 leaked_cf=0 sends=%d\n",checks,wire_sends);
 return 0;
}
