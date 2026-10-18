#ifndef MELEE_PC_FROZEN_CONTENT_SET_H
#define MELEE_PC_FROZEN_CONTENT_SET_H
#include <pc/frozen_overlay.h>
#include <pc/content_digest.h>
typedef struct MeleePc_FrozenContentEntry {const char* dvd_path;const wchar_t* local_path;} MeleePc_FrozenContentEntry;
typedef struct MeleePc_FrozenContentItem {MeleePc_FrozenOverlay provider;char name[256];} MeleePc_FrozenContentItem;
typedef struct MeleePc_FrozenContentSet {
    MeleePc_FrozenContentItem* items;AuroraOverlayFile* files;unsigned count;
} MeleePc_FrozenContentSet;
/* Owner preparation; zero initialize. Does not mutate the DVD registry.
 * Paths sorted without ASCII case; preserve actual DVD spelling, copy names
 * and guard every file before
 * publishing the set. A failed file leaves the caller's set unchanged. */
static int MeleePc_FrozenContentSetBegin(MeleePc_FrozenContentSet* set,
    const MeleePc_FrozenContentEntry* entries,unsigned count)
{
    MeleePc_FrozenContentSet candidate={0};unsigned prepared=0;
    if(!set||set->items||set->files||set->count||!entries||!count||count>4096)return 0;
    for(unsigned i=0;i<count;i++){
        const char* name=entries[i].dvd_path;
        char canonical[256];size_t length;
        if(!name||name[0]!='/'||!entries[i].local_path)return 0;
        length=strlen(name);if(length>252)return 0;
        for(size_t j=0;j<=length;j++){
            unsigned char c=(unsigned char)name[j];
            canonical[j]=(char)(c>='A'&&c<='Z'?c+('a'-'A'):c);
        }
        if(!MeleePc_ContentIdLength(canonical+1)||
           (i&&_stricmp(entries[i-1].dvd_path,name)>=0))return 0;
    }
    candidate.items=(MeleePc_FrozenContentItem*)calloc(count,sizeof(*candidate.items));
    candidate.files=(AuroraOverlayFile*)calloc(count,sizeof(*candidate.files));
    if(!candidate.items||!candidate.files)goto fail;
    for(;prepared<count;prepared++){
        MeleePc_FrozenContentItem* item=&candidate.items[prepared];
        if(!MeleePc_FrozenOverlayBegin(&item->provider,entries[prepared].local_path))goto fail;
        memcpy(item->name,entries[prepared].dvd_path,strlen(entries[prepared].dvd_path)+1);
        candidate.files[prepared].fileName=item->name;
        candidate.files[prepared].userData=&item->provider;
        candidate.files[prepared].size=item->provider.size;
    }
    candidate.count=count;*set=candidate;return 1;
fail:
    for(unsigned i=0;i<prepared;i++)MeleePc_FrozenOverlayEnd(&candidate.items[i].provider);
    free(candidate.items);free(candidate.files);return 0;
}
/* Caller must detach the registry and prevent new opens before release.
 * Readers keep the entire set alive; no active-reader partial teardown. */
static int MeleePc_FrozenContentSetEnd(MeleePc_FrozenContentSet* set)
{
    if(!set||!set->items||!set->files||!set->count)return 0;
    for(unsigned i=0;i<set->count;i++)if(set->items[i].provider.readers)return 0;
    for(unsigned i=0;i<set->count;i++)if(!MeleePc_FrozenOverlayEnd(&set->items[i].provider))return 0;
    free(set->items);free(set->files);memset(set,0,sizeof(*set));return 1;
}
#endif
