#ifndef MELEE_PC_MAIN_MENU_NAMES_H
#define MELEE_PC_MAIN_MENU_NAMES_H
/* Original auto/refuse arrays end in a relocated pointer to an empty string.
 * Count includes that terminator. Default-name roots have one string pointer.
 * Text bytes (including multibyte encoding) are never endian-swapped. */
static void MeleePc_MainMenuNamesPlan(MeleePc_TitlePlan* p, uint32_t root,
                                    uint32_t count, int list)
{
    if (p->error) return;
    if ((root & 3) || !count || count > p->size / 4 ||
        (!list && count != 1) || !title_range(p, root, count * 4)) {
        p->error = -1; return;
    }
    for (uint32_t i = 0; i < count && !p->error; ++i) {
        uint32_t target = title_pointer(p, root + i * 4);
        if (target == UINT32_MAX || !title_range(p, target, 1)) {
            p->error = -1; return;
        }
        unsigned char* text = p->bytes + 32 + target;
        unsigned char* end = memchr(text, 0, p->size - target);
        if (!end || (list && ((end == text) != (i + 1 == count)))) {
            p->error = -1; return;
        }
        uint32_t size = (uint32_t)(end - text) + 1;
        for (uint32_t at = target & ~3U; at < target + size; at += 4)
            if (p->relocs[at / 4] ||
                (p->external_slots && p->external_slots[at / 4])) {
                p->error = -1; return;
            }
        title_claim(p, target, size, 1);
    }
}
#endif
