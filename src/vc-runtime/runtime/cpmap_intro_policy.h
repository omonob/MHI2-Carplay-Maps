#ifndef CPMAP_INTRO_POLICY_H
#define CPMAP_INTRO_POLICY_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "cpmap_animation_preference.h"

#ifndef INTRO_POLICY_PATH
#define INTRO_POLICY_PATH "/mnt/app/carplaymaps/intro-policy"
#endif

typedef enum CpmapIntroPolicy {
    CPMAP_INTRO_OFF = 0,
    CPMAP_INTRO_ONCE_PER_BOOT = 2,
    CPMAP_INTRO_EVERY_CONNECT = 1
} CpmapIntroPolicy;

/* Missing, truncated or unknown configuration retains the proven default.
 * This is data, never a shell script or an environment command. */
static inline CpmapIntroPolicy cpmap_intro_read_policy(const char *path)
{
    char value[64];
    FILE *file;
    size_t size;
    int mode, preference=cpmap_animation_read(CPMAP_ANIMATION_STATE_DIR,&mode);
    if(preference>0)return mode==0?CPMAP_INTRO_EVERY_CONNECT:mode==1?CPMAP_INTRO_ONCE_PER_BOOT:CPMAP_INTRO_OFF;
    /* Corrupt UI data suppresses only the intro, never the live map stream. */
    if(preference<0)return CPMAP_INTRO_OFF;
    file=fopen(path,"rb");
    if (!file) return CPMAP_INTRO_ONCE_PER_BOOT;
    size = fread(value, 1, sizeof(value) - 1U, file);
    if (ferror(file) || !feof(file)) {
        fclose(file);
        return CPMAP_INTRO_ONCE_PER_BOOT;
    }
    fclose(file);
    value[size] = '\0';
    if((size==4U&&memcmp(value,"off\n",4U)==0)||(size==5U&&memcmp(value,"off\r\n",5U)==0))return CPMAP_INTRO_OFF;
    if ((size == 14U && memcmp(value, "every-connect\n", 14U) == 0) ||
        (size == 15U && memcmp(value, "every-connect\r\n", 15U) == 0))
        return CPMAP_INTRO_EVERY_CONNECT;
    return CPMAP_INTRO_ONCE_PER_BOOT;
}

static inline int cpmap_intro_should_play(CpmapIntroPolicy policy,
                                         const char *boot_marker)
{
    return policy != CPMAP_INTRO_OFF && (policy == CPMAP_INTRO_EVERY_CONNECT ||
           access(boot_marker, F_OK) != 0);
}

#endif
