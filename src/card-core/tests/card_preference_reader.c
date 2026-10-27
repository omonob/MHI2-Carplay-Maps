#include "../source/cpmap_card_preferences.h"
int main(int argc,char **argv){
 struct cpmap_card_preference p;int rc;
 if(argc!=2)return 2;
 rc=cpmap_card_read(argv[1],&p);
 if(rc<0){puts("CARD_PREFERENCE_INVALID");return 1;}
 printf("position=%d generation=%llu source=%d rc=%d\n",p.position,(unsigned long long)p.generation,p.source,rc);
 return 0;
}
