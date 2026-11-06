/* THE MUSIC BOX HOUSE - Trijam #397 (Escape Room theme)
 * Brand-new first-person escape-room game on the GBA instant raycaster.
 * Clockwork music-box house: 7 gated rooms, musical puzzles, original
 * melodies on the Pumpkin Parade Fix5-style frame-driven PSG driver.
 * Tile-step movement (Up step / Down back / Left-Right turn 90),
 * A interact, Start inventory, Select minimap. Instant redraw.
 * All SFX non-blocking (stepped per vblank). Gate jingle ONLY on
 * first entry to a new area through a newly unlocked gate.
 */
typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32;
typedef signed char s8; typedef short s16; typedef int s32;
#define REG16(a) (*(volatile u16*)(a))
#define REG_DISPCNT REG16(0x04000000)
#define REG_VCOUNT REG16(0x04000006)
#define REG_KEY REG16(0x04000130)
#define PAL ((volatile u16*)0x05000000)
#define FB0 ((volatile u8*)0x06000000)
#define FB1 ((volatile u8*)0x0600A000)
#define SRAM ((volatile u8*)0x0E000000)
#define SND(x) REG16(0x04000060 + (x))
#define REG_SOUNDCNT_L REG16(0x04000080)
#define REG_SOUNDCNT_H REG16(0x04000082)
#define REG_SOUNDCNT_X REG16(0x04000084)
#define REG_SND1CNT_L REG16(0x04000060)
#define REG_SND1CNT_H REG16(0x04000062)
#define REG_SND1CNT_X REG16(0x04000064)
#define REG_SND2CNT_L REG16(0x04000068)
#define REG_SND2CNT_H REG16(0x0400006C)
#define REG_SND4CNT_L REG16(0x04000078)
#define REG_SND4CNT_H REG16(0x0400007C)
#define KEY_A 0x0001
#define KEY_B 0x0002
#define KEY_SELECT 0x0004
#define KEY_START 0x0008
#define KEY_RIGHT 0x0010
#define KEY_LEFT 0x0020
#define KEY_UP 0x0040
#define KEY_DOWN 0x0080
#define RGB15(r,g,b) ((u16)((r) | ((g)<<5) | ((b)<<10)))

#include "trig.h"
#include "font.h"

/* ---- cell codes ---- */
#define C_FLOOR 0
#define C_WOOD 1
#define C_BRASS 2
#define C_DARK 3
#define C_DIAL 4
#define C_LEVER 5
#define C_BELL 6
#define C_PLATE 7
#define C_NOTE 8
#define C_CHEST 9
#define C_CORE 10
#define C_MECH 11
#define C_GATE 12
#define C_BACK 13
#define C_ITEM 14
#define C_STAND 15

#define MW 9
#define MH 9
#define NAREAS 7

/* Maps: '1' wood wall, 'D' gear dial, 'L' lever, 'b' bell, 'P' piano plate,
 * 'N' note, 'C' chest, 'X' core, 'S' music stand, 'G' forward gate,
 * 'B' back gate, 'K' brass key, 'k' brass cog, 'T' tuning fork,
 * 'c' clock key, 'F' final key (spawned by chest). */
static char MAPS[NAREAS][MH][MW + 1] = {
{   /* 1. The Winding Room */
    "111111111",
    "10D000D01",
    "100000001",
    "100000001",
    "1N000000G",
    "100000001",
    "10D000D01",
    "1C00000K1",
    "111111111",
},
 {   /* 2. The Sheet-Music Library */
    "111111111",
    "1NS0S0SN1",
    "100000001",
    "100000001",
    "B0000000G",
    "100000001",
    "100000001",
    "1000S00k1",
    "111111111",
},
{   /* 3. The Gear Gallery */
    "111111111",
    "1NL0L0L01",
    "100000001",
    "100000001",
    "B0000000G",
    "100000001",
    "100000001",
    "100000001",
    "111111111",
},
{   /* 4. The Piano Parlor */
    "111111111",
    "1NPPPPP01",
    "100000001",
    "100000001",
    "B0000000G",
    "100000001",
    "100000001",
    "100000001",
    "111111111",
},
{   /* 5. The Bell Tower */
    "111111111",
    "1Nb0b0b01",
    "100000001",
    "100000001",
    "B0000000G",
    "100000001",
    "100000001",
    "100000001",
    "111111111",
},
{   /* 6. The Melody Room */
    "111111111",
    "1NPPPPP01",
    "100000001",
    "100000001",
    "B0000000G",
    "100000001",
    "100000001",
    "100000001",
    "111111111",
},
{   /* 7. The Music-Box Core */
    "111111111",
    "1000X0001",
    "100000001",
    "100000001",
    "B0N000001",
    "100000001",
    "100000001",
    "100000001",
    "111111111",
},
};

static const char *AREA_NAME[NAREAS] = {
    "THE WINDING ROOM", "THE SHEET-MUSIC LIBRARY", "THE GEAR GALLERY",
    "THE PIANO PARLOR", "THE BELL TOWER", "THE MELODY ROOM",
    "THE MUSIC-BOX CORE",
};

/* items: 0 brass key, 1 brass cog, 2 tuning fork, 3 clock key, 4 final key */
static const char *ITEM_NAME[5] = {
    "BRASS KEY", "BRASS COG", "TUNING FORK", "CLOCK KEY", "FINAL KEY",
};
#define IT_KEY 0
#define IT_COG 1
#define IT_FORK 2
#define IT_CLOCK 3
#define IT_FINAL 4

static int iabs(int v) { return v < 0 ? -v : v; }
static void vsync(void) {
    while (REG_VCOUNT >= 160) { }
    while (REG_VCOUNT < 160) { }
}

static int cell_code(char c) {
    switch (c) {
        case '0': return C_FLOOR;
        case '1': return C_WOOD;
        case '2': return C_BRASS;
        case '3': return C_DARK;
        case 'D': return C_DIAL;
        case 'L': return C_LEVER;
        case 'b': return C_BELL;
        case 'P': return C_PLATE;
        case 'N': return C_NOTE;
        case 'C': return C_CHEST;
        case 'X': return C_CORE;
        case 'M': return C_MECH;
        case 'G': return C_GATE;
        case 'S': return C_STAND;
        case 'B': return C_BACK;
        default: return C_ITEM; /* K k T c F */
    }
}
static int item_type_at(int area, int x, int y) {
    char c = MAPS[area][y][x];
    if (c == 'K') return IT_KEY;
    if (c == 'k') return IT_COG;
    if (c == 'T') return IT_FORK;
    if (c == 'c') return IT_CLOCK;
    return IT_FINAL; /* 'F' */
}

/* Each wall/object type gets 6 palette bands of 4 shades (near/mid/far x
 * front/side), 24 entries, starting at 16 + type*24. */
static const int TYPE_BAND[16] = {
    0, 16, 40, 64, 88, 112, 136, 160, 184, 208, 232, 88, 40, 40, 0, 0
};
/* item -> palette band (distinct colour per item in the 3D view) */
static const int ITEM_BAND[5] = {40, 88, 136, 112, 232};
static const int TYPE_R[10][4] = {
    {8,14,20,28}, {22,26,30,31}, {24,28,30,31}, {10,16,24,30}, {10,18,26,31},
    {14,22,29,31}, {14,20,26,31}, {16,22,28,31}, {8,14,20,28}, {8,12,18,26},
};
static const int TYPE_G[10][4] = {
    {5,9,13,19}, {17,21,25,29}, {7,10,13,17}, {6,10,14,20}, {5,9,13,19},
    {11,17,23,29}, {13,19,25,30}, {15,21,27,31}, {5,9,13,19}, {10,16,22,28},
};
static const int TYPE_B[10][4] = {
    {2,4,6,9}, {5,7,9,12}, {7,10,13,17}, {8,12,16,22}, {2,3,5,8},
    {3,5,7,12}, {10,15,20,26}, {12,17,22,28}, {3,5,7,10}, {12,20,28,31},
};
static const int BAND_FAC[6] = {16, 12, 12, 10, 10, 8};

static u8 tex_wall[64 * 64];
static u8 tex_panel[64 * 64];
static u8 tex_gate[64 * 64];

/* Pastel music-box palette, per area: sky / floor dark / floor light / wall. */
static const int SKY_RGB[7][3] = {
    {26,22,31}, {18,26,28}, {22,24,31}, {30,22,26}, {20,26,31}, {28,22,30}, {24,18,30},
};
static const int FLOOR_D_RGB[7][3] = {
    {24,18,22}, {22,24,20}, {22,20,26}, {26,20,22}, {20,22,28}, {26,20,26}, {24,18,26},
};
static const int FLOOR_L_RGB[7][3] = {
    {30,27,24}, {28,30,26}, {28,26,30}, {31,28,28}, {26,28,31}, {31,27,29}, {30,24,30},
};
static const int WALL_R[7][4] = {
    {22,25,28,31}, {14,18,22,26}, {24,26,28,31}, {26,28,30,31}, {18,21,24,28}, {26,28,30,31}, {24,26,28,31},
};
static const int WALL_G[7][4] = {
    {16,19,22,26}, {22,25,28,31}, {20,22,24,27}, {20,23,26,29}, {20,23,26,30}, {18,21,24,28}, {16,20,24,28},
};
static const int WALL_B[7][4] = {
    {20,23,26,30}, {20,23,26,29}, {16,20,24,28}, {22,24,27,30}, {26,28,30,31}, {24,26,28,31}, {26,28,30,31},
};
static void build_palette(int a) {
    int t, s2, i;
    if (a < 0) a = 0; if (a > 6) a = 6;
    PAL[0] = RGB15(SKY_RGB[a][0], SKY_RGB[a][1], SKY_RGB[a][2]);       /* sky */
    PAL[1] = RGB15(FLOOR_D_RGB[a][0], FLOOR_D_RGB[a][1], FLOOR_D_RGB[a][2]); /* floor dark */
    PAL[2] = RGB15(FLOOR_L_RGB[a][0], FLOOR_L_RGB[a][1], FLOOR_L_RGB[a][2]); /* floor light */
    PAL[3] = RGB15(30, 24, 8);    /* text gold */
    PAL[4] = RGB15(31, 31, 28);   /* white */
    PAL[5] = RGB15(28, 6, 6);     /* red */
    PAL[6] = RGB15(8, 26, 10);    /* green */
    PAL[7] = RGB15(10, 14, 28);   /* blue */
    PAL[8] = RGB15(24, 12, 20);   /* rose */
    PAL[9] = RGB15(12, 22, 26);   /* teal */
    PAL[10] = RGB15(26, 18, 6);   /* amber */
    PAL[11] = RGB15(8, 8, 10);    /* dark slate (banner / message bar) */
    PAL[12] = RGB15(30, 16, 22);  /* title pink */
    PAL[13] = RGB15(16, 28, 22);  /* title mint */
    PAL[14] = RGB15(26, 22, 31);  /* title lavender background */
    PAL[15] = RGB15(31, 28, 24);  /* title cream */
    for (t = 0; t < 10; t++) {
        for (s2 = 0; s2 < 6; s2++) {
            for (i = 0; i < 4; i++) {
                int r = (t == 0) ? WALL_R[a][i] : TYPE_R[t][i];
                int g = (t == 0) ? WALL_G[a][i] : TYPE_G[t][i];
                int b = (t == 0) ? WALL_B[a][i] : TYPE_B[t][i];
                PAL[16 + t * 24 + s2 * 4 + i] = RGB15(
                    r * BAND_FAC[s2] / 16,
                    g * BAND_FAC[s2] / 16,
                    b * BAND_FAC[s2] / 16);
            }
        }
    }
}
static void build_textures(void) {
    int tx, ty;
    for (ty = 0; ty < 64; ty++) {
        for (tx = 0; tx < 64; tx++) {
            int c;
            int stalk = tx & 7;
            if (stalk == 0) c = 0;
            else if (stalk == 1) c = 2;
            else c = 1;
            if (((tx * 7 + ty * 5) & 7) == 0) c = 2;
            if ((ty & 31) == 0 && (tx & 3) != 0) c = 3;
            tex_wall[ty * 64 + tx] = (u8)c;
            /* panel: plain with a horizontal mechanism band */
            c = 1;
            if (ty >= 26 && ty <= 38) c = 3;
            else if (ty == 24 || ty == 25 || ty == 39 || ty == 40) c = 2;
            tex_panel[ty * 64 + tx] = (u8)c;
            /* gate: vertical brass stripe */
            c = 1;
            if (tx >= 28 && tx <= 36) c = 3;
            else if (tx == 26 || tx == 27 || tx == 37 || tx == 38) c = 2;
            tex_gate[ty * 64 + tx] = (u8)c;
        }
    }
}

/* ---- framebuffer text + shapes ---- */
static int glyph_index(char c) {
    int i;
    if (c >= 'a' && c <= 'z') c = (char)(c - 32);
    for (i = 0; FONT_ORDER[i]; i++) if (FONT_ORDER[i] == c) return i;
    return 0;
}
/* 8x8 font (v7, GRIMLIGHT's): 8 rows of 8 bits, bit 7 = leftmost pixel,
 * 8px advance. Glyphs are 8px tall (was 6) with wider letterforms. */
static void draw_text(volatile u8 *fb, int x, int y, const char *str, int scale, int color) {
    int ci, gx, gy, sx, sy;
    for (ci = 0; str[ci]; ci++) {
        int g = glyph_index(str[ci]);
        for (gy = 0; gy < 8; gy++) {
            int bits = FONT_GLYPHS[g * 8 + gy];
            for (gx = 0; gx < 8; gx++) {
                if (bits & (1 << (7 - gx))) {
                    for (sy = 0; sy < scale; sy++) for (sx = 0; sx < scale; sx++) {
                        int px = x + ci * 8 * scale + gx * scale + sx;
                        int py = y + gy * scale + sy;
                        if (px >= 0 && px < 240 && py >= 0 && py < 160) fb[py * 240 + px] = (u8)color;
                    }
                }
            }
        }
    }
}
static void draw_text_c(volatile u8 *fb, int y, const char *str, int scale, int color) {
    int w = 0, i;
    for (i = 0; str[i]; i++) w++;
    draw_text(fb, (240 - w * 8 * scale) / 2, y, str, scale, color);
}
/* fb_stride/fb_h let the icon painter draw into a 16x16 sprite buffer
 * (stride 16) as well as the real framebuffer (stride 240). Colour 0 in
 * buffer mode means "hole" -> transparent (255). */
static int fb_stride = 240, fb_h = 160;
static void fill_rect(volatile u8 *fb, int x0, int y0, int x1, int y1, int color) {
    int x, y;
    if (fb_stride == 16 && color == 0) color = 255;
    for (y = y0; y <= y1; y++) for (x = x0; x <= x1; x++) {
        if (x >= 0 && x < fb_stride && y >= 0 && y < fb_h) fb[y * fb_stride + x] = (u8)color;
    }
}
static void fill_circle(volatile u8 *fb, int cx, int cy, int r, int color) {
    int x, y;
    if (fb_stride == 16 && color == 0) color = 255;
    for (y = cy - r; y <= cy + r; y++) for (x = cx - r; x <= cx + r; x++) {
        int dx = x - cx, dy = y - cy;
        if (dx * dx + dy * dy <= r * r && x >= 0 && x < fb_stride && y >= 0 && y < fb_h)
            fb[y * fb_stride + x] = (u8)color;
    }
}

/* Distinct 16x16 icon per item/object kind, drawn with primitives.
 * kind: 0 key, 1 cog, 2 fork, 3 clock key, 4 final key, 5 dial,
 * 6 lever, 7 bell, 8 plate, 9 note, 10 chest, 11 core, 12 gate.
 * Multi-tone art (gold 3 / amber 10 / white 4 / dark slate 11 /
 * red 5 / green 6 / rose 8), and SOLID: every drawn pixel is opaque —
 * no colour-0 "holes" that let the wall show through. In buffer mode
 * the icon is a solid silhouette; only the 16x16 corners stay clear.
 * Lever: param 1 = pulled correctly -> green stick + knob; pulled
 * levers reset to gold/amber when the order is wrong. */
static void draw_icon(volatile u8 *fb, int x, int y, int kind, int color, int param) {
    int i;
    switch (kind) {
        case 0: /* brass key: gold bow, amber stem, gold teeth */
            fill_circle(fb, x + 4, y + 5, 4, color);
            fill_circle(fb, x + 4, y + 5, 2, 11);
            fill_rect(fb, x + 7, y + 4, x + 14, y + 6, 10);
            fill_rect(fb, x + 11, y + 7, x + 12, y + 9, color);
            fill_rect(fb, x + 13, y + 7, x + 14, y + 8, color);
            fill_rect(fb, x + 3, y + 3, x + 4, y + 3, 4);
            break;
        case 1: /* cog: gold teeth + ring, amber face, dark hub */
            for (i = 0; i < 4; i++) {
                fill_rect(fb, x + 6, y + 0, x + 9, y + 2, color);
                fill_rect(fb, x + 6, y + 13, x + 9, y + 15, color);
                fill_rect(fb, x + 0, y + 6, x + 2, y + 9, color);
                fill_rect(fb, x + 13, y + 6, x + 15, y + 9, color);
            }
            fill_circle(fb, x + 8, y + 8, 6, color);
            fill_circle(fb, x + 8, y + 8, 4, 10);
            fill_circle(fb, x + 8, y + 8, 2, 11);
            fill_rect(fb, x + 6, y + 4, x + 7, y + 5, 4);
            break;
        case 2: /* tuning fork: white tines, amber stem, gold base */
            fill_rect(fb, x + 4, y + 1, x + 6, y + 7, 4);
            fill_rect(fb, x + 10, y + 1, x + 12, y + 7, 4);
            fill_rect(fb, x + 7, y + 6, x + 9, y + 9, 10);
            fill_rect(fb, x + 7, y + 9, x + 9, y + 15, color);
            break;
        case 3: /* clock key: gold bow, amber stem, gold wings */
            fill_circle(fb, x + 8, y + 4, 4, color);
            fill_circle(fb, x + 8, y + 4, 2, 11);
            fill_rect(fb, x + 7, y + 7, x + 9, y + 14, 10);
            fill_rect(fb, x + 4, y + 11, x + 6, y + 12, color);
            fill_rect(fb, x + 10, y + 11, x + 12, y + 12, color);
            break;
        case 4: /* final key: white star bow, amber stem, gold teeth */
            fill_circle(fb, x + 5, y + 5, 2, color);
            fill_rect(fb, x + 4, y + 1, x + 6, y + 9, color);
            fill_rect(fb, x + 1, y + 4, x + 9, y + 6, color);
            fill_rect(fb, x + 6, y + 5, x + 14, y + 7, 10);
            fill_rect(fb, x + 11, y + 8, x + 12, y + 10, color);
            fill_rect(fb, x + 13, y + 8, x + 14, y + 9, color);
            break;
        case 5: /* dial: gold ring, amber face, white pointer (param = facing) */
            fill_circle(fb, x + 8, y + 8, 7, color);
            fill_circle(fb, x + 8, y + 8, 5, 10);
            fill_circle(fb, x + 8, y + 8, 1, 11);
            if (param == 0) fill_rect(fb, x + 8, y + 7, x + 13, y + 9, 4);
            else if (param == 1) fill_rect(fb, x + 7, y + 8, x + 9, y + 13, 4);
            else if (param == 2) fill_rect(fb, x + 3, y + 7, x + 8, y + 9, 4);
            else fill_rect(fb, x + 7, y + 3, x + 9, y + 8, 4);
            break;
        case 6: /* lever: dark base + stick; pulled (param) = green, else gold/amber */
            fill_rect(fb, x + 3, y + 12, x + 13, y + 14, 11);
            if (param) {
                fill_rect(fb, x + 7, y + 2, x + 9, y + 12, 6);
                fill_circle(fb, x + 8, y + 2, 3, 6);
                fill_rect(fb, x + 7, y + 1, x + 8, y + 2, 4);
            } else {
                fill_rect(fb, x + 9, y + 4, x + 12, y + 12, color);
                fill_circle(fb, x + 12, y + 3, 3, 10);
                fill_rect(fb, x + 11, y + 2, x + 12, y + 3, 4);
            }
            break;
        case 7: /* bell: gold body, amber top, white highlight, amber clapper */
            fill_rect(fb, x + 6, y + 1, x + 10, y + 3, 10);
            fill_rect(fb, x + 4, y + 4, x + 12, y + 8, color);
            fill_rect(fb, x + 2, y + 9, x + 14, y + 11, color);
            fill_rect(fb, x + 5, y + 4, x + 6, y + 10, 4);
            fill_circle(fb, x + 8, y + 13, 2, 10);
            break;
        case 8: /* piano keys: gold frame, white keys, dark black keys */
            fill_rect(fb, x + 2, y + 3, x + 14, y + 13, color);
            fill_rect(fb, x + 3, y + 4, x + 13, y + 12, 4);
            fill_rect(fb, x + 2, y + 3, x + 14, y + 4, 10);
            fill_rect(fb, x + 2, y + 12, x + 14, y + 13, 10);
            fill_rect(fb, x + 5, y + 5, x + 6, y + 9, 11);
            fill_rect(fb, x + 8, y + 5, x + 9, y + 9, 11);
            fill_rect(fb, x + 11, y + 5, x + 12, y + 9, 11);
            break;
        case 9: /* note: solid white paper, dark staff lines, red seal */
            fill_rect(fb, x + 3, y + 1, x + 13, y + 15, color);
            fill_rect(fb, x + 3, y + 1, x + 13, y + 2, 10);
            fill_rect(fb, x + 5, y + 4, x + 11, y + 5, 11);
            fill_rect(fb, x + 5, y + 7, x + 11, y + 8, 11);
            fill_rect(fb, x + 5, y + 10, x + 9, y + 11, 11);
            fill_circle(fb, x + 11, y + 13, 1, 5);
            break;
        case 10: /* chest: amber wood, dark bands, gold trim, white lock */
            fill_rect(fb, x + 2, y + 4, x + 14, y + 7, 10);
            fill_rect(fb, x + 2, y + 8, x + 14, y + 14, 10);
            fill_rect(fb, x + 2, y + 7, x + 14, y + 8, 11);
            fill_rect(fb, x + 2, y + 4, x + 14, y + 5, color);
            fill_rect(fb, x + 7, y + 6, x + 9, y + 9, 4);
            fill_rect(fb, x + 8, y + 7, x + 8, y + 8, 11);
            break;
        case 11: /* core: gold ring, amber inner, white centre, amber spokes */
            fill_circle(fb, x + 8, y + 8, 7, color);
            fill_circle(fb, x + 8, y + 8, 4, 10);
            fill_circle(fb, x + 8, y + 8, 2, 4);
            fill_rect(fb, x + 7, y + 0, x + 9, y + 3, 10);
            fill_rect(fb, x + 7, y + 13, x + 9, y + 15, 10);
            fill_rect(fb, x + 0, y + 7, x + 3, y + 9, 10);
            fill_rect(fb, x + 13, y + 7, x + 15, y + 9, 10);
            break;
        case 13: /* music stand: white sheet on a dark pole + base */
            fill_rect(fb, x + 3, y + 1, x + 13, y + 9, 4);
            fill_rect(fb, x + 3, y + 1, x + 13, y + 2, 10);
            fill_rect(fb, x + 5, y + 4, x + 11, y + 5, 11);
            fill_rect(fb, x + 5, y + 7, x + 9, y + 8, 11);
            fill_rect(fb, x + 7, y + 9, x + 9, y + 15, 11);
            fill_rect(fb, x + 4, y + 14, x + 12, y + 15, 11);
            break;
        default: /* gate: gold arch + dark bars */
            fill_rect(fb, x + 2, y + 1, x + 14, y + 3, color);
            fill_rect(fb, x + 2, y + 1, x + 4, y + 15, color);
            fill_rect(fb, x + 12, y + 1, x + 14, y + 15, color);
            fill_rect(fb, x + 7, y + 4, x + 9, y + 15, 11);
            break;
    }
}

/* ---- sound: Pumpkin Parade Fix5-style frame-driven PSG driver ----
 * All multi-note sounds are non-blocking: one sequencer step per frame.
 * Original melodies written for this game. */
static void audio_init(void) {
    REG_SOUNDCNT_X = 0x0080;
    /* v7 FIX — the step sound was silent because of this value: SOUNDCNT_L
     * bits 8-11 / 12-15 route PSG channels 1-4 to the left/right speakers
     * (tonc/GBATEK). 0x7777 routes only channels 1-3 (bits 11 and 15, the
     * noise channel's L4/R4 enables, are 0), so the channel-4 noise tick
     * fired into a channel connected to NEITHER speaker — the game was
     * playing a step sound nobody could hear. 0xFF77 routes all four
     * channels to both speakers. (The Ninth Wound's identical tick values
     * are audible there because the GB's NR51 enables all four channels;
     * the values were never the problem — the GBA routing was.) */
    REG_SOUNDCNT_L = (u16)(0xFF77);
    REG_SOUNDCNT_H = 0x0002;
}
static const short NOTE_HZ[12] = {262,278,294,311,330,349,370,392,415,440,466,494};
static int note_hz(int midi) {
    int oct = (midi - 60) / 12, idx = (midi - 60) % 12;
    if (idx < 0) { idx += 12; oct -= 1; }
    int hz = NOTE_HZ[idx];
    while (oct > 0) { hz *= 2; oct--; }
    while (oct < 0) { hz = (hz + 1) / 2; oct++; }
    return hz;
}
static int psg_freq(int midi) {
    int hz = note_hz(midi);
    if (hz < 65) hz = 65;
    return 2048 - (131072 / hz);
}
static void sq_note(int ch, int midi, int vol, int duty) {
    if (ch == 1) {
        REG_SND1CNT_L = 0;
        REG_SND1CNT_H = (u16)((duty << 6) | (vol << 12) | (2 << 8));
        REG_SND1CNT_X = (u16)(psg_freq(midi) | 0x8000);
    } else {
        REG_SND2CNT_L = (u16)((duty << 6) | (vol << 12) | (2 << 8));
        REG_SND2CNT_H = (u16)(psg_freq(midi) | 0x8000);
    }
}
static void sq_stop(void) {
    REG_SND1CNT_H = 0; REG_SND1CNT_X = 0; REG_SND2CNT_L = 0; REG_SND2CNT_H = 0;
}
static void noise_hit(int vol) {
    REG_SND4CNT_L = (u16)((vol << 12) | (2 << 8));
    REG_SND4CNT_H = (u16)(0x8000 | 8);
}

/* Non-blocking jingle sequencer: one note every seq_step frames.
 * seq_is_success marks a puzzle-completion melody (bells, pianos,
 * melody-match): while one is playing, sfx_unlock() must NOT overwrite
 * it with the generic 3-note jingle — that clobber is why v6 played the
 * unlock jingle instead of the full melody on the bells, the Melody
 * Room (via check_gate) and Area 2. */
static int seq_notes[16], seq_len, seq_idx, seq_t, seq_step, seq_is_success;
static void seq_start(const int *notes, int len, int step) {
    int i;
    for (i = 0; i < len && i < 16; i++) seq_notes[i] = notes[i];
    seq_len = len; seq_idx = 0; seq_t = 0; seq_step = step;
    seq_is_success = 0;
}
/* Start a puzzle-success melody: protected from unlock-jingle clobber. */
static void seq_start_success(const int *notes, int len, int step) {
    seq_start(notes, len, step);
    seq_is_success = 1;
}
static int seq_active(void) { return seq_idx < seq_len; }
static void seq_tick(void) {
    if (!seq_active()) return;
    if (--seq_t > 0) return;
    seq_t = seq_step;
    if (seq_notes[seq_idx] == 0) {
        /* v10: rest slot (0) — a short silence gap before the success
         * jingle. Stop the square channel so the last melody note does
         * not ring on through the pause, then wait 10 frames (~167 ms)
         * before the jingle's first note. */
        sq_stop();
        seq_t = 10;
        seq_idx++;
        if (seq_idx >= seq_len) seq_is_success = 0;
        return;
    }
    sq_note(1, seq_notes[seq_idx], 11, 2);
    seq_idx++;
    if (seq_idx >= seq_len) seq_is_success = 0;
}

/* Title waltz "The Music Box House": original, slow, D harmonic minor.
 * 24 steps, one step every 10 frames. Bass root on each bar (every 3). */
static const s8 WALTZ_LEAD[24] = {
    74,69,65, 64,65,67, 69,70,69, 67,65,64,
    74,73,74, 77,76,74, 73,74,69, 65,64,62
};
static const s8 WALTZ_BASS[8] = {50, 45, 46, 45, 50, 43, 45, 50};
/* Ending theme: the waltz returns in D major (F# / B natural). */
static const s8 END_LEAD[24] = {
    74,69,66, 64,66,67, 69,71,69, 67,66,64,
    74,73,74, 77,76,74, 73,74,69, 66,64,62
};
static const s8 END_BASS[8] = {50, 45, 47, 45, 50, 43, 45, 50};
static int mus_step, mus_timer;
static void music_tick(int ending) {
    if (--mus_timer > 0) return;
    mus_timer = 20; /* ~333 ms/note at 60 fps: slow gloomy title tempo (Pumpkin Parade) */
    {
        int st = mus_step % 24;
        const s8 *lead = ending ? END_LEAD : WALTZ_LEAD;
        const s8 *bass = ending ? END_BASS : WALTZ_BASS;
        if (lead[st] >= 0) sq_note(1, lead[st], 8, 2);
        if ((st % 3) == 0) sq_note(2, bass[st / 3], 9, 1);
        mus_step++;
    }
}

/* Jingles (MIDI notes). */
/* Gate jingle (v6): straight RISING D-major scale, 8 notes, slower —
 * Playtesting disliked the rise-fall-rise shape of the 12-note version. */
static const int J_GATE[8] = {62, 64, 66, 67, 69, 71, 73, 74};
static const int J_UNLOCK[3] = {74, 78, 81};
static const int J_KEY[2] = {69, 74};
static const int J_COG[3] = {67, 72, 76};
static const int J_FORK[2] = {76, 81};
static const int J_CLOCK[3] = {72, 69, 76};
static const int J_FINAL[3] = {74, 78, 86};
static const int J_LEVER[2] = {64, 67};
static const int J_DIAL[1] = {76};
static const int J_SCALE[5] = {60, 62, 64, 65, 67};
static const int J_BUZZ[1] = {45};
/* v9: success jingle — FIVE rising major-scale notes (D E F# G A).
 * Distinct from the 8-note gate jingle (same D-major scale, longer)
 * and from the old 3-note unlock jingle (74,78,81): it plays AFTER
 * the full correct melody of a solved musical puzzle, so the player
 * clearly hears the puzzle is solved.
 * v10: transposed ONE OCTAVE UP (D5 E5 F#5 G5 A5 = 74,76,78,79,81)
 * so the success jingle reads as a bright, higher-pitched reward,
 * clearly above the melody that precedes it; and a rest slot (0)
 * is inserted before it in seq_start_melody_success, so the
 * sequencer leaves a short silence gap between the melody and
 * the jingle instead of running them together.
 * v11: FIVE notes -> a FULL EIGHT up the scale (D5..D6 =
 * 74,76,78,79,81,83,85,86), so it ends satisfyingly on the top
 * of the scale.
 * v12: the second-to-last note was 84 (C natural) — wrong for
 * D major, and it sounded off. Fixed to 85 (C#6), so the jingle
 * is a TRUE D-major scale: D E F# G A B C# D. */
static const int J_SUCCESS[8] = {74, 76, 78, 79, 81, 83, 85, 86};
static const int J_PICKUP[5][3] = {
    {69, 74, -1}, {67, 72, 76}, {76, 81, -1}, {72, 69, 76}, {74, 78, 86}
};
static const int J_PICKUP_LEN[5] = {2, 3, 2, 3, 3};
static void sfx_gate(void) { seq_start(J_GATE, 8, 11); }
/* The unlock clunk: if a puzzle-success melody is playing, play only the
 * mechanical noise and leave the melody alone (see seq_is_success).
 * unlock_quiet (v8): a pickup that opens a gate must NOT make the
 * wall-bump noise right before its success sound — collect_item sets
 * this around check_gate so the pickup jingle plays clean.
 * v10: the noise is ALSO skipped while a success melody is playing.
 * A musical solve starts its success sequence and then immediately
 * calls check_gate, which calls sfx_unlock — the noise burst fired
 * over the very start of the melody and read as a high-pitched
 * bump tone right after the solve, even though the melody itself
 * was clean. The success melody IS the feedback there, so the
 * clunk is redundant; the gate still opens silently. The noise
 * stays for dial/lever solves, where no success melody plays and
 * the clunk is the gate-opening sound.
 * v11: that reasoning was wrong — playtesting still heard the clunk as a
 * "bump wall" tone on the gear-dial and lever solves, so the dial
 * and lever handlers no longer call sfx_unlock at all, and
 * check_gate() skips it for areas 0 and 2. Their own feedback
 * (gear direction note / lever clunk-down + message, then the gate
 * jingle on first entry) is enough. The noise + J_UNLOCK now fire
 * nowhere in normal play — sfx_unlock stays as the gate-open hook
 * for the other areas, where seq_is_success / unlock_quiet already
 * silence it. */
static int unlock_quiet = 0;
static int unlock_noise_count = 0; /* v8 QA proof: unlock noises fired */
static void sfx_unlock(void) {
    if (!unlock_quiet && !seq_is_success) { noise_hit(6); unlock_noise_count++; }
    if (!seq_is_success) seq_start(J_UNLOCK, 3, 9);
}
/* v10: opening the final chest must NOT call sfx_unlock — its noise
 * burst plus the high J_UNLOCK arpeggio (74,78,81) is the high-pitched
 * tone heard on chest-open. A soft low click instead: one low
 * square note (D3, midi 50). It touches neither the sequencer (so it
 * cannot overwrite a playing melody) nor the unlock-noise counter.
 * v12: the chest handler now plays that click as the first note of a
 * sequenced success (click, rest, 8-note jingle) via
 * seq_start_jingle_success; this direct-click helper is kept for the
 * seqprobe, which verifies it in isolation. */
static void sfx_chest_open(void) { sq_note(1, 50, 9, 2); }
static void sfx_pickup(int item) { seq_start_success(J_PICKUP[item], J_PICKUP_LEN[item], 8); }
static void sfx_lever(void) { seq_start(J_LEVER, 2, 7); }
/* v7: the note a gear plays depends on the DIRECTION it points to, the
 * same for all four gears — a rising D-major scale as the dial turns
 * EAST 62, SOUTH 64, WEST 66, NORTH 69. (v6 played a different note
 * per GEAR, the same note for every direction — backwards.) */
static const int DIAL_FACE_NOTE[4] = {62, 64, 66, 69};
static int last_dial_note = 0; /* QA proof: last gear note played */
static void sfx_dial_note_face(int face) {
    int s1[1];
    last_dial_note = DIAL_FACE_NOTE[face & 3];
    s1[0] = last_dial_note;
    seq_start(s1, 1, 7);
}
static void sfx_wrong(void) { noise_hit(10); seq_start(J_BUZZ, 1, 10); }
/* v8: a WRONG melody note still plays its own pitch FIRST, so the player
 * learns what note that stand/plate/bell plays — then ONE low tone (45)
 * says the sequence was wrong. No high-pitched noise: the old sfx_wrong
 * noise burst is the hiss heard, so wrong melody notes use this
 * instead (levers keep sfx_wrong, unchanged). */
static void sfx_wrong_note(int note) { int s2[2]; s2[0] = note; s2[1] = 45; seq_start(s2, 2, 12); }
/* v9: a solved musical puzzle plays, in ONE protected sequence:
 * the last note the player just pressed, then the full correct
 * melody, then the eight-note rising success jingle. The old code
 * started the single note and immediately overwrote it with the
 * success melody, so the last note was never heard at all. The
 * combined sequence is at most 1+5+1+5 = 12 notes (seq holds 16).
 * v10: a rest slot (0) sits between the melody and the jingle —
 * seq_tick silences the channel for 10 frames there, so the jingle
 * starts after a clearly audible pause — and the jingle itself is
 * now the higher octave-up J_SUCCESS (74,76,78,79,81).
 * v11: the jingle is now a FULL EIGHT notes up the scale
 * (74,76,78,79,81,83,85,86), ending on the top D6; the combined
 * sequence is at most 1+5+1+8 = 15 notes (seq holds 16).
 * step is the melody tempo (16 in the piano rooms and, since v11,
 * the bells too); the jingle rides the same tempo. */
static void seq_start_melody_success(int last_note, const int *mel, int mel_len, int step) {
    int comb[16], i, n = 0;
    comb[n++] = last_note;
    for (i = 0; i < mel_len && n < 11; i++) comb[n++] = mel[i];
    comb[n++] = 0; /* v10 rest slot: silence gap before the jingle */
    for (i = 0; i < 8 && n < 16; i++) comb[n++] = J_SUCCESS[i];
    seq_start_success(comb, n, step);
}
/* v12: gear/lever/chest solves get the SAME success jingle as the
 * musical puzzles — the last sound first (gear direction note /
 * lever clunk-down / chest soft low click), then the 10-frame rest,
 * then the full 8-note rising J_SUCCESS. No unlock noise anywhere:
 * v11 removed it from these solves and it stays removed; the jingle
 * REPLACES it, it does not sit on top of it. step is the jingle
 * tempo (16, the slow success tempo of the musical puzzles). */
static void seq_start_jingle_success(const int *first, int first_len, int step) {
    int comb[16], i, n = 0;
    for (i = 0; i < first_len && n < 7; i++) comb[n++] = first[i];
    comb[n++] = 0; /* v10 rest slot: silence gap before the jingle */
    for (i = 0; i < 8 && n < 16; i++) comb[n++] = J_SUCCESS[i];
    seq_start_success(comb, n, step);
}
/* QA proof for the step sound: how many step ticks have fired, and the
 * exact noise registers the last one wrote. */
static int step_sfx_count = 0;
static int last_step_l = 0, last_step_h = 0;
static void sfx_step(void) {
    REG_SND4CNT_L = (u16)(0xC130); /* NR41 len 48, NR42 vol 12 */
    REG_SND4CNT_H = (u16)(0xC060); /* NR43 0x60, NR44 trigger + length on */
    last_step_l = REG_SND4CNT_L; last_step_h = REG_SND4CNT_H;
    step_sfx_count++;
}
/* v9 QA proof: how many bump noises have fired. */
static int bump_noise_count = 0;
static void sfx_bump(void) { noise_hit(7); bump_noise_count++; }

static void draw_sprites(volatile u8 *fb, int area, int px, int py, int ang);

/* ---- raycaster (adapted from the engine) ---- */
static int cam_x[240];
static int zbuf[240]; /* wall depth per screen column, for sprite clipping */
static int lever_on[3]; /* area 3 lever states, for bright rendering */
static int gate_open[NAREAS]; /* forward-declared: render_frame colours gates by state */
static int ddx_fn(int a) { return (a < 5 || a > 512) ? 16000 : 65536 / a; }

static int cell_at(int area, int x, int y) {
    char c;
    if (x < 0 || y < 0 || x >= MW || y >= MH) return C_WOOD;
    c = MAPS[area][y][x];
    return cell_code(c);
}

static void render_frame(volatile u8 *fb, int area, int px, int py, int ang) {
    int dir_x = COS256[ang], dir_y = SIN256[ang];
    int plane_x = -(dir_y * 169) / 256;
    int plane_y = (dir_x * 169) / 256;
    int bob = 0; /* instant tile-step: no walking bob */
    int x, y;
    {
        volatile u32 *fb32 = (volatile u32*)fb;
        for (y = 0; y < 80; y++) {
            u32 w = 0x00000000u;
            for (x = 0; x < 60; x++) fb32[y * 60 + x] = w;
        }
        for (y = 80; y < 160; y++) {
            u32 c = ((y >> 2) & 1) ? 2u : 1u;
            u32 w = c | (c << 8) | (c << 16) | (c << 24);
            for (x = 0; x < 60; x++) fb32[y * 60 + x] = w;
        }
    }
    for (x = 0; x < 240; x++) {
        int ray_x = dir_x + plane_x * cam_x[x] / 256;
        int ray_y = dir_y + plane_y * cam_x[x] / 256;
        int map_x = px >> 8, map_y = py >> 8;
        int ddx, ddy, sdx, sdy, step_x, step_y, side = 0, hit = 0, cell = 1;
        int perp, line_h, orig_top, top, bottom, tex_step, tex_pos, tex_x, shade;
        int steps = 0;
        int ax = iabs(ray_x), ay = iabs(ray_y);
        ddx = ddx_fn(ax);
        ddy = ddx_fn(ay);
        if (ray_x < 0) { step_x = -1; sdx = ((px & 255) * ddx) >> 8; }
        else { step_x = 1; sdx = ((256 - (px & 255)) * ddx) >> 8; }
        if (ray_y < 0) { step_y = -1; sdy = ((py & 255) * ddy) >> 8; }
        else { step_y = 1; sdy = ((256 - (py & 255)) * ddy) >> 8; }
        while (!hit && steps < 32) {
            if (sdx < sdy) { sdx += ddx; map_x += step_x; side = 0; }
            else { sdy += ddy; map_y += step_y; side = 1; }
            cell = cell_at(area, map_x, map_y);
            /* only solid walls/gates stop a ray: dials, levers, bells,
             * plates, notes, chests, cores and items are billboard
             * sprites now, drawn in the sprite pass below. */
            if (cell == C_WOOD || cell == C_BRASS || cell == C_DARK ||
                cell == C_GATE || cell == C_BACK) hit = 1;
            steps++;
        }
        perp = (side == 0) ? (sdx - ddx) : (sdy - ddy);
        if (perp < 16) perp = 16;
        zbuf[x] = perp;
        line_h = (perp < 4096) ? 47104 / perp : 144;
        if (line_h > 144) line_h = 144;
        orig_top = (160 - line_h) / 2 + bob;
        top = orig_top < 0 ? 0 : orig_top;
        bottom = orig_top + line_h - 1;
        if (bottom > 159) bottom = 159;
        if (top > 159 || bottom < 0) continue;
        tex_step = (line_h > 0 && line_h <= 144) ? 16384 / line_h : 256;
        tex_pos = (top - orig_top) * tex_step;
        {
            int wall_x;
            if (side == 0) wall_x = py + (perp * ray_y) / 256;
            else wall_x = px + (perp * ray_x) / 256;
            tex_x = (wall_x & 255) >> 2;
        }
        {
            int base = TYPE_BAND[cell];
            if (cell == C_GATE) {
                /* locked gate = reddish band (t=2), open gate = gold (t=1) */
                base = gate_open[area] ? 40 : 64;
                if (perp < 512) shade = base;
                else if (perp < 1024) shade = base + 8;
                else shade = base + 16;
                if (side) shade += 4;
                goto draw_col;
            }
            if (cell == C_BACK) {
                /* back gate is always open: gold */
                base = 40;
                if (perp < 512) shade = base;
                else if (perp < 1024) shade = base + 8;
                else shade = base + 16;
                if (side) shade += 4;
                goto draw_col;
            }
            if (cell == C_LEVER && area == 2) {
                /* pulled levers glow bright regardless of distance */
                int li = map_x - 2;
                if (li == 0 || li == 2 || li == 4) {
                    int idx = (map_x == 2) ? 0 : (map_x == 4) ? 1 : 2;
                    if (lever_on[idx]) shade = base;
                    else shade = (perp < 512) ? base : (perp < 1024) ? base + 8 : base + 16;
                    if (side && !lever_on[idx]) shade += 4;
                    goto draw_col;
                }
            }
            if (perp < 512) shade = base;
            else if (perp < 1024) shade = base + 8;
            else shade = base + 16;
            if (side) shade += 4;
        draw_col:;
            {
                const u8 *tex = (cell == C_WOOD || cell == C_BRASS || cell == C_DARK)
                    ? tex_wall : (cell == C_GATE || cell == C_BACK) ? tex_gate : tex_panel;
                volatile u8 *p = fb + top * 240 + x;
                for (y = top; y <= bottom; y++) {
                    int tex_y = tex_pos >> 8;
                    if (tex_y < 0) tex_y = 0;
                    if (tex_y > 63) tex_y = 63;
                    *p = (u8)(shade + tex[tex_y * 64 + tex_x]);
                    p += 240;
                    tex_pos += tex_step;
                }
            }
        }
    }
    draw_sprites(fb, area, px, py, ang);
}

/* ---- game state ---- */
#define ST_TITLE 0
#define ST_PLAY 1
#define ST_WIN 2
#define ST_NOTE 3
#define ST_INV 4
#define ST_MAP 5

static int state = ST_TITLE;
static int area = 0;
static int ptx = 4, pty = 4, facing = 3; /* 0=east 1=south 2=west 3=north */
static int px = 4 * 256 + 128, py = 4 * 256 + 128, ang = 192;
static int page = 0;
static int frame = 0;
static int inv = 0; /* bitmask of items held */
static int visited[NAREAS];
static int gate_open[NAREAS];
static int gate_jingle_ct[NAREAS];
static int dial_face[4] = {0, 1, 2, 3}; /* area 1 gear dials */
static int dials_solved = 0;
static int mel_prog = 0; /* area 2 melody-match progress 0..4 */
static int mel_solved = 0;
static int melody_prog = 0; /* area 6 (Melody Room): repeat-the-melody 0..5 */
static int melody_solved = 0;
static int lever_pull_ct = 0;
static int levers_solved = 0;
static int scale_prog = 0; /* area 4: 0..5 notes pressed in order */
static int scale_solved = 0;
static int bell_prog = 0; /* area 5: 0..5 bells rung in order */
static int bells_solved = 0;
static int chest_open = 0;
static int won = 0;
static int banner_timer = 0;
static int msg_timer = 0;
static char msg_buf[64];
static int note_lines = 0;
static char note_buf[3][32];

/* dial positions in area 1, in order */
static const int DIAL_X[4] = {2, 6, 2, 6};
static const int DIAL_Y[4] = {1, 1, 6, 6};
/* lever positions in area 3 (large, medium, small) */
static const int LEVER_X[3] = {2, 4, 6};
static const int LEVER_Y[3] = {1, 1, 1};
/* piano plates in area 4: x 2..6, notes C D E F G */
static const int PLATE_NOTE[5] = {60, 62, 64, 65, 67};
/* bells in area 5 (v8): x 2 low C, x 4 med E, x 6 high G — LEFT TO RIGHT
 * IN RISING PITCH ORDER, so the player can read the order off the room
 * (v7 had x 4 = high G in the middle, which contradicted the hint and
 * made the melody feel wrong). Order low-med-high-med-low. */
static const int BELL_NOTE[3] = {60, 64, 67};
static const int BELL_X[3] = {2, 4, 6};
static const int BELL_ORDER[5] = {0, 1, 2, 1, 0}; /* low, med, high, med, low */
static const int BELL_MEL[5] = {60, 64, 67, 64, 60}; /* hint playback */
/* Area 2 music stands: (x, y, note, letter). A is the decoy stand. */
static const int STAND2_X[4] = {2, 4, 6, 4};
static const int STAND2_Y[4] = {1, 1, 1, 7};
static const int STAND2_NOTE[4] = {60, 64, 67, 69}; /* C E G A */
static const char STAND2_LETTER[4] = {'C', 'E', 'G', 'A'};
static const int MEL2[4] = {60, 64, 67, 64}; /* C E G E: the song to play back */
static const int MEL6[5] = {67, 65, 64, 62, 64}; /* G F E D E: Melody Room song */

static const int fdx[4] = {1, 0, -1, 0};
static const int fdy[4] = {0, 1, 0, -1};
static const char *DIR_NAME[4] = {"EAST", "SOUTH", "WEST", "NORTH"};

static int has_item(int it) { return (inv >> it) & 1; }

static void set_msg(const char *s) {
    int i;
    for (i = 0; i < 63 && s[i]; i++) msg_buf[i] = s[i];
    msg_buf[i] = 0;
    msg_timer = 150;
}
static void show_banner(void) { banner_timer = 120; }

/* forward declarations */
static void present(void);
static void check_gate(int a);

/* notes: (area, x, y) -> up to 3 lines */
static int find_note(int a, int x, int y) {
    note_lines = 0;
    if (a == 0 && x == 1 && y == 4) {
        note_lines = 3;
        /* "WHEN ALL FOUR GEARS TURN TO THE SAME NOTE, THE PATH OPENS." */
        { const char *l0 = "WHEN ALL FOUR GEARS"; const char *l1 = "TURN TO THE SAME NOTE,"; const char *l2 = "THE PATH OPENS.";
          int i; for (i = 0; l0[i]; i++) note_buf[0][i] = l0[i]; note_buf[0][i] = 0;
          for (i = 0; l1[i]; i++) note_buf[1][i] = l1[i]; note_buf[1][i] = 0;
          for (i = 0; l2[i]; i++) note_buf[2][i] = l2[i]; note_buf[2][i] = 0; }
        return 1;
    }
    if (a == 1 && x == 1 && y == 1) {
        note_lines = 2;
        { const char *l0 = "THE SONG PLAYS FOUR NOTES."; const char *l1 = "PLAY THEM BACK ON THE STANDS."; int i;
          for (i = 0; l0[i]; i++) note_buf[0][i] = l0[i]; note_buf[0][i] = 0;
          for (i = 0; l1[i]; i++) note_buf[1][i] = l1[i]; note_buf[1][i] = 0; }
        return 1;
    }
    if (a == 1 && x == 7 && y == 1) {
        note_lines = 2;
        { const char *l0 = "LOW, MIDDLE, HIGH,"; const char *l1 = "MIDDLE AGAIN."; int i;
          for (i = 0; l0[i]; i++) note_buf[0][i] = l0[i]; note_buf[0][i] = 0;
          for (i = 0; l1[i]; i++) note_buf[1][i] = l1[i]; note_buf[1][i] = 0; }
        return 1;
    }
    if (a == 2 && x == 1 && y == 1) {
        note_lines = 2;
        { const char *l0 = "BIG TURNS FIRST,"; const char *l1 = "LITTLE LAST."; int i;
          for (i = 0; l0[i]; i++) note_buf[0][i] = l0[i]; note_buf[0][i] = 0;
          for (i = 0; l1[i]; i++) note_buf[1][i] = l1[i]; note_buf[1][i] = 0; }
        return 1;
    }
    if (a == 3 && x == 1 && y == 1) {
        note_lines = 3;
        { const char *l0 = "PLAY THE SCALE, LOW"; const char *l1 = "TO HIGH, AND THE"; const char *l2 = "LID WILL LIFT."; int i;
          for (i = 0; l0[i]; i++) note_buf[0][i] = l0[i]; note_buf[0][i] = 0;
          for (i = 0; l1[i]; i++) note_buf[1][i] = l1[i]; note_buf[1][i] = 0;
          for (i = 0; l2[i]; i++) note_buf[2][i] = l2[i]; note_buf[2][i] = 0; }
        return 1;
    }
    if (a == 4 && x == 1 && y == 1) {
        note_lines = 3;
        { const char *l0 = "LOW, MIDDLE, HIGH,"; const char *l1 = "MIDDLE, LOW -"; const char *l2 = "FIVE NOTES SING."; int i;
          for (i = 0; l0[i]; i++) note_buf[0][i] = l0[i]; note_buf[0][i] = 0;
          for (i = 0; l1[i]; i++) note_buf[1][i] = l1[i]; note_buf[1][i] = 0;
          for (i = 0; l2[i]; i++) note_buf[2][i] = l2[i]; note_buf[2][i] = 0; }
        return 1;
    }
    if (a == 5 && x == 1 && y == 1) {
        note_lines = 3;
        { const char *l0 = "LISTEN WELL."; const char *l1 = "G, F, E, D, E."; const char *l2 = "PLAY IT BACK."; int i;
          for (i = 0; l0[i]; i++) note_buf[0][i] = l0[i]; note_buf[0][i] = 0;
          for (i = 0; l1[i]; i++) note_buf[1][i] = l1[i]; note_buf[1][i] = 0;
          for (i = 0; l2[i]; i++) note_buf[2][i] = l2[i]; note_buf[2][i] = 0; }
        return 1;
    }
    if (a == 6 && x == 2 && y == 4) {
        note_lines = 3;
        { const char *l0 = "THE FINAL KEY SLEEPS"; const char *l1 = "IN THE WINDING ROOM."; const char *l2 = "A CLOCK KEY OPENS ITS CHEST."; int i;
          for (i = 0; l0[i]; i++) note_buf[0][i] = l0[i]; note_buf[0][i] = 0;
          for (i = 0; l1[i]; i++) note_buf[1][i] = l1[i]; note_buf[1][i] = 0;
          for (i = 0; l2[i]; i++) note_buf[2][i] = l2[i]; note_buf[2][i] = 0; }
        return 1;
    }
    return 0;
}

/* gate open conditions per area (forward gate) */
static int gate_condition(int a) {
    switch (a) {
        case 0: return dials_solved && has_item(IT_KEY);
        case 1: return mel_solved && has_item(IT_COG);
        case 2: return levers_solved;
        case 3: return scale_solved && has_item(IT_FORK);
        case 4: return bells_solved && has_item(IT_CLOCK);
        case 5: return melody_solved;
        default: return 0;
    }
}
static const char *gate_hint(int a) {
    if (a == 0) {
        if (!dials_solved) return "THE GEARS MUST AGREE.";
        return "THE GATE NEEDS THE BRASS KEY.";
    }
    if (a == 1) {
        if (!mel_solved) return "PLAY THE SONG BACK.";
        return "THE GATE NEEDS THE BRASS COG.";
    }
    if (a == 2) return "THE LEVERS MUST TURN IN ORDER.";
    if (a == 3) {
        if (!scale_solved) return "PLAY THE SCALE FIRST.";
        return "THE GATE NEEDS THE TUNING FORK.";
    }
    if (a == 4) {
        if (!bells_solved) return "RING THE BELLS IN ORDER.";
        return "THE GATE NEEDS THE CLOCK KEY.";
    }
    if (a == 5) return "REPEAT THE MELODY FIRST.";
    return "THE GATE IS WOUND SHUT.";
}
static void check_gate(int a) {
    if (a < 0 || a >= NAREAS - 1) return;
    if (!gate_open[a] && gate_condition(a)) {
        gate_open[a] = 1;
        /* v11: NO unlock sound for the gear-dial (area 0) and lever
         * (area 2) gates — the noise burst + high J_UNLOCK arpeggio
         * is the "bump wall" tone playtesting still heard on those solves.
         * Their own feedback (gear note / lever clunk-down + message,
         * then the gate jingle on first entry) is enough. Other
         * areas keep sfx_unlock, which is already silent there while
         * a success melody plays (seq_is_success) or on a pickup
         * (unlock_quiet). */
        if (a != 0 && a != 2) sfx_unlock();
        set_msg("SOMETHING WINDS OPEN.");
    }
}

/* focus HUD: what is directly in front of the player */
static void draw_focus(volatile u8 *fb) {
    int fx = ptx + fdx[facing], fy = pty + fdy[facing];
    int code, kind = -1, param = 0, color = 3;
    const char *label = 0;
    char lab[32];
    int i;
    if (fx < 0 || fy < 0 || fx >= MW || fy >= MH) return;
    code = cell_at(area, fx, fy);
    if (code == C_FLOOR) return;
    switch (code) {
        case C_DIAL: {
            int d;
            kind = 5; color = 3;
            for (d = 0; d < 4; d++) if (DIAL_X[d] == fx && DIAL_Y[d] == fy) param = dial_face[d];
            label = "GEAR DIAL - A: TURN";
            break;
        }
        case C_STAND: {
            int s;
            kind = 13; color = 4;
            { const char *pre = "STAND - "; int k = 0; while (pre[k]) { lab[k] = pre[k]; k++; }
              lab[k] = '?';
              for (s = 0; s < 4; s++) if (STAND2_X[s] == fx && STAND2_Y[s] == fy) lab[k] = STAND2_LETTER[s];
              { const char *suf = " - A: PLAY"; int j = 0; k++; while (suf[j]) { lab[k++] = suf[j++]; } lab[k] = 0; } }
            label = lab;
            break;
        }
        case C_LEVER: {
            int l;
            kind = 6; color = 3; param = 0;
            for (l = 0; l < 3; l++) if (LEVER_X[l] == fx) param = lever_on[l];
            label = "LEVER - A: PULL";
            break;
        }
        case C_BELL: {
            /* v8: the focus label names the bell's pitch, so the player
             * can tell which bell to ring to repeat the melody. */
            int b;
            kind = 7; color = 3;
            label = "BELL - A: RING";
            for (b = 0; b < 3; b++) if (BELL_X[b] == fx) {
                label = (b == 0) ? "LOW BELL - A: RING" : (b == 1) ? "MIDDLE BELL - A: RING" : "HIGH BELL - A: RING";
            }
            break;
        }
        case C_PLATE: kind = 8; color = 3; label = "PIANO KEY - A: PLAY"; break;
        case C_NOTE: kind = 9; color = 4; label = "A NOTE - A: READ"; break;
        case C_CHEST: kind = 10; color = 3; label = chest_open ? "OPEN CHEST" : "LOCKED CHEST - A: OPEN"; break;
        case C_CORE: kind = 11; color = 3; label = "CORE - A: USE"; break;
        case C_GATE: kind = 12; color = gate_open[area] ? 3 : 5; label = gate_open[area] ? "OPEN GATE" : "WOUND GATE - A: TRY"; break;
        case C_BACK: kind = 12; color = 3; label = "GATE BACK"; break;
        case C_ITEM: {
            int it = item_type_at(area, fx, fy);
            kind = it; color = 3;
            for (i = 0; ITEM_NAME[it][i]; i++) lab[i] = ITEM_NAME[it][i];
            lab[i] = 0;
            label = lab;
            break;
        }
        default: return;
    }
    fill_rect(fb, 0, 116, 239, 137, 11);
    draw_icon(fb, 8, 120, kind, color, param);
    draw_text(fb, 30, 124, label, 1, 4);
}

/* wrapped message: up to 2 lines of 28 chars (8px font), white on dark slate */
static void draw_msg(volatile u8 *fb) {
    char l0[37], l1[37];
    int n0 = 0, n1 = 0, i = 0, cur = 0;
    l0[0] = 0; l1[0] = 0;
    while (msg_buf[i]) {
        int w0 = i, wlen = 0;
        while (msg_buf[i + wlen] && msg_buf[i + wlen] != ' ') wlen++;
        if (cur == 0) {
            if (n0 > 0 && n0 + 1 + wlen > 28) { cur = 1; continue; }
            if (n0 > 0) l0[n0++] = ' ';
            { int k; for (k = 0; k < wlen && n0 < 28; k++) l0[n0++] = msg_buf[w0 + k]; }
            l0[n0] = 0;
        } else {
            if (n1 > 0 && n1 + 1 + wlen > 28) break;
            if (n1 > 0) l1[n1++] = ' ';
            { int k; for (k = 0; k < wlen && n1 < 28; k++) l1[n1++] = msg_buf[w0 + k]; }
            l1[n1] = 0;
        }
        i += wlen;
        while (msg_buf[i] == ' ') i++;
    }
    fill_rect(fb, 0, 138, 239, 159, 11);
    if (n1 > 0) {
        draw_text(fb, 4, 141, l0, 1, 4);
        draw_text(fb, 4, 151, l1, 1, 4);
    } else {
        draw_text(fb, 4, 147, l0, 1, 4);
    }
}

/* overlays drawn on the presented frame */
static void draw_overlays(volatile u8 *fb) {
    if (banner_timer > 0) {
        fill_rect(fb, 0, 18, 239, 34, 11);
        draw_text_c(fb, 23, AREA_NAME[area], 1, 4);
    }
    if (msg_timer > 0) draw_msg(fb);
    draw_focus(fb);
}

/* render + flip + overlays */
static void present(void) {
    volatile u8 *fb;
    page = page ? 0 : 1;
    fb = page ? FB1 : FB0;
    render_frame(fb, area, px, py, ang);
    draw_overlays(fb); /* overlays into the HIDDEN page, before the flip */
    REG_DISPCNT = (u16)(4 | 0x0400 | (page ? 0x0010 : 0));
}

static void draw_music_note(volatile u8 *fb, int x, int y, int color) {
    fill_circle(fb, x + 3, y + 10, 3, color);
    fill_rect(fb, x + 5, y + 1, x + 6, y + 10, color);
    fill_rect(fb, x + 6, y + 1, x + 10, y + 4, color);
}
static void draw_gear(volatile u8 *fb, int cx, int cy, int r, int color, int bg) {
    fill_rect(fb, cx - 2, cy - r - 3, cx + 2, cy - r + 1, color);
    fill_rect(fb, cx - 2, cy + r - 1, cx + 2, cy + r + 3, color);
    fill_rect(fb, cx - r - 3, cy - 2, cx - r + 1, cy + 2, color);
    fill_rect(fb, cx + r - 1, cy - 2, cx + r + 3, cy + 2, color);
    fill_circle(fb, cx, cy, r, color);
    fill_circle(fb, cx, cy, r - 3, bg);
    fill_circle(fb, cx, cy, 1, color);
}
static void draw_title(void) {
    volatile u8 *fb = FB0;
    int i, k;
    page = 0;
    REG_DISPCNT = 0; /* blank while drawing: no tearing on the title screen */
    for (i = 0; i < 240 * 160; i++) fb[i] = 14; /* lavender background */
    /* gold frame */
    fill_rect(fb, 0, 0, 239, 2, 3);
    fill_rect(fb, 0, 157, 239, 159, 3);
    fill_rect(fb, 0, 0, 2, 159, 3);
    fill_rect(fb, 237, 0, 239, 159, 3);
    draw_text_c(fb, 10, "THE MUSIC BOX", 2, 7);
    draw_text_c(fb, 30, "HOUSE", 2, 7);
    draw_text_c(fb, 50, "A CLOCKWORK ESCAPE ROOM", 1, 11);
    /* floating notes + gears around the music box */
    draw_music_note(fb, 34, 56, 4);
    draw_music_note(fb, 196, 56, 4);
    draw_music_note(fb, 56, 34, 13);
    draw_music_note(fb, 174, 34, 13);
    draw_gear(fb, 52, 92, 9, 3, 14);
    draw_gear(fb, 188, 92, 9, 3, 14);
    /* the music box itself: pink lid, cream body, gold trim, piano keys */
    fill_rect(fb, 72, 58, 167, 66, 12);
    fill_rect(fb, 72, 58, 167, 59, 3);
    fill_rect(fb, 78, 67, 161, 116, 15);
    fill_rect(fb, 78, 67, 161, 68, 3);
    fill_rect(fb, 78, 115, 161, 116, 3);
    fill_rect(fb, 78, 67, 79, 116, 3);
    fill_rect(fb, 160, 67, 161, 116, 3);
    /* crank on the right side */
    fill_rect(fb, 162, 84, 172, 88, 3);
    fill_circle(fb, 175, 86, 4, 3);
    fill_circle(fb, 175, 86, 2, 14);
    /* little dancers' notes on the body */
    draw_music_note(fb, 92, 72, 12);
    draw_music_note(fb, 128, 72, 13);
    /* piano-key strip along the bottom of the box */
    for (k = 0; k < 8; k++) {
        fill_rect(fb, 80 + k * 10, 104, 88 + k * 10, 114, 4);
        if (k < 7) fill_rect(fb, 87 + k * 10, 104, 89 + k * 10, 110, 11);
    }
    draw_text_c(fb, 124, "PRESS START", 1, 7);
    draw_text_c(fb, 134, "UP: STEP  DOWN: BACK", 1, 11);
    draw_text_c(fb, 144, "LEFT/RIGHT: TURN  A: USE", 1, 11);
    draw_text_c(fb, 152, "START: ITEMS  TRIJAM #397", 1, 11);
    REG_DISPCNT = (u16)(4 | 0x0400);
}
static void draw_win(void) {
    REG_DISPCNT = 0; /* blank while drawing */
    volatile u8 *fb = FB0;
    int i, k;
    page = 0;
    for (i = 0; i < 240 * 160; i++) fb[i] = 14; /* lavender, like the title */
    /* gold frame */
    fill_rect(fb, 0, 0, 239, 2, 3);
    fill_rect(fb, 0, 157, 239, 159, 3);
    fill_rect(fb, 0, 0, 2, 159, 3);
    fill_rect(fb, 237, 0, 239, 159, 3);
    draw_text_c(fb, 12, "YOU ESCAPED!", 2, 7);
    draw_text_c(fb, 32, "THE HOUSE PLAYS ON", 1, 7);
    /* celebratory music box + gears + floating notes */
    draw_music_note(fb, 30, 52, 12);
    draw_music_note(fb, 200, 52, 13);
    draw_music_note(fb, 52, 40, 4);
    draw_music_note(fb, 178, 40, 4);
    draw_gear(fb, 48, 96, 9, 3, 14);
    draw_gear(fb, 192, 96, 9, 3, 14);
    fill_rect(fb, 72, 58, 167, 66, 12);
    fill_rect(fb, 72, 58, 167, 59, 3);
    fill_rect(fb, 78, 67, 161, 116, 15);
    fill_rect(fb, 78, 67, 161, 68, 3);
    fill_rect(fb, 78, 115, 161, 116, 3);
    fill_rect(fb, 78, 67, 79, 116, 3);
    fill_rect(fb, 160, 67, 161, 116, 3);
    fill_rect(fb, 162, 84, 172, 88, 3);
    fill_circle(fb, 175, 86, 4, 3);
    fill_circle(fb, 175, 86, 2, 14);
    draw_music_note(fb, 92, 72, 12);
    draw_music_note(fb, 128, 72, 13);
    for (k = 0; k < 8; k++) {
        fill_rect(fb, 80 + k * 10, 104, 88 + k * 10, 114, 4);
        if (k < 7) fill_rect(fb, 87 + k * 10, 104, 89 + k * 10, 110, 11);
    }
    /* confetti stars */
    for (k = 0; k < 12; k++) {
        int cx = 16 + (k * 37) % 208, cy = 44 + (k * 23) % 100;
        fill_rect(fb, cx, cy, cx + 1, cy + 1, (k & 1) ? 12 : 13);
    }
    draw_text_c(fb, 124, "PRESS START", 1, 7);
    draw_text_c(fb, 134, "THE MUSIC BOX HOUSE", 1, 7);
    REG_DISPCNT = (u16)(4 | 0x0400);
}
static void draw_note_screen(void) {
    REG_DISPCNT = 0; /* blank while drawing */
    volatile u8 *fb = FB0;
    int i, y = 56;
    page = 0;
    for (i = 0; i < 240 * 160; i++) fb[i] = 11;
    draw_text_c(fb, 24, "A NOTE", 1, 3);
    for (i = 0; i < note_lines; i++) {
        draw_text_c(fb, y, note_buf[i], 1, 4);
        y += 12;
    }
    draw_text_c(fb, 140, "A: CONTINUE", 1, 3);
    REG_DISPCNT = (u16)(4 | 0x0400);
}
static void draw_inventory(void) {
    REG_DISPCNT = 0; /* blank while drawing */
    volatile u8 *fb = FB0;
    int i, y = 44, count = 0;
    page = 0;
    for (i = 0; i < 240 * 160; i++) fb[i] = 11;
    draw_text_c(fb, 16, "INVENTORY", 1, 3);
    for (i = 0; i < 5; i++) {
        if (has_item(i)) {
            draw_icon(fb, 28, y - 4, i, 3, 0);
            draw_text(fb, 52, y, ITEM_NAME[i], 1, 4);
            y += 20;
            count++;
        }
    }
    if (!count) draw_text_c(fb, 60, "NOTHING YET", 1, 4);
    draw_text_c(fb, 140, "START: CLOSE", 1, 3);
    REG_DISPCNT = (u16)(4 | 0x0400);
}
static void draw_minimap(void) {
    REG_DISPCNT = 0; /* blank while drawing */
    volatile u8 *fb = FB0;
    int i, x, y;
    page = 0;
    for (i = 0; i < 240 * 160; i++) fb[i] = 0;
    fill_rect(fb, 0, 4, 239, 20, 11);
    draw_text_c(fb, 8, AREA_NAME[area], 1, 4);
    for (y = 0; y < MH; y++) {
        for (x = 0; x < MW; x++) {
            int code = cell_at(area, x, y);
            int cx = 70 + x * 11, cy = 28 + y * 11;
            int col = 1;
            if (code == C_WOOD) col = 10;
            else if (code == C_GATE) col = gate_open[area] ? 3 : 5;
            else if (code == C_BACK) col = 3;
            else if (code == C_ITEM) col = 3;
            else if (code != C_FLOOR) col = 7;
            fill_rect(fb, cx, cy, cx + 9, cy + 9, col);
        }
    }
    /* player arrow */
    {
        int cx = 70 + ptx * 11 + 5, cy = 28 + pty * 11 + 5;
        if (facing == 0) { fill_rect(fb, cx - 2, cy - 1, cx + 4, cy + 1, 4); fill_rect(fb, cx + 3, cy - 3, cx + 3, cy + 3, 4); }
        else if (facing == 1) { fill_rect(fb, cx - 1, cy - 2, cx + 1, cy + 4, 4); fill_rect(fb, cx - 3, cy + 3, cx + 3, cy + 3, 4); }
        else if (facing == 2) { fill_rect(fb, cx - 4, cy - 1, cx + 2, cy + 1, 4); fill_rect(fb, cx - 3, cy - 3, cx - 3, cy + 3, 4); }
        else { fill_rect(fb, cx - 1, cy - 4, cx + 1, cy + 2, 4); fill_rect(fb, cx - 3, cy - 3, cx + 3, cy - 3, 4); }
    }
    fill_rect(fb, 0, 136, 239, 152, 11);
    draw_text_c(fb, 140, "SELECT: CLOSE", 1, 4);
    REG_DISPCNT = (u16)(4 | 0x0400);
}

/* ---- billboard sprites (Ninth Wound style) ----
 * Every interactable / item / note is a floor-anchored sprite projected
 * into camera space and drawn over the walls, clipped by the per-column
 * wall depth (zbuf). Levers render at visibly different sizes:
 * large / medium / small, in puzzle order. */
static u8 icon_buf[256];
static void draw_icon_buf(int kind, int color, int param) {
    int i;
    for (i = 0; i < 256; i++) icon_buf[i] = 255;
    fb_stride = 16; fb_h = 16;
    draw_icon(icon_buf, 0, 0, kind, color, param);
    fb_stride = 240; fb_h = 160;
}
static void draw_sprites(volatile u8 *fb, int area, int px, int py, int ang) {
    int dir_x = COS256[ang], dir_y = SIN256[ang];
    int sx[32], sy[32], sd[32], sk[32], sc[32], sp[32], ss[32], srx[32], sry[32];
    int n = 0, x, y, i, j;
    for (y = 0; y < MH; y++) for (x = 0; x < MW; x++) {
        int code = cell_at(area, x, y);
        int kind = -1, color = 3, param = 0, scale = 10, d, l;
        switch (code) {
            case C_DIAL:
                kind = 5;
                for (d = 0; d < 4; d++) if (DIAL_X[d] == x && DIAL_Y[d] == y) param = dial_face[d];
                break;
            case C_STAND: kind = 13; color = 4; scale = 9; break;
            case C_LEVER:
                kind = 6; scale = 10;
                for (l = 0; l < 3; l++) if (LEVER_X[l] == x) {
                    param = lever_on[l];
                    scale = (l == 0) ? 12 : (l == 1) ? 10 : 8; /* large/medium/small */
                }
                break;
            case C_BELL: {
                /* v8: bells render in rising-pitch size order — smallest
                 * = lowest note (x 2), largest = highest (x 6) — so the
                 * player can see which bell to ring. */
                int b;
                kind = 7;
                for (b = 0; b < 3; b++) if (BELL_X[b] == x) scale = (b == 0) ? 8 : (b == 1) ? 10 : 12;
                break;
            }
            case C_PLATE: kind = 8; break;
            case C_NOTE: kind = 9; color = 4; scale = 8; break;
            case C_CHEST: kind = 10; scale = 12; break;
            case C_CORE: kind = 11; scale = 12; break;
            case C_ITEM: {
                int it = item_type_at(area, x, y);
                kind = it;
                color = (it == IT_FINAL) ? 4 : 3; /* final key: bright white */
                scale = 9;
                break;
            }
            default: continue;
        }
        if (n < 32) {
            int relx = (x * 256 + 128) - px, rely = (y * 256 + 128) - py;
            int depth = (relx * dir_x + rely * dir_y) >> 8;
            if (depth <= 64) continue;
            sx[n] = x; sy[n] = y; sd[n] = depth; sk[n] = kind; sc[n] = color;
            sp[n] = param; ss[n] = scale; srx[n] = relx; sry[n] = rely;
            n++;
        }
    }
    /* far -> near so nearer sprites paint over farther ones */
    for (i = 0; i < n; i++) for (j = i + 1; j < n; j++) {
        if (sd[j] > sd[i]) {
            int t;
            t = sd[i]; sd[i] = sd[j]; sd[j] = t; t = sk[i]; sk[i] = sk[j]; sk[j] = t;
            t = sc[i]; sc[i] = sc[j]; sc[j] = t; t = sp[i]; sp[i] = sp[j]; sp[j] = t;
            t = ss[i]; ss[i] = ss[j]; ss[j] = t; t = srx[i]; srx[i] = srx[j]; srx[j] = t;
            t = sry[i]; sry[i] = sry[j]; sry[j] = t;
        }
    }
    for (i = 0; i < n; i++) {
        int depth = sd[i];
        int lh = 47104 / depth; if (lh > 144) lh = 144; if (lh < 8) lh = 8;
        /* v5: sprites roughly half the v4 height (divisor 20, not 10) and
         * capped at 80px, so they read as believable objects, not giants. */
        int h = lh * ss[i] / 20; if (h > 80) h = 80; if (h < 4) h = 4;
        int w = h;
        /* lateral offset along the camera-right vector (-dir_y, dir_x) */
        int lat = (srx[i] * (-dir_y) + sry[i] * dir_x) >> 8;
        int scrx = 120 + (30720 * lat) / (depth * 169);
        int left = scrx - w / 2;
        int top = 80 + lh / 2 - h; /* floor-anchored: base sits on the floor */
        int xx, yy;
        int step_x, step_y, ty_fp;
        if (depth > 2048) continue; /* too far to matter */
        if (left + w < 0 || left >= 240 || top + h < 0 || top >= 160) continue;
        draw_icon_buf(sk[i], sc[i], sp[i]);
        /* fixed-point texel steps: NO per-pixel divisions in the hot path
         * (v4 did yy*16/h and xx*16/w per pixel — the lag). */
        step_x = (16 << 8) / w;
        step_y = (16 << 8) / h;
        ty_fp = 0;
        for (yy = 0; yy < h; yy++) {
            int py2 = top + yy, ty = ty_fp >> 8, tx_fp = 0;
            ty_fp += step_y;
            if (py2 < 0 || py2 >= 160) continue;
            for (xx = 0; xx < w; xx++) {
                int px2 = left + xx, tx = tx_fp >> 8;
                u8 c;
                tx_fp += step_x;
                if (px2 < 0 || px2 >= 240) continue;
                if (depth >= zbuf[px2]) continue; /* hidden behind a wall */
                c = icon_buf[ty * 16 + tx];
                if (c == 255) continue;
                fb[py2 * 240 + px2] = c;
            }
        }
    }
}

/* ---- interactions ---- */
static void interact(void) {
    int fx = ptx + fdx[facing], fy = pty + fdy[facing];
    int code;
    if (fx < 0 || fy < 0 || fx >= MW || fy >= MH) return;
    code = cell_at(area, fx, fy);
    switch (code) {
        case C_NOTE:
            if (find_note(area, fx, fy)) {
                /* sheet music PLAYS its song so the player can learn it.
                 * v8: the Bell Tower hint and the first piano room's hint
                 * do the same — read the note, hear the sequence. */
                if (area == 1 && fx == 1 && fy == 1) seq_start(MEL2, 4, 10);
                if (area == 3 && fx == 1 && fy == 1) seq_start(J_SCALE, 5, 10);
                if (area == 4 && fx == 1 && fy == 1) seq_start(BELL_MEL, 5, 10);
                if (area == 5 && fx == 1 && fy == 1) seq_start(MEL6, 5, 10);
                state = ST_NOTE; draw_note_screen();
            }
            break;
        case C_DIAL: {
            int d;
            for (d = 0; d < 4; d++) {
                if (DIAL_X[d] == fx && DIAL_Y[d] == fy) {
                    dial_face[d] = (dial_face[d] + 1) & 3;
                    /* the note follows the direction the gear now points
                     * to — same mapping on every gear */
                    sfx_dial_note_face(dial_face[d]);
                    set_msg("THE GEAR TURNS.");
                    if (dial_face[0] == dial_face[1] && dial_face[1] == dial_face[2] && dial_face[2] == dial_face[3] && !dials_solved) {
                        dials_solved = 1;
                        /* v11: NO sfx_unlock here — its noise burst +
                         * high J_UNLOCK arpeggio is the "bump wall"
                         * tone heard on the gear solve.
                         * v12: the solve DOES get a success signal —
                         * the last gear note first, then the pause,
                         * then the 8-note success jingle (same as the
                         * musical puzzles). No unlock noise. */
                        {
                            int s1[1]; s1[0] = last_dial_note;
                            seq_start_jingle_success(s1, 1, 16);
                        }
                        set_msg("THE GEARS CLICK INTO TUNE.");
                        check_gate(0);
                    }
                    break;
                }
            }
            break;
        }
        case C_STAND: {
            int s;
            for (s = 0; s < 4; s++) {
                if (STAND2_X[s] == fx && STAND2_Y[s] == fy) {
                    int note = STAND2_NOTE[s];
                    int single[1]; single[0] = note;
                    if (mel_solved) { seq_start(single, 1, 7); break; }
                    if (note == MEL2[mel_prog]) {
                        mel_prog++;
                        if (mel_prog == 4) {
                            mel_solved = 1;
                            /* v9: last note (E 64) sounds first, then the
                             * full song, then the 8-note success jingle.
                             * The old code started the single note and
                             * immediately overwrote it with the song. */
                            seq_start_melody_success(note, MEL2, 4, 16);
                            set_msg("THE SONG RINGS TRUE.");
                            check_gate(1);
                        } else {
                            seq_start(single, 1, 7);
                        }
                    } else {
                        mel_prog = 0;
                        sfx_wrong_note(note);
                        set_msg("WRONG NOTE. THE SONG RESETS.");
                    }
                    break;
                }
            }
            break;
        }
        case C_LEVER: {
            int l;
            for (l = 0; l < 3; l++) {
                if (LEVER_X[l] == fx) {
                    if (lever_on[l]) break; /* already pulled */
                    if (l == lever_pull_ct) {
                        lever_on[l] = 1;
                        lever_pull_ct++;
                        sfx_lever();
                        set_msg("THE LEVER CLUNKS DOWN.");
                        if (lever_pull_ct == 3) {
                            levers_solved = 1;
                            /* v11: NO sfx_unlock here — same "bump
                             * wall" tone as the gear solve.
                             * v12: the solve DOES get a success
                             * signal — the lever clunk-down first,
                             * then the pause, then the 8-note success
                             * jingle (same as the musical puzzles).
                             * No unlock noise. */
                            seq_start_jingle_success(J_LEVER, 2, 16);
                            set_msg("THE GEARS GRIND INTO PLACE.");
                            check_gate(2);
                        }
                    } else {
                        /* wrong order: buzz + reset */
                        lever_on[0] = lever_on[1] = lever_on[2] = 0;
                        lever_pull_ct = 0;
                        sfx_wrong();
                        set_msg("WRONG ORDER. THE LEVERS RESET.");
                    }
                    break;
                }
            }
            break;
        }
        case C_PLATE: {
            int p = fx - 2;
            if (p >= 0 && p < 5) {
                int note = PLATE_NOTE[p];
                int single[1];
                single[0] = note;
                if (area == 5) {
                    /* Melody Room: repeat G-F-E-D-E (learned from the note) */
                    if (melody_solved) { seq_start(single, 1, 7); break; }
                    if (note == MEL6[melody_prog]) {
                        melody_prog++;
                        if (melody_prog == 5) {
                            melody_solved = 1;
                            /* v9: last note (E 64) sounds first, then the
                             * full melody, then the 8-note success jingle
                             * (step 16, the slow piano-room tempo). */
                            seq_start_melody_success(note, MEL6, 5, 16);
                            set_msg("THE MELODY RINGS TRUE.");
                            check_gate(5);
                        } else {
                            seq_start(single, 1, 7);
                        }
                    } else {
                        melody_prog = 0;
                        sfx_wrong_note(note);
                        set_msg("WRONG NOTE. THE MELODY RESETS.");
                    }
                    break;
                }
                if (scale_solved) { seq_start(single, 1, 7); break; }
                if (p == scale_prog) {
                    scale_prog++;
                    if (scale_prog == 5) {
                        scale_solved = 1;
                        /* v9: last note (G 67) sounds first, then the
                         * full scale, then the 8-note success jingle
                         * (step 16, the slow piano-room tempo). */
                        seq_start_melody_success(note, J_SCALE, 5, 16);
                        set_msg("THE LID LIFTS. A FORK SHINES.");
                        MAPS[3][4][4] = 'T'; /* tuning fork appears at centre */
                        check_gate(3);
                    } else {
                        seq_start(single, 1, 7);
                    }
                } else {
                    scale_prog = 0;
                    sfx_wrong_note(note);
                    set_msg("WRONG NOTE. THE SCALE RESETS.");
                }
            }
            break;
        }
        case C_BELL: {
            int b;
            for (b = 0; b < 3; b++) {
                if (BELL_X[b] == fx) {
                    int note = BELL_NOTE[b];
                    int seq[5];
                    int i;
                    if (bells_solved) { int s1[1]; s1[0] = note; seq_start(s1, 1, 7); break; }
                    if (b == BELL_ORDER[bell_prog]) {
                        bell_prog++;
                        if (bell_prog == 5) {
                            bells_solved = 1;
                            /* v9: last bell (low C 60) sounds first, then
                             * the FULL five-bell melody, then the 8-note
                             * success jingle — one protected sequence.
                             * v11: step 16 now, the slow piano-room
                             * tempo — the bells' success music played
                             * too fast at step 8. The growing melody
                             * after each intermediate bell stays at 8. */
                            { int full[5], i2; for (i2 = 0; i2 < 5; i2++) full[i2] = BELL_NOTE[BELL_ORDER[i2]]; seq_start_melody_success(note, full, 5, 16); }
                            set_msg("THE BELLS SING TOGETHER.");
                            MAPS[4][4][4] = 'c'; /* clock key appears at centre */
                            check_gate(4);
                        } else {
                            /* growing melody: the bells rung so far, in order */
                            for (i = 0; i < bell_prog; i++) seq[i] = BELL_NOTE[BELL_ORDER[i]];
                            seq_start(seq, bell_prog, 8);
                        }
                    } else {
                        bell_prog = 0;
                        sfx_wrong_note(note);
                        set_msg("WRONG BELL. THE TUNE RESETS.");
                    }
                    break;
                }
            }
            break;
        }
        case C_CHEST:
            if (chest_open) { set_msg("THE CHEST STANDS OPEN."); break; }
            if (has_item(IT_CLOCK)) {
                chest_open = 1;
                MAPS[0][4][4] = 'F'; /* final key appears in the CENTRE of the room */
                /* v10: soft low click, not sfx_unlock's noise + high
                 * jingle. v12: the click is the first note of a
                 * success sequence — click, pause, then the 8-note
                 * success jingle (same as the gear/lever solves).
                 * No unlock noise. */
                {
                    int s1[1]; s1[0] = 50;
                    seq_start_jingle_success(s1, 1, 16);
                }
                set_msg("CHEST OPEN! FINAL KEY IS IN THE MIDDLE. STEP ON IT!");
            } else {
                set_msg("LOCKED. A CLOCK-SHAPED KEYHOLE.");
            }
            break;
        case C_CORE:
            if (has_item(IT_FORK) && has_item(IT_FINAL)) {
                won = 1;
                state = ST_WIN;
                sq_stop();
                seq_len = 0; seq_idx = 0;
                mus_step = 0; mus_timer = 0;
                draw_win();
            } else if (!has_item(IT_FORK)) {
                set_msg("THE CORE NEEDS A TUNING FORK.");
            } else {
                set_msg("CORE NEEDS THE FINAL KEY. CHECK THE WINDING ROOM.");
            }
            break;
        case C_GATE:
            if (gate_open[area]) set_msg("THE GATE STANDS OPEN.");
            else set_msg(gate_hint(area));
            break;
        default:
            break;
    }
}

static void collect_item(int it) {
    inv |= (1 << it);
    sfx_pickup(it);
    {
        char m[32];
        int i = 0, j;
        const char *pre = "PICKED UP: ";
        while (pre[i]) { m[i] = pre[i]; i++; }
        for (j = 0; ITEM_NAME[it][j] && i < 30; j++) m[i++] = ITEM_NAME[it][j];
        m[i] = 0;
        set_msg(m);
    }
    /* v8: a pickup that opens a gate must not fire the unlock noise
     * (the wall-bump sound) over its own pickup jingle. */
    unlock_quiet = 1;
    check_gate(area);
    unlock_quiet = 0;
}

/* area transition: dir +1 forward gate, -1 back gate */
static void transition(int dir) {
    if (dir > 0) {
        area++;
        ptx = 1; pty = 4; facing = 0; ang = 0;
    } else {
        area--;
        ptx = 7; pty = 4; facing = 2; ang = 128;
    }
    px = ptx * 256 + 128; py = pty * 256 + 128;
    build_palette(area); /* each room has its own pastel palette */
    if (!visited[area]) {
        visited[area] = 1;
        gate_jingle_ct[area]++;
        sfx_gate();
        show_banner();
    }
}

static void sram_dump(void) {
    int i;
    SRAM[0] = 'M'; SRAM[1] = 'B';
    SRAM[2] = (u8)state;
    SRAM[3] = (u8)area;
    SRAM[4] = (u8)ptx; SRAM[5] = (u8)pty; SRAM[6] = (u8)facing;
    SRAM[7] = (u8)(ang & 255);
    SRAM[8] = (u8)((frame >> 8) & 255); SRAM[9] = (u8)(frame & 255);
    SRAM[10] = (u8)inv;
    SRAM[11] = (u8)dials_solved; SRAM[12] = (u8)levers_solved;
    SRAM[13] = (u8)scale_solved; SRAM[14] = (u8)bells_solved;
    SRAM[15] = (u8)chest_open; SRAM[16] = (u8)won;
    SRAM[17] = (u8)(dial_face[0] | (dial_face[1] << 2) | (dial_face[2] << 4) | (dial_face[3] << 6));
    SRAM[18] = (u8)(lever_on[0] | (lever_on[1] << 1) | (lever_on[2] << 2) | (lever_pull_ct << 3));
    SRAM[19] = (u8)scale_prog; SRAM[20] = (u8)bell_prog; SRAM[21] = (u8)mel_prog;
    for (i = 0; i < NAREAS; i++) SRAM[22 + i] = (u8)gate_open[i];
    for (i = 0; i < NAREAS; i++) SRAM[29 + i] = (u8)gate_jingle_ct[i];
    SRAM[36] = (u8)seq_active();
    SRAM[37] = (u8)banner_timer;
    SRAM[38] = (u8)mel_solved;
    SRAM[39] = (u8)melody_solved;
    SRAM[40] = (u8)melody_prog;
    /* v7 QA proof fields */
    SRAM[41] = (u8)last_dial_note;              /* last gear note played */
    SRAM[42] = (u8)(step_sfx_count & 255);      /* step ticks fired (lo) */
    SRAM[43] = (u8)((step_sfx_count >> 8) & 255); /* step ticks (hi) */
    SRAM[44] = (u8)seq_len;                     /* sequencer length */
    for (i = 0; i < 5; i++) SRAM[45 + i] = (u8)seq_notes[i]; /* sequencer notes */
    SRAM[50] = (u8)((REG_SOUNDCNT_L >> 8) & 255); /* expect 0xFF: ch4 routed */
    SRAM[51] = (u8)((last_step_l >> 8) & 255);  /* expect 0xC1 (NR42) */
    SRAM[52] = (u8)(last_step_l & 255);         /* expect 0x30 (NR41) */
    SRAM[53] = (u8)((last_step_h >> 8) & 255);  /* expect 0xC0 (NR44) */
    SRAM[54] = (u8)(last_step_h & 255);         /* expect 0x60 (NR43) */
    /* v8 QA proof fields */
    SRAM[55] = (u8)(unlock_noise_count & 255);  /* unlock noises fired */
    SRAM[56] = (u8)seq_step;                    /* sequencer tempo (frames/note) */
    /* v9 QA proof fields: the combined success sequence can be up to
     * 11 notes, so expose notes 5..10 too (notes 0..4 are SRAM[45..49]);
     * SRAM[63] counts bump noises, to prove musical items don't bump.
     * v10: the sequence is up to 12 notes (rest slot + higher jingle),
     * so expose note 11 at SRAM[64] as well.
     * v11: the jingle is now 8 notes, so the sequence is up to 15 —
     * expose notes 12..14 at SRAM[65..67] as well. */
    for (i = 0; i < 6; i++) SRAM[57 + i] = (u8)seq_notes[5 + i];
    SRAM[64] = (u8)seq_notes[11]; /* v10: 12th note of the success sequence */
    for (i = 0; i < 3; i++) SRAM[65 + i] = (u8)seq_notes[12 + i]; /* v11: notes 12..14 */
    SRAM[63] = (u8)(bump_noise_count & 255);    /* bump noises fired */
}

/* Full new-game reset (v6 bug fix: after winning and starting again, the
 * Level 1 gears would not unlock the gate because dial/gate/inventory
 * state from the finished game was never cleared). */
static void reset_game(void) {
    int i;
    dial_face[0] = 0; dial_face[1] = 1; dial_face[2] = 2; dial_face[3] = 3;
    dials_solved = 0;
    mel_prog = 0; mel_solved = 0;
    lever_on[0] = lever_on[1] = lever_on[2] = 0;
    lever_pull_ct = 0; levers_solved = 0;
    scale_prog = 0; scale_solved = 0;
    bell_prog = 0; bells_solved = 0;
    melody_prog = 0; melody_solved = 0;
    chest_open = 0; won = 0; inv = 0;
    for (i = 0; i < NAREAS; i++) { gate_open[i] = 0; gate_jingle_ct[i] = 0; visited[i] = 0; }
    banner_timer = 0; msg_timer = 0; msg_buf[0] = 0;
    mus_step = 0; mus_timer = 0;
    seq_len = 0; seq_idx = 0; seq_t = 0;
    /* restore item/spawn tiles the previous game collected or spawned */
    MAPS[0][7][7] = 'K';
    MAPS[1][7][7] = 'k';
    MAPS[0][4][4] = '0';
    MAPS[3][4][4] = '0';
    MAPS[4][4][4] = '0';
}

int main(void) {
    int prev_keys = 0, bump_timer = 0, c;
    int idle_frames = 0;
    int prev_banner = 0, prev_msg = 0;
    build_palette(0);
    build_textures();
    audio_init();
    for (c = 0; c < 240; c++) cam_x[c] = ((2 * c - 239) * 256) / 239;
    draw_title();
    while (1) {
        int keys, pressed, held;
        vsync();
        keys = (~REG_KEY) & 0x03FF;
        pressed = keys & ~prev_keys;
        prev_keys = keys;
        held = keys;
        /* audio: jingle overrides music; music only on title/win screens */
        if (seq_active()) seq_tick();
        else if (state == ST_TITLE) music_tick(0);
        else if (state == ST_WIN) music_tick(1);
        if (banner_timer > 0) banner_timer--;
        if (msg_timer > 0) msg_timer--;
        if (state == ST_TITLE) {
            if (pressed & KEY_START) {
                reset_game();
                area = 0; ptx = 4; pty = 4; facing = 3;
                px = 4 * 256 + 128; py = 4 * 256 + 128; ang = 192;
                idle_frames = 0;
                visited[0] = 1;
                show_banner();
                set_msg("FIND THE WAY OUT.");
                state = ST_PLAY;
                sq_stop(); seq_len = 0; seq_idx = 0;
                present();
            }
            frame++;
        } else if (state == ST_WIN) {
            if (pressed & KEY_START) {
                state = ST_TITLE;
                sq_stop();
                draw_title();
            }
            frame++;
        } else if (state == ST_NOTE) {
            if (pressed & (KEY_A | KEY_B | KEY_START)) {
                state = ST_PLAY;
                present();
            }
            frame++;
        } else if (state == ST_INV) {
            if (pressed & (KEY_START | KEY_B | KEY_A)) {
                state = ST_PLAY;
                present();
            }
            frame++;
        } else if (state == ST_MAP) {
            if (pressed & (KEY_SELECT | KEY_B | KEY_A)) {
                state = ST_PLAY;
                present();
            }
            frame++;
        } else {
            /* INSTANT tile-step: one press = one full tile / one 90-degree
             * turn, rendered once. No interpolation, no bob, and no
             * per-frame re-render while idle, so the framebuffer (and the
             * status bar) stays rock-steady between actions. */
            int dir = 0; /* 1 fwd, 2 back, 3 left, 4 right */
            if (pressed & KEY_UP) dir = 1;
            else if (pressed & KEY_DOWN) dir = 2;
            else if (pressed & KEY_LEFT) dir = 3;
            else if (pressed & KEY_RIGHT) dir = 4;
            else if (pressed & KEY_A) { interact(); if (state == ST_PLAY) present(); frame++; sram_dump(); continue; }
            else if (pressed & KEY_START) { state = ST_INV; draw_inventory(); frame++; sram_dump(); continue; }
            else if (pressed & KEY_SELECT) { state = ST_MAP; draw_minimap(); frame++; sram_dump(); continue; }
            else if (idle_frames >= 18) {
                if (held & KEY_UP) dir = 1;
                else if (held & KEY_DOWN) dir = 2;
                else if (held & KEY_LEFT) dir = 3;
                else if (held & KEY_RIGHT) dir = 4;
            }
            if (dir == 1 || dir == 2) {
                int sv = (dir == 1) ? 1 : -1;
                int tx = ptx + fdx[facing] * sv, ty = pty + fdy[facing] * sv;
                int code = cell_at(area, tx, ty);
                int ok = 0;
                if (code == C_FLOOR || code == C_ITEM) ok = 1;
                else if (code == C_GATE) {
                    if (gate_open[area]) ok = 1;
                    else if (bump_timer == 0) { set_msg(gate_hint(area)); sfx_bump(); bump_timer = 20; idle_frames = 0; present(); }
                } else if (code == C_BACK) {
                    if (area > 0) ok = 1;
                }
                if (ok) {
                    ptx = tx; pty = ty;
                    px = tx * 256 + 128; py = ty * 256 + 128;
                    sfx_step();
                    if (code == C_ITEM) {
                        int it = item_type_at(area, ptx, pty);
                        MAPS[area][pty][ptx] = '0';
                        collect_item(it);
                    } else if (code == C_GATE) {
                        transition(1);
                    } else if (code == C_BACK) {
                        transition(-1);
                    }
                    present();
                } else if (code != C_GATE && bump_timer == 0) {
                    /* v9: the bump sound is ONLY for a real wall (wood /
                     * brass / dark) or the back gate at area 0, which is
                     * a solid gate structure. Walking into a gear dial,
                     * music stand, piano plate, bell, note, lever, chest
                     * or core must stay silent — the old code bumped on
                     * every solid cell, and the high-pitched noise
                     * stepped on the tone of the musical item in front
                     * of the player. (Locked forward gates bump in
                     * their own branch above, unchanged.) */
                    if (code == C_WOOD || code == C_BRASS || code == C_DARK || code == C_BACK) {
                        sfx_bump(); bump_timer = 20; idle_frames = 0;
                    }
                }
            } else if (dir == 3 || dir == 4) {
                facing = (dir == 4) ? (facing + 1) & 3 : (facing + 3) & 3;
                ang = facing * 64;
                sfx_step();
                present();
            }
            if (bump_timer) bump_timer--;
            if (dir) idle_frames = 0; else idle_frames++;
            /* banner/message just expired: re-present once so the overlay
             * clears. Otherwise the frame is left alone — no flicker. */
            if ((prev_banner > 0 && banner_timer == 0) || (prev_msg > 0 && msg_timer == 0)) present();
            prev_banner = banner_timer; prev_msg = msg_timer;
            frame++;
        }
        sram_dump();
    }
    return 0;
}
