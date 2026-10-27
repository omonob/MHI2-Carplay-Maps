#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define ALTSCREEN_RUNTIME_CARD_POSITION 1
#include "../source/navigation_card_scene.h"
#include "../source/card_transition_policy.h"
static int checks;
static void check(int condition){checks++;assert(condition);}
static void converge(struct card_transition_policy *p,int position,int view,uint64_t generation){
 struct card_command command;int n=0;
 card_policy_desire(p,position,view,generation);
 while(card_policy_take(p,&command)){
  check(++n<=3);check(card_policy_next(p)==CARD_COMMAND_NONE);
  check(card_policy_completed(p,&command,0)==1);
  check(card_policy_completed(p,&command,0)==0); /* duplicate ACK ignored */
 }
 check(p->applied_position==position&&p->applied_view==view&&!p->busy&&!p->blocked&&!p->need_idr);
}
int main(void){
 struct card_transition_policy p;struct card_command old,command;int a,b,route;
 card_policy_reset(&p,1,0);check(!card_policy_take(&p,&command));p.ready=1;
 for(a=0;a<4;a++)for(b=0;b<4;b++)for(route=0;route<2;route++){
  converge(&p,a,cpmap_card_view_for(a,route),(uint64_t)checks+1);
  converge(&p,b,cpmap_card_view_for(b,route),(uint64_t)checks+1);
 }
 card_policy_desire(&p,0,3,10000);check(card_policy_take(&p,&old));
 card_policy_desire(&p,1,1,10001);check(!card_policy_take(&p,&command));
 check(card_policy_completed(&p,&old,0)==1);converge(&p,1,1,10001);
 card_policy_desire(&p,2,2,10002);check(card_policy_take(&p,&old));
 card_policy_reset(&p,2,0);p.ready=1;
 check(card_policy_completed(&p,&old,0)==0); /* stale previous-session ACK */
 card_policy_desire(&p,3,0,10003);check(card_policy_take(&p,&command));
 check(card_policy_completed(&p,&command,-7)==-1);check(!card_policy_take(&p,&old));
 card_policy_desire(&p,3,0,10004);check(card_policy_take(&p,&command));
 card_policy_defer_unsent(&p,&command);check(!p.busy&&!p.blocked);
 converge(&p,3,0,10004);
 printf("CARD_TRANSITION_POLICY_PASS checks=%d rapid_changes=1 stale_ack_rejected=1 single_inflight=1 no_retry_storm=1\n",checks);
 return 0;
}
