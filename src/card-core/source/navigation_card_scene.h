#ifndef VW790_NAVIGATION_CARD_SCENE_H
#define VW790_NAVIGATION_CARD_SCENE_H
#ifndef VW790_CARD_SIDE
#define VW790_CARD_SIDE 2
#endif
#ifdef ALTSCREEN_RUNTIME_CARD_POSITION
#include "cpmap_card_preferences.h"
#ifndef CPMAP_CARD_PROFILE_791
#include "cpmap_navigation_ownership.h"
#endif
struct cpmap_card_rect {int x,y,width,height;};
#ifdef CPMAP_CARD_PROFILE_791
/*791 has one fixed drawable layout, not790's left/right/center views.
 * Keep the actual installed safe area. ON/OFF changes only scene URL. */
#define CPMAP_CARD_VIEW_COUNT 1
static struct cpmap_card_rect cpmap_card_rect_for_view(int index){
 struct cpmap_card_rect r={ALTSCREEN_SAFE_ORIGIN_X,ALTSCREEN_SAFE_ORIGIN_Y,ALTSCREEN_SAFE_WIDTH,ALTSCREEN_SAFE_HEIGHT};
 (void)index;return r;
}
static int cpmap_card_view_for(int position,int navigating){(void)position;(void)navigating;return 0;}
static const char *cpmap_card_url_for(int position){
 return position==CPMAP_CARD_OFF?"maps:/car/instrumentcluster/map":"maps:/car/instrumentcluster?showETA=no&maneuverLayout=topAligned&showSpeedLimit=no";
}
#else
#define CPMAP_CARD_VIEW_COUNT 5
/* All outer viewAreas and the encoded stream remain EXACTLY 1010x376.
 * 0 is the accepted N7/N8 idle view; 1/2 are byte-for-byte golden safe rects.
 * 3 is explicitly a narrow, topAligned compact CENTER declaration, NOT full.
 * 4 is the separately calibrated iPhone17ProMax navigation view for card OFF;
 * the original card rectangles 0..3 are not changed by map centering.
 */
static struct cpmap_card_rect cpmap_card_rect_for_view(int index){
 static const struct cpmap_card_rect rects[5]={{202,16,606,344},{18,75,606,284},{386,75,606,284},{325,75,360,284},{42,16,606,344}};
 return rects[index>=0&&index<5?index:0];
}
static int cpmap_card_view_for(int position,int navigating){
 if(position==CPMAP_CARD_OFF)return navigating?4:0;
 if(position==CPMAP_CARD_CENTER)return 3;
 if(navigating&&position==CPMAP_CARD_LEFT)return 1;
 if(navigating&&position==CPMAP_CARD_RIGHT)return 2;
 return 0;
}
static const char *cpmap_card_url_for(int position){
 switch(position){
 case CPMAP_CARD_CENTER:return "maps:/car/instrumentcluster?showETA=no&maneuverLayout=topAligned&showSpeedLimit=no";
 case CPMAP_CARD_LEFT:return "maps:/car/instrumentcluster?showETA=no&maneuverLayout=leftAligned&showSpeedLimit=no";
 case CPMAP_CARD_OFF:return "maps:/car/instrumentcluster/map?showETA=no&maneuverLayout=no&showSpeedLimit=no";
 default:return "maps:/car/instrumentcluster?showETA=no&maneuverLayout=rightAligned&showSpeedLimit=no";
 }
}
#endif
static pthread_mutex_t g_card_preference_lock=PTHREAD_MUTEX_INITIALIZER;
static struct cpmap_card_preference g_card_preference={CPMAP_CARD_RIGHT,0,0};
static int card_refresh_preference(void){
 struct cpmap_card_preference readback;int rc;const char *root=getenv("CPMAP_CARD_STATE_DIR");
 if(!root||!root[0])root="/mnt/icab/persistent/cpmap-ui";
 rc=cpmap_card_read(root,&readback);
 pthread_mutex_lock(&g_card_preference_lock);
 if(rc>=0&&(rc>0||g_card_preference.generation==0))g_card_preference=readback;
 pthread_mutex_unlock(&g_card_preference_lock);
 return rc;
}
static struct cpmap_card_preference card_snapshot(void){
 struct cpmap_card_preference p;pthread_mutex_lock(&g_card_preference_lock);p=g_card_preference;pthread_mutex_unlock(&g_card_preference_lock);return p;
}
static const char *card_current_url(void){return cpmap_card_url_for(card_snapshot().position);}
#define ALT_MAP_URL card_current_url()
#else
#if VW790_CARD_SIDE == 0
#define ALT_MAP_URL "maps:/car/instrumentcluster/map"
#elif VW790_CARD_SIDE == 1
#define ALT_MAP_URL "maps:/car/instrumentcluster?showETA=no&maneuverLayout=leftAligned&showSpeedLimit=no"
#elif VW790_CARD_SIDE == 2
#define ALT_MAP_URL "maps:/car/instrumentcluster?showETA=no&maneuverLayout=rightAligned&showSpeedLimit=no"
#else
#error "Select baseline0, left1 or right2"
#endif
#endif
#endif
