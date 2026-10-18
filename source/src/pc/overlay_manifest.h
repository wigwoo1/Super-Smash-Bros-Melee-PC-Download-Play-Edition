#ifndef MELEE_PC_OVERLAY_MANIFEST_H
#define MELEE_PC_OVERLAY_MANIFEST_H
#include <pc/frozen_content_set.h>
/* UTF-8, no BOM: MELEE-OVERLAYS-v1 newline, then DVD path TAB relative
 * local path newline. LF/CRLF supported. Entries must be ASCII case sorted.
 * Relative paths resolve beside the manifest, never beside the executable.
 * This describes file overlays, not character/stage registration or rights. */
static int MeleePc_OverlayManifestBegin(MeleePc_FrozenContentSet* set,const wchar_t* manifest)
{
    MeleePc_FrozenOverlay input={0};void* reader=NULL;char* bytes=NULL;
    MeleePc_FrozenContentEntry* entries=NULL;wchar_t** paths=NULL;
    unsigned count=0;int success=0;size_t directory;
    if(!set||set->items||set->files||set->count||!manifest)return 0;
    if(!MeleePc_FrozenOverlayBegin(&input,manifest))return 0;
    if(!input.size||input.size>1048576)goto done;
    directory=wcslen(input.path);
    while(directory&&input.path[directory-1]!=L'\\'&&input.path[directory-1]!=L'/')directory--;
    if(!directory)goto done;
    bytes=(char*)malloc((size_t)input.size+1);
    entries=(MeleePc_FrozenContentEntry*)calloc(4096,sizeof(*entries));
    paths=(wchar_t**)calloc(4096,sizeof(*paths));
    if(!bytes||!entries||!paths)goto done;
    reader=MeleePc_FrozenOverlayOpen(&input);if(!reader)goto done;
    if(MeleePc_FrozenOverlayRead(reader,bytes,input.size)!=input.size)goto done;
    bytes[input.size]=0;if(memchr(bytes,0,input.size))goto done;
    char* line=bytes;char* next=strchr(line,'\n');if(!next)goto done;
    *next++=0;size_t length=strlen(line);if(length&&line[length-1]=='\r')line[--length]=0;
    if(strcmp(line,"MELEE-OVERLAYS-v1"))goto done;
    line=next;
    while(*line){
        char* tab;int wide_length;size_t segment=0;
        if(count==4096)goto done;
        next=strchr(line,'\n');if(next)*next++=0;
        length=strlen(line);if(length&&line[length-1]=='\r')line[--length]=0;
        tab=strchr(line,'\t');if(!tab||strchr(tab+1,'\t'))goto done;
        *tab++=0;if(!*tab)goto done;
        /* Explicit relative segments; no traversal, drive or rooted paths. */
        for(size_t i=0;tab[i];i++){
            unsigned char c=(unsigned char)tab[i];
            if(c<32||c==':'||c=='\\')goto done;
            if(c=='/'){if(!segment)goto done;segment=0;}
            else{if(c=='.'&&!segment)goto done;segment++;}
        }
        if(!segment)goto done;
        wide_length=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,tab,-1,NULL,0);
        if(wide_length<=0||directory+(size_t)wide_length>32767)goto done;
        paths[count]=(wchar_t*)malloc((directory+(size_t)wide_length)*sizeof(wchar_t));
        if(!paths[count])goto done;
        memcpy(paths[count],input.path,directory*sizeof(wchar_t));
        if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,tab,-1,paths[count]+directory,wide_length))goto done;
        entries[count].dvd_path=line;entries[count].local_path=paths[count];count++;
        if(!next)break;line=next;
    }
    if(count)success=MeleePc_FrozenContentSetBegin(set,entries,count);
done:
    if(reader)MeleePc_FrozenOverlayClose(reader);
    MeleePc_FrozenOverlayEnd(&input);
    if(paths){for(unsigned i=0;i<4096;i++)free(paths[i]);}
    free(paths);free(entries);free(bytes);return success;
}
#endif
