#ifndef CPMAP_CARD_TRANSITION_POLICY_H
#define CPMAP_CARD_TRANSITION_POLICY_H
/* Pure transition policy. The transport adapter must issue at most ONE command
 * at a time and call completed only for its matching async callback. No sleeps,
 * DSI calls, filesystem access, encoder changes, geometry or bus setters here.
 */
#include <stdint.h>
enum {CARD_COMMAND_NONE=0,CARD_COMMAND_SCENE=1,CARD_COMMAND_VIEW=2,CARD_COMMAND_IDR=3};
struct card_command {
 int operation,position,view;
 uint64_t token,epoch,generation;
};
struct card_transition_policy {
 int ready,busy,blocked,desired_position,desired_view,applied_position,applied_view,need_idr;
 uint64_t epoch,serial,desired_generation;
 struct card_command sent;
};
static void card_policy_reset(struct card_transition_policy *p,uint64_t epoch,int initial_view){
 memset(p,0,sizeof(*p));p->epoch=epoch;
 p->desired_position=CPMAP_CARD_RIGHT;p->desired_view=initial_view;
 p->applied_position=-1; /* READY must still perform original mandatory showUI. */
 p->applied_view=initial_view;
}
static void card_policy_desire(struct card_transition_policy *p,int position,int view,uint64_t generation){
 if(position<0||position>3||view<0||view>=CPMAP_CARD_VIEW_COUNT)return;
 if(position!=p->desired_position||view!=p->desired_view||generation!=p->desired_generation)p->blocked=0;
 p->desired_position=position;p->desired_view=view;p->desired_generation=generation;
}
static int card_policy_next(const struct card_transition_policy *p){
 if(!p->ready||p->busy||p->blocked)return CARD_COMMAND_NONE;
 if(p->desired_position!=p->applied_position)return CARD_COMMAND_SCENE;
 if(p->desired_view!=p->applied_view)return CARD_COMMAND_VIEW;
 return p->need_idr?CARD_COMMAND_IDR:CARD_COMMAND_NONE;
}
static int card_policy_take(struct card_transition_policy *p,struct card_command *out){
 int operation=card_policy_next(p);if(operation==CARD_COMMAND_NONE)return 0;
 if(++p->serial==0)++p->serial;
 p->sent.operation=operation;p->sent.position=p->desired_position;p->sent.view=p->desired_view;
 p->sent.token=p->serial;p->sent.epoch=p->epoch;p->sent.generation=p->desired_generation;
 p->busy=1;*out=p->sent;return 1;
}
/* Only use when the wire gate is busy and NOTHING has been sent. */
static void card_policy_defer_unsent(struct card_transition_policy *p,const struct card_command *command){
 if(p->busy&&p->epoch==command->epoch&&p->sent.token==command->token)p->busy=0;
}
static int card_policy_completed(struct card_transition_policy *p,const struct card_command *command,int status){
 if(!p->busy||p->epoch!=command->epoch||p->sent.token!=command->token)return 0;
 p->busy=0;
 if(status){
  /* Do not hammer an unsupported command. A new setting/view/connection may
   * try again; a lost ACK keeps busy until the session is torn down. */
  p->blocked=command->position==p->desired_position&&command->view==p->desired_view&&command->generation==p->desired_generation;
  return -1;
 }
 if(command->operation==CARD_COMMAND_SCENE){p->applied_position=command->position;
#ifdef CPMAP_CARD_PROFILE_791
  p->applied_view=command->view; /*one fixed791 view: scene toggle is not a layout change*/
#else
  p->applied_view=-1;
#endif
  p->need_idr=1;}
 else if(command->operation==CARD_COMMAND_VIEW){p->applied_view=command->view;p->need_idr=1;}
 else if(command->operation==CARD_COMMAND_IDR)p->need_idr=0;
 return 1;
}
#endif
