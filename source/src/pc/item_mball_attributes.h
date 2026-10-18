#ifndef MELEE_PC_ITEM_MBALL_ATTRIBUTES_H
#define MELEE_PC_ITEM_MBALL_ATTRIBUTES_H

/* The M Ball article starts with itMBallAttributes, but the opening/demo
 * Pokemon-spawn path also reads the itPokemonSpawn_DatAttrs prefix that follows
 * those two floats. Convert only the source-consumed prefix: fourteen floats,
 * a six-element count, and thirty Pokemon weights. The trailing padding in the
 * 0x2bc-byte declaration is not interpreted by this planner. */
static void MeleePc_ItemMBallAttributesPlan(MeleePc_TitlePlan* p, uint32_t root)
{
    const uint32_t prefix_size = 0x3c + 30 * 4;
    uint32_t weight_sum = 0;
    if (p->error || (root & 3) || !title_range(p, root, prefix_size)) {
        p->error = -1;
        return;
    }
    for (uint32_t off = 0; off < prefix_size && !p->error; off += 4) {
        uint32_t slot = root + off;
        uint32_t bits = title_u32(p, slot);
        if (p->relocs[slot / 4] ||
            (p->external_slots && p->external_slots[slot / 4])) {
            p->error = -1;
            break;
        }
        if (off < 0x38) {
            float value;
            memcpy(&value, &bits, sizeof(value));
            if (!isfinite(value) || (off == 0x18 && value <= 0.0f)) {
                p->error = -1;
                break;
            }
        } else if (off == 0x38) {
            if (bits > 6) {
                p->error = -1;
                break;
            }
        } else {
            if (bits > 0x7fffffffU - weight_sum) {
                p->error = -1;
                break;
            }
            weight_sum += bits;
        }
        title_claim(p, slot, 4, 4);
    }
}

#endif
