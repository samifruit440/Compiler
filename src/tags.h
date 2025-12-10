#ifndef TAGS_H
#define TAGS_H

/* Type tagging scheme for immediate constants */

#define fixnum_mask 3
#define fixnum_tag 0
#define fixnum_shift 2

#define char_mask 0xFF
#define char_tag 0x0F

#define bool_mask 0xFF
#define bool_tag 0x1F

#define empty_list_tag 0x2F

/* Inline helper to tag a fixnum */
static inline int tag_fixnum(int value) {
    return value << fixnum_shift;
}

/* Inline helper to untag a fixnum */
static inline int untag_fixnum(int value) {
    return value >> fixnum_shift;
}

#endif
