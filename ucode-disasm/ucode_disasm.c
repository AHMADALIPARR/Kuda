// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
/*
 * ucode_disasm.c - Portable C11 bit-field parser & structured disassembly table
 * generator for undocumented 32-bit microcode / raw binary words.
 *
 * Build:
 * cc -std=c11 -O2 -Wall -Wextra -Werror -pedantic ucode_disasm.c -o ucode_disasm
 *
 * Usage:
 * ./ucode_disasm (runs on built-in sample dump)
 * ./ucode_disasm dump.txt (parses file; use '-' for stdin)
 * ./ucode_disasm --csv dump.txt (emits CSV table instead of aligned text)
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t mask;
    uint32_t match;
    const char *mnemonic;
    const char *unit;
} OpcodeRule;

typedef struct {
    uint32_t addr;
    bool has_addr;
    uint32_t tag_hex;
    bool has_tag_hex;
    uint32_t word;
    bool valid;
} InsnRecord;

/* Extract inclusive bit slice [hi:lo] from a 32-bit word (bit 31 = MSB, 0 = LSB). */
static inline uint32_t bit_slice(uint32_t word, unsigned hi, unsigned lo) {
    unsigned width = (hi - lo) + 1U;
    uint32_t mask = (width >= 32U) ? 0xFFFFFFFFU : ((1U << width) - 1U);
    return (word >> lo) & mask;
}

/*
 * Extensible mask/match decode table for undocumented 32-bit microcode.
 * More specific masks are placed first; fallback rules catch unknown variants.
 */
static const OpcodeRule OPCODE_TABLE[] = {
    /* Control / Block Sync & Termination */
    {0xFFF00000U, 0x02000000U, "SYNC.BAR", "CTRL"},
    {0xFFF00000U, 0x1E000000U, "BLK.END", "CTRL"},

    /* Group 1 (OP=0x1) */
    {0xFFF00000U, 0x10000000U, "UOP1.BASE", "GRP1"},
    {0xFFF00000U, 0x18000000U, "UOP1.F3", "GRP1"},
    {0xFFF00000U, 0x14000000U, "UOP1.F2", "GRP1"},
    {0xFFF00000U, 0x12000000U, "UOP1.F1", "GRP1"},

    /* Group 2 (OP=0x2) */
    {0xFFF00000U, 0x20000000U, "UOP2.BASE", "GRP2"},
    {0xFFF00000U, 0x21000000U, "UOP2.F0", "GRP2"},
    {0xFFF00000U, 0x20800000U, "UOP2.M3", "GRP2"},
    {0xFFF00000U, 0x20400000U, "UOP2.M2", "GRP2"},

    /* Group 3 (OP=0x3) */
    {0xFFF00000U, 0x30000000U, "UOP3.BASE", "GRP3"},
    {0xFFF00000U, 0x31000000U, "UOP3.F0", "GRP3"},
    {0xFFF00000U, 0x32000000U, "UOP3.F1", "GRP3"},
    {0xFFF00000U, 0x30800000U, "UOP3.M3", "GRP3"},
    {0xFFF00000U, 0x30400000U, "UOP3.M2", "GRP3"},
};

static const OpcodeRule *decode_opcode(uint32_t word) {
    size_t count = sizeof(OPCODE_TABLE) / sizeof(OPCODE_TABLE[0]);
    for (size_t i = 0; i < count; ++i) {
        if ((word & OPCODE_TABLE[i].mask) == OPCODE_TABLE[i].match) {
            return &OPCODE_TABLE[i];
        }
    }
    return NULL;
}

static char *trim_ws(char *s) {
    while (*s && isspace((unsigned char)*s)) {
        ++s;
    }
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[--len] = '\0';
    }
    return s;
}

static bool parse_binary32(const char *s, uint32_t *out) {
    uint32_t val = 0U;
    size_t bits = 0U;
    for (const char *p = s; *p != '\0'; ++p) {
        if (*p == '0' || *p == '1') {
            if (bits >= 32U) {
                return false;
            }
            val = (val << 1U) | (uint32_t)(*p - '0');
            ++bits;
        } else if (*p == '_' || isspace((unsigned char)*p)) {
            continue;
        } else {
            return false;
        }
    }
    if (bits != 32U) {
        return false;
    }
    *out = val;
    return true;
}

static bool parse_hex32(const char *s, uint32_t *out) {
    char *end = NULL;
    unsigned long v = strtoul(s, &end, 16);
    if (end == s || *trim_ws(end) != '\0' || v > 0xFFFFFFFFUL) {
        return false;
    }
    *out = (uint32_t)v;
    return true;
}

/*
 * Supported line formats:
 * 1) "ADDR: TAG_HEX | 32_BIT_BINARY"
 * 2) "ADDR: 32_BIT_BINARY"
 * 3) "32_BIT_BINARY"
 * 4) "0xHEX32"
 */
static InsnRecord parse_line(const char *raw_line) {
    InsnRecord rec = {0};
    char buf[512];
    snprintf(buf, sizeof(buf), "%s", raw_line);

    char *line = trim_ws(buf);
    if (*line == '\0' || *line == '#') {
        return rec;
    }

    char *pipe = strchr(line, '|');
    if (pipe != NULL) {
        *pipe = '\0';
        char *left = trim_ws(line);
        char *right = trim_ws(pipe + 1);

        if (!parse_binary32(right, &rec.word)) {
            return rec;
        }
        rec.valid = true;

        char *colon = strchr(left, ':');
        if (colon != NULL) {
            *colon = '\0';
            char *addr_str = trim_ws(left);
            char *tag_str = trim_ws(colon + 1);
            rec.has_addr = parse_hex32(addr_str, &rec.addr);
            rec.has_tag_hex = parse_hex32(tag_str, &rec.tag_hex);
        } else {
            rec.has_tag_hex = parse_hex32(left, &rec.tag_hex);
        }
        return rec;
    }

    char *colon = strchr(line, ':');
    if (colon != NULL) {
        *colon = '\0';
        char *addr_str = trim_ws(line);
        char *rhs = trim_ws(colon + 1);
        rec.has_addr = parse_hex32(addr_str, &rec.addr);
        if (parse_binary32(rhs, &rec.word) || parse_hex32(rhs, &rec.word)) {
            rec.valid = true;
        }
        return rec;
    }

    if (parse_binary32(line, &rec.word)) {
        rec.valid = true;
    } else if (strncmp(line, "0x", 2) == 0 || strncmp(line, "0X", 2) == 0) {
        rec.valid = parse_hex32(line + 2, &rec.word);
    }
    return rec;
}

static void print_header(bool csv) {
    if (csv) {
        printf("addr,word_hex,tag_hex,tag_match,op_31_28,sub_27_24,mod_23_20,rsv_19_12,imm_11_0,unit,mnemonic\n");
    } else {
        printf("%-6s | %-10s | %-10s | %-5s | %-4s | %-4s | %-4s | %-4s | %-5s | %-5s | %-12s\n",
               "ADDR", "BIN_HEX", "TAG_HEX", "MATCH", "OP", "SUB", "MOD", "RSV", "IMM12", "UNIT", "MNEMONIC");
        printf("-------+------------+------------+-------+------+------+------+------+-------+-------+-------------\n");
    }
}

static void emit_record(const InsnRecord *rec, uint32_t fallback_idx, bool csv) {
    uint32_t addr = rec->has_addr ? rec->addr : fallback_idx;
    uint32_t op = bit_slice(rec->word, 31U, 28U);
    uint32_t sub = bit_slice(rec->word, 27U, 24U);
    uint32_t mod = bit_slice(rec->word, 23U, 20U);
    uint32_t rsv = bit_slice(rec->word, 19U, 12U);
    uint32_t imm = bit_slice(rec->word, 11U, 0U);

    const OpcodeRule *rule = decode_opcode(rec->word);
    const char *unit = rule ? rule->unit : "UNK";
    const char *mnem = rule ? rule->mnemonic : "RAW.UNK";

    const char *match_str = "N/A";
    if (rec->has_tag_hex) {
        match_str = (rec->tag_hex == rec->word) ? "OK" : "DIFF";
    }

    if (csv) {
        printf("%04X,0x%08X,%08X,%s,0x%X,0x%X,0x%X,0x%02X,0x%03X,%s,%s\n",
               addr, rec->word, rec->has_tag_hex ? rec->tag_hex : 0U,
               match_str, op, sub, mod, rsv, imm, unit, mnem);
    } else {
        char tag_buf[16];
        if (rec->has_tag_hex) {
            snprintf(tag_buf, sizeof(tag_buf), "0x%08X", rec->tag_hex);
        } else {
            snprintf(tag_buf, sizeof(tag_buf), "-");
        }
        printf("%04X | 0x%08X | %-10s | %-5s | 0x%X | 0x%X | 0x%X | 0x%02X | 0x%03X | %-5s | %-12s\n",
               addr, rec->word, tag_buf, match_str, op, sub, mod, rsv, imm, unit, mnem);
    }
}

static const char *DEFAULT_DUMP[] = {
    "0800: 10000001 | 00010000000000000000000000000001",
    "0801: 10000012 | 00010000000000000000000000010010",
    "0802: 10000100 | 00010000000000000000000100000000",
    "0803: 11000001 | 00011000000000000000000000000001",
    "0804: 11000012 | 00011000000000000000000000010010",
    "0805: 11000100 | 00011000000000000000000100000000",
    "0806: 10100001 | 00010100000000000000000000000001",
    "0807: 10010001 | 00010010000000000000000000000001",
    "0808: 00100001 | 00000010000000000000000000000001",
    "0809: 11110001 | 00011110000000000000000000000001",
    "0810: 20000001 | 00100000000000000000000000000001",
    "0811: 20000012 | 00100000000000000000000000010010",
    "0812: 21000001 | 00100001000000000000000000000001",
    "0813: 21000012 | 00100001000000000000000000010010",
    "0814: 20100001 | 00100000100000000000000000000001",
    "0815: 20010001 | 00100000010000000000000000000001",
    "0816: 00100001 | 00000010000000000000000000000001",
    "0817: 11110001 | 00011110000000000000000000000001",
    "0820: 30000001 | 00110000000000000000000000000001",
    "0821: 310000FF | 00110001000000000000000011111111",
    "0822: 30100001 | 00110000100000000000000000000001",
    "0823: 30010001 | 00110000010000000000000000000001",
    "0824: 00100001 | 00000010000000000000000000000001",
    "0825: 32000001 | 00110010000000000000000000000001",
    "0826: 11110001 | 00011110000000000000000000000001",
};

int main(int argc, char **argv) {
    bool csv = false;
    const char *path = NULL;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--csv") == 0) {
            csv = true;
        } else {
            path = argv[i];
        }
    }

    print_header(csv);

    if (path == NULL) {
        size_t n = sizeof(DEFAULT_DUMP) / sizeof(DEFAULT_DUMP[0]);
        for (size_t i = 0; i < n; ++i) {
            InsnRecord rec = parse_line(DEFAULT_DUMP[i]);
            if (rec.valid) {
                emit_record(&rec, (uint32_t)i, csv);
            }
        }
        return 0;
    }

    FILE *fp = (strcmp(path, "-") == 0) ? stdin : fopen(path, "r");
    if (fp == NULL) {
        perror("fopen");
        return 1;
    }

    char line[512];
    uint32_t idx = 0U;
    while (fgets(line, sizeof(line), fp) != NULL) {
        InsnRecord rec = parse_line(line);
        if (rec.valid) {
            emit_record(&rec, idx++, csv);
        }
    }

    if (fp != stdin) {
        fclose(fp);
    }
    return 0;
}
