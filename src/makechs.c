#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef int bool;
typedef unsigned char uchar;

#ifndef PLATFORM_LINUX
typedef unsigned short ushort;
typedef unsigned long ulong;
#endif

#define TRUE 1
#define FALSE 0

#ifdef PLATFORM_LINUX
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

struct fallback
{
   ushort u;
   uchar *str;
};

/* The table below is Markus Kuhn's transliteration table transtab with some minor changes (see comments below) */
/* The original table can be found at http://www.cl.cam.ac.uk/~mgk25/download/transtab.tar.gz */

struct fallback transtab[] = {
/* { 0x0027, "’" },        */ /* removed, non-ascii */
/* { 0x0060, "‛', '‘" }, */ /* removed, non-ascii */
   { 0x00A0, " " },
   { 0x00A1, "!" },
   { 0x00A2, "c" },
   { 0x00A3, "GBP" },
   { 0x00A5, "Y" },
   { 0x00A6, "|" },
   { 0x00A7, "S" },
   { 0x00A8, "\"" },
   { 0x00A9, "(c)" },
   { 0x00AA, "a" },
   { 0x00AB, "<<" },
   { 0x00AC, "-" },
   { 0x00AD, "-" },
   { 0x00AE, "(R)" },
   { 0x00AF, "-" },
   { 0x00B0, " " },
   { 0x00B1, "+/-" },
   { 0x00B2, "^2" },
   { 0x00B3, "^3" },
   { 0x00B4, "'" },
   { 0x00B5, "u" },
   { 0x00B6, "P" },
   { 0x00B7, "." },
   { 0x00B8, "," },
   { 0x00B9, "1" },
   { 0x00BA, "o" },
   { 0x00BB, ">>" },
   { 0x00BC, " 1/4" },
   { 0x00BD, " 1/2" },
   { 0x00BE, " 3/4" },
   { 0x00BF, "?" },
   { 0x00C0, "A" },
   { 0x00C1, "A" },
   { 0x00C2, "A" },
   { 0x00C3, "A" },
   { 0x00C4, "Ae" },
   { 0x00C5, "Aa" },
   { 0x00C6, "AE" },
   { 0x00C7, "C" },
   { 0x00C8, "E" },
   { 0x00C9, "E" },
   { 0x00CA, "E" },
   { 0x00CB, "E" },
   { 0x00CC, "I" },
   { 0x00CD, "I" },
   { 0x00CE, "I" },
   { 0x00CF, "I" },
   { 0x00D0, "D" },
   { 0x00D1, "N" },
   { 0x00D2, "O" },
   { 0x00D3, "O" },
   { 0x00D4, "O" },
   { 0x00D5, "O" },
   { 0x00D6, "Oe" },
   { 0x00D7, "x" },
   { 0x00D8, "O" },
   { 0x00D9, "U" },
   { 0x00DA, "U" },
   { 0x00DB, "U" },
   { 0x00DC, "Ue" },
   { 0x00DD, "Y" },
   { 0x00DE, "Th" },
   { 0x00DF, "ss" },
   { 0x00E0, "a" },
   { 0x00E1, "a" },
   { 0x00E2, "a" },
   { 0x00E3, "a" },
   { 0x00E4, "ae" },
   { 0x00E5, "aa" },
   { 0x00E6, "ae" },
   { 0x00E7, "c" },
   { 0x00E8, "e" },
   { 0x00E9, "e" },
   { 0x00EA, "e" },
   { 0x00EB, "e" },
   { 0x00EC, "i" },
   { 0x00ED, "i" },
   { 0x00EE, "i" },
   { 0x00EF, "i" },
   { 0x00F0, "d" },
   { 0x00F1, "n" },
   { 0x00F2, "o" },
   { 0x00F3, "o" },
   { 0x00F4, "o" },
   { 0x00F5, "o" },
   { 0x00F6, "oe" },
   { 0x00F7, ":" },
   { 0x00F8, "o" },
   { 0x00F9, "u" },
   { 0x00FA, "u" },
   { 0x00FB, "u" },
   { 0x00FC, "ue" },
   { 0x00FD, "y" },
   { 0x00FE, "th" },
   { 0x00FF, "y" },
   { 0x0100, "A" },
   { 0x0101, "a" },
   { 0x0102, "A" },
   { 0x0103, "a" },
   { 0x0104, "A" },
   { 0x0105, "a" },
   { 0x0106, "C" },
   { 0x0107, "c" },
   { 0x0108, "Ch" },
   { 0x0109, "ch" },
   { 0x010A, "C" },
   { 0x010B, "c" },
   { 0x010C, "C" },
   { 0x010D, "c" },
   { 0x010E, "D" },
   { 0x010F, "d" },
   { 0x0110, "D" },
   { 0x0111, "d" },
   { 0x0112, "E" },
   { 0x0113, "e" },
   { 0x0114, "E" },
   { 0x0115, "e" },
   { 0x0116, "E" },
   { 0x0117, "e" },
   { 0x0118, "E" },
   { 0x0119, "e" },
   { 0x011A, "E" },
   { 0x011B, "e" },
   { 0x011C, "Gh" },
   { 0x011D, "gh" },
   { 0x011E, "G" },
   { 0x011F, "g" },
   { 0x0120, "G" },
   { 0x0121, "g" },
   { 0x0122, "G" },
   { 0x0123, "g" },
   { 0x0124, "Hh" },
   { 0x0125, "hh" },
   { 0x0126, "H" },
   { 0x0127, "h" },
   { 0x0128, "I" },
   { 0x0129, "i" },
   { 0x012A, "I" },
   { 0x012B, "i" },
   { 0x012C, "I" },
   { 0x012D, "i" },
   { 0x012E, "I" },
   { 0x012F, "i" },
   { 0x0130, "I" },
   { 0x0131, "i" },
   { 0x0132, "IJ" },
   { 0x0133, "ij" },
   { 0x0134, "Jh" },
   { 0x0135, "jh" },
   { 0x0136, "K" },
   { 0x0137, "k" },
   { 0x0138, "k" },
   { 0x0139, "L" },
   { 0x013A, "l" },
   { 0x013B, "L" },
   { 0x013C, "l" },
   { 0x013D, "L" },
   { 0x013E, "l" },
   { 0x013F, "L" },
   { 0x0140, "l" },
   { 0x0141, "L" },
   { 0x0142, "l" },
   { 0x0143, "N" },
   { 0x0144, "n" },
   { 0x0145, "N" },
   { 0x0146, "n" },
   { 0x0147, "N" },
   { 0x0148, "n" },
   { 0x0149, "'n" },
   { 0x014A, "NG" },
   { 0x014B, "ng" },
   { 0x014C, "O" },
   { 0x014D, "o" },
   { 0x014E, "O" },
   { 0x014F, "o" },
   { 0x0150, "O" },
   { 0x0151, "o" },
   { 0x0152, "OE" },
   { 0x0153, "oe" },
   { 0x0154, "R" },
   { 0x0155, "r" },
   { 0x0156, "R" },
   { 0x0157, "r" },
   { 0x0158, "R" },
   { 0x0159, "r" },
   { 0x015A, "S" },
   { 0x015B, "s" },
   { 0x015C, "Sh" },
   { 0x015D, "sh" },
   { 0x015E, "S" },
   { 0x015F, "s" },
   { 0x0160, "S" },
   { 0x0161, "s" },
   { 0x0162, "T" },
   { 0x0163, "t" },
   { 0x0164, "T" },
   { 0x0165, "t" },
   { 0x0166, "T" },
   { 0x0167, "t" },
   { 0x0168, "U" },
   { 0x0169, "u" },
   { 0x016A, "U" },
   { 0x016B, "u" },
   { 0x016C, "U" },
   { 0x016D, "u" },
   { 0x016E, "U" },
   { 0x016F, "u" },
   { 0x0170, "U" },
   { 0x0171, "u" },
   { 0x0172, "U" },
   { 0x0173, "u" },
   { 0x0174, "W" },
   { 0x0175, "w" },
   { 0x0176, "Y" },
   { 0x0177, "y" },
   { 0x0178, "Y" },
   { 0x0179, "Z" },
   { 0x017A, "z" },
   { 0x017B, "Z" },
   { 0x017C, "z" },
   { 0x017D, "Z" },
   { 0x017E, "z" },
   { 0x017F, "s" },
   { 0x0192, "f" },
   { 0x0218, "S" },
   { 0x0219, "s" },
   { 0x021A, "T" },
   { 0x021B, "t" },
   { 0x02B9, "'" },
/* { 0x02BB, "‘" }, */ /* removed, non-ascii */
   { 0x02BC, "'" },
/* { 0x02BD, "‛" }, */ /* removed, non-ascii */
   { 0x02C6, "^" },
   { 0x02C8, "'" },
/* { 0x02C9, "¯" }, */ /* removed, non-ascii */
   { 0x02CC, "," },
   { 0x02D0, ":" },
/* { 0x02DA, "°" }, */ /* removed, non-ascii */
   { 0x02DC, "~" },
   { 0x02DD, "\"" },
   { 0x0374, "'" },
   { 0x0375, "," },
   { 0x037E, ";" },
   { 0x1E02, "B" },
   { 0x1E03, "b" },
   { 0x1E0A, "D" },
   { 0x1E0B, "d" },
   { 0x1E1E, "F" },
   { 0x1E1F, "f" },
   { 0x1E40, "M" },
   { 0x1E41, "m" },
   { 0x1E56, "P" },
   { 0x1E57, "p" },
   { 0x1E60, "S" },
   { 0x1E61, "s" },
   { 0x1E6A, "T" },
   { 0x1E6B, "t" },
   { 0x1E80, "W" },
   { 0x1E81, "w" },
   { 0x1E82, "W" },
   { 0x1E83, "w" },
   { 0x1E84, "W" },
   { 0x1E85, "w" },
   { 0x1EF2, "Y" },
   { 0x1EF3, "y" },
   { 0x2000, " " },
   { 0x2001, "  " },
   { 0x2002, " " },
   { 0x2003, "  " },
   { 0x2004, " " },
   { 0x2005, " " },
   { 0x2006, " " },
   { 0x2007, " " },
   { 0x2008, " " },
   { 0x2009, " " },
   { 0x200A, "" },
   { 0x200B, "" },
   { 0x200C, "" },
   { 0x200D, "" },
   { 0x200E, "" },
   { 0x200F, "" },
   { 0x2010, "-" },
   { 0x2011, "-" },
   { 0x2012, "-" },
   { 0x2013, "-" },
   { 0x2014, "--" },
   { 0x2015, "--" },
   { 0x2016, "||" },
   { 0x2017, "_" },
   { 0x2018, "'" },
   { 0x2019, "'" },
   { 0x201A, "'" },
   { 0x201B, "'" },
   { 0x201C, "\"" },
   { 0x201D, "\"" },
   { 0x201E, "\"" },
   { 0x201F, "\"" },
   { 0x2020, "+" },
   { 0x2021, "++" },
   { 0x2022, "o" },
   { 0x2023, ">" },
   { 0x2024, "." },
   { 0x2025, ".." },
   { 0x2026, "..." },
   { 0x2027, "-" },
   { 0x202A, "" },
   { 0x202B, "" },
   { 0x202C, "" },
   { 0x202D, "" },
   { 0x202E, "" },
   { 0x202F, " " },
   { 0x2030, " 0/00" },
   { 0x2032, "'" },
   { 0x2033, "\"" },
   { 0x2034, "'''" },
   { 0x2035, "`" },
   { 0x2036, "``" },
   { 0x2037, "```" },
   { 0x2039, "<" },
   { 0x203A, ">" },
   { 0x203C, "!!" },
   { 0x203E, "-" },
   { 0x2043, "-" },
   { 0x2044, "/" },
   { 0x2048, "?!" },
   { 0x2049, "!?" },
   { 0x204A, "7" },
   { 0x2070, "^0" },
   { 0x2074, "^4" },
   { 0x2075, "^5" },
   { 0x2076, "^6" },
   { 0x2077, "^7" },
   { 0x2078, "^8" },
   { 0x2079, "^9" },
   { 0x207A, "^+" },
   { 0x207C, "^=" },
   { 0x207D, "^(" },
   { 0x207E, "^)" },
   { 0x207F, "^n" },
   { 0x2080, "_0" },
   { 0x2081, "_1" },
   { 0x2082, "_2" },
   { 0x2083, "_3" },
   { 0x2084, "_4" },
   { 0x2085, "_5" },
   { 0x2086, "_6" },
   { 0x2087, "_7" },
   { 0x2088, "_8" },
   { 0x2089, "_9" },
   { 0x208A, "_+" },
   { 0x208B, "_-" },
   { 0x208C, "_=" },
   { 0x208D, "_(" },
   { 0x208E, "_)" },
   { 0x20AC, "EUR" },
   { 0x2100, "a/c" },
   { 0x2101, "a/s" },
   { 0x2103, "oC"  }, /* replaced, non-ascii */
   { 0x2105, "c/o" },
   { 0x2106, "c/u" },
   { 0x2109, "oF"  }, /* replaced, non-ascii */
   { 0x2113, "l" },
   { 0x2116, "No" },
   { 0x2117, "(P)" },
   { 0x2120, "[SM]" },
   { 0x2121, "TEL" },
   { 0x2122, "[TM]" },
   { 0x2126, "ohm" },
   { 0x212A, "K" },
   { 0x212B, "A" }, /* replaced, non-ascii */
   { 0x212E, "e" },
   { 0x2153, " 1/3" },
   { 0x2154, " 2/3" },
   { 0x2155, " 1/5" },
   { 0x2156, " 2/5" },
   { 0x2157, " 3/5" },
   { 0x2158, " 4/5" },
   { 0x2159, " 1/6" },
   { 0x215A, " 5/6" },
   { 0x215B, " 1/8" },
   { 0x215C, " 3/8" },
   { 0x215D, " 5/8" },
   { 0x215E, " 7/8" },
   { 0x215F, " 1/" },
   { 0x2160, "I" },
   { 0x2161, "II" },
   { 0x2162, "III" },
   { 0x2163, "IV" },
   { 0x2164, "V" },
   { 0x2165, "VI" },
   { 0x2166, "VII" },
   { 0x2167, "VIII" },
   { 0x2168, "IX" },
   { 0x2169, "X" },
   { 0x216A, "XI" },
   { 0x216B, "XII" },
   { 0x216C, "L" },
   { 0x216D, "C" },
   { 0x216E, "D" },
   { 0x216F, "M" },
   { 0x2170, "i" },
   { 0x2171, "ii" },
   { 0x2172, "iii" },
   { 0x2173, "iv" },
   { 0x2174, "v" },
   { 0x2175, "vi" },
   { 0x2176, "vii" },
   { 0x2177, "viii" },
   { 0x2178, "ix" },
   { 0x2179, "x" },
   { 0x217A, "xi" },
   { 0x217B, "xii" },
   { 0x217C, "l" },
   { 0x217D, "c" },
   { 0x217E, "d" },
   { 0x217F, "m" },
   { 0x2190, "<-" },
   { 0x2191, "^" },
   { 0x2192, "->" },
   { 0x2193, "v" },
   { 0x2194, "<->" },
   { 0x21D0, "<=" },
   { 0x21D2, "=>" },
   { 0x21D4, "<=>" },
   { 0x2212, "-" },
   { 0x2215, "/" },
   { 0x2216, "\\" },
   { 0x2217, "*" },
   { 0x2218, "o" },
/* { 0x2219, "·" }, */ /* removed, non-ascii */
   { 0x221E, "inf" },
   { 0x2223, "|" },
   { 0x2225, "||" },
   { 0x2236, ":" },
   { 0x223C, "~" },
   { 0x2260, "/=" },
   { 0x2261, "=" },
   { 0x2264, "<=" },
   { 0x2265, ">=" },
   { 0x226A, "<<" },
   { 0x226B, ">>" },
   { 0x2295, "(+)" },
   { 0x2296, "(-)" },
   { 0x2297, "(x)" },
   { 0x2298, "(/)" },
   { 0x22A2, "|-" },
   { 0x22A3, "-|" },
   { 0x22A6, "|-" },
   { 0x22A7, "|=" },
   { 0x22A8, "|=" },
   { 0x22A9, "||-" },
/* { 0x22C5, "·" }, */ /* removed, non-ascii */
   { 0x22C6, "*" },
   { 0x22D5, "#" },
   { 0x22D8, "<<<" },
   { 0x22D9, ">>>" },
   { 0x22EF, "..." },
   { 0x2329, "<" },
   { 0x232A, ">" },
   { 0x2400, "NUL" },
   { 0x2401, "SOH" },
   { 0x2402, "STX" },
   { 0x2403, "ETX" },
   { 0x2404, "EOT" },
   { 0x2405, "ENQ" },
   { 0x2406, "ACK" },
   { 0x2407, "BEL" },
   { 0x2408, "BS" },
   { 0x2409, "HT" },
   { 0x240A, "LF" },
   { 0x240B, "VT" },
   { 0x240C, "FF" },
   { 0x240D, "CR" },
   { 0x240E, "SO" },
   { 0x240F, "SI" },
   { 0x2410, "DLE" },
   { 0x2411, "DC1" },
   { 0x2412, "DC2" },
   { 0x2413, "DC3" },
   { 0x2414, "DC4" },
   { 0x2415, "NAK" },
   { 0x2416, "SYN" },
   { 0x2417, "ETB" },
   { 0x2418, "CAN" },
   { 0x2419, "EM" },
   { 0x241A, "SUB" },
   { 0x241B, "ESC" },
   { 0x241C, "FS" },
   { 0x241D, "GS" },
   { 0x241E, "RS" },
   { 0x241F, "US" },
   { 0x2420, "SP" },
   { 0x2421, "DEL" },
   { 0x2423, "_" },
   { 0x2424, "NL" },
   { 0x2425, "///" },
   { 0x2426, "?" },
   { 0x2460, "(1)" },
   { 0x2461, "(2)" },
   { 0x2462, "(3)" },
   { 0x2463, "(4)" },
   { 0x2464, "(5)" },
   { 0x2465, "(6)" },
   { 0x2466, "(7)" },
   { 0x2467, "(8)" },
   { 0x2468, "(9)" },
   { 0x2469, "(10)" },
   { 0x246A, "(11)" },
   { 0x246B, "(12)" },
   { 0x246C, "(13)" },
   { 0x246D, "(14)" },
   { 0x246E, "(15)" },
   { 0x246F, "(16)" },
   { 0x2470, "(17)" },
   { 0x2471, "(18)" },
   { 0x2472, "(19)" },
   { 0x2473, "(20)" },
   { 0x2474, "(1)" },
   { 0x2475, "(2)" },
   { 0x2476, "(3)" },
   { 0x2477, "(4)" },
   { 0x2478, "(5)" },
   { 0x2479, "(6)" },
   { 0x247A, "(7)" },
   { 0x247B, "(8)" },
   { 0x247C, "(9)" },
   { 0x247D, "(10)" },
   { 0x247E, "(11)" },
   { 0x247F, "(12)" },
   { 0x2480, "(13)" },
   { 0x2481, "(14)" },
   { 0x2482, "(15)" },
   { 0x2483, "(16)" },
   { 0x2484, "(17)" },
   { 0x2485, "(18)" },
   { 0x2486, "(19)" },
   { 0x2487, "(20)" },
   { 0x2488, "1." },
   { 0x2489, "2." },
   { 0x248A, "3." },
   { 0x248B, "4." },
   { 0x248C, "5." },
   { 0x248D, "6." },
   { 0x248E, "7." },
   { 0x248F, "8." },
   { 0x2490, "9." },
   { 0x2491, "10." },
   { 0x2492, "11." },
   { 0x2493, "12." },
   { 0x2494, "13." },
   { 0x2495, "14." },
   { 0x2496, "15." },
   { 0x2497, "16." },
   { 0x2498, "17." },
   { 0x2499, "18." },
   { 0x249A, "19." },
   { 0x249B, "20." },
   { 0x249C, "(a)" },
   { 0x249D, "(b)" },
   { 0x249E, "(c)" },
   { 0x249F, "(d)" },
   { 0x24A0, "(e)" },
   { 0x24A1, "(f)" },
   { 0x24A2, "(g)" },
   { 0x24A3, "(h)" },
   { 0x24A4, "(i)" },
   { 0x24A5, "(j)" },
   { 0x24A6, "(k)" },
   { 0x24A7, "(l)" },
   { 0x24A8, "(m)" },
   { 0x24A9, "(n)" },
   { 0x24AA, "(o)" },
   { 0x24AB, "(p)" },
   { 0x24AC, "(q)" },
   { 0x24AD, "(r)" },
   { 0x24AE, "(s)" },
   { 0x24AF, "(t)" },
   { 0x24B0, "(u)" },
   { 0x24B1, "(v)" },
   { 0x24B2, "(w)" },
   { 0x24B3, "(x)" },
   { 0x24B4, "(y)" },
   { 0x24B5, "(z)" },
   { 0x24B6, "(A)" },
   { 0x24B7, "(B)" },
   { 0x24B8, "(C)" },
   { 0x24B9, "(D)" },
   { 0x24BA, "(E)" },
   { 0x24BB, "(F)" },
   { 0x24BC, "(G)" },
   { 0x24BD, "(H)" },
   { 0x24BE, "(I)" },
   { 0x24BF, "(J)" },
   { 0x24C0, "(K)" },
   { 0x24C1, "(L)" },
   { 0x24C2, "(M)" },
   { 0x24C3, "(N)" },
   { 0x24C4, "(O)" },
   { 0x24C5, "(P)" },
   { 0x24C6, "(Q)" },
   { 0x24C7, "(R)" },
   { 0x24C8, "(S)" },
   { 0x24C9, "(T)" },
   { 0x24CA, "(U)" },
   { 0x24CB, "(V)" },
   { 0x24CC, "(W)" },
   { 0x24CD, "(X)" },
   { 0x24CE, "(Y)" },
   { 0x24CF, "(Z)" },
   { 0x24D0, "(a)" },
   { 0x24D1, "(b)" },
   { 0x24D2, "(c)" },
   { 0x24D3, "(d)" },
   { 0x24D4, "(e)" },
   { 0x24D5, "(f)" },
   { 0x24D6, "(g)" },
   { 0x24D7, "(h)" },
   { 0x24D8, "(i)" },
   { 0x24D9, "(j)" },
   { 0x24DA, "(k)" },
   { 0x24DB, "(l)" },
   { 0x24DC, "(m)" },
   { 0x24DD, "(n)" },
   { 0x24DE, "(o)" },
   { 0x24DF, "(p)" },
   { 0x24E0, "(q)" },
   { 0x24E1, "(r)" },
   { 0x24E2, "(s)" },
   { 0x24E3, "(t)" },
   { 0x24E4, "(u)" },
   { 0x24E5, "(v)" },
   { 0x24E6, "(w)" },
   { 0x24E7, "(x)" },
   { 0x24E8, "(y)" },
   { 0x24E9, "(z)" },
   { 0x24EA, "(0)" },
   { 0x2500, "-" },
   { 0x2501, "=" },
   { 0x2502, "|" },
   { 0x2503, "|" },
   { 0x2504, "-" },
   { 0x2505, "=" },
   { 0x2506, "|" },
   { 0x2507, "|" },
   { 0x2508, "-" },
   { 0x2509, "=" },
   { 0x250A, "|" },
   { 0x250B, "|" },
   { 0x250C, "+" },
   { 0x250D, "+" },
   { 0x250E, "+" },
   { 0x250F, "+" },
   { 0x2510, "+" },
   { 0x2511, "+" },
   { 0x2512, "+" },
   { 0x2513, "+" },
   { 0x2514, "+" },
   { 0x2515, "+" },
   { 0x2516, "+" },
   { 0x2517, "+" },
   { 0x2518, "+" },
   { 0x2519, "+" },
   { 0x251A, "+" },
   { 0x251B, "+" },
   { 0x251C, "+" },
   { 0x251D, "+" },
   { 0x251E, "+" },
   { 0x251F, "+" },
   { 0x2520, "+" },
   { 0x2521, "+" },
   { 0x2522, "+" },
   { 0x2523, "+" },
   { 0x2524, "+" },
   { 0x2525, "+" },
   { 0x2526, "+" },
   { 0x2527, "+" },
   { 0x2528, "+" },
   { 0x2529, "+" },
   { 0x252A, "+" },
   { 0x252B, "+" },
   { 0x252C, "+" },
   { 0x252D, "+" },
   { 0x252E, "+" },
   { 0x252F, "+" },
   { 0x2530, "+" },
   { 0x2531, "+" },
   { 0x2532, "+" },
   { 0x2533, "+" },
   { 0x2534, "+" },
   { 0x2535, "+" },
   { 0x2536, "+" },
   { 0x2537, "+" },
   { 0x2538, "+" },
   { 0x2539, "+" },
   { 0x253A, "+" },
   { 0x253B, "+" },
   { 0x253C, "+" },
   { 0x253D, "+" },
   { 0x253E, "+" },
   { 0x253F, "+" },
   { 0x2540, "+" },
   { 0x2541, "+" },
   { 0x2542, "+" },
   { 0x2543, "+" },
   { 0x2544, "+" },
   { 0x2545, "+" },
   { 0x2546, "+" },
   { 0x2547, "+" },
   { 0x2548, "+" },
   { 0x2549, "+" },
   { 0x254A, "+" },
   { 0x254B, "+" },
   { 0x254C, "-" },
   { 0x254D, "=" },
   { 0x254E, "|" },
   { 0x254F, "|" },
   { 0x2550, "=" },
   { 0x2551, "|" },
   { 0x2552, "+" },
   { 0x2553, "+" },
   { 0x2554, "+" },
   { 0x2555, "+" },
   { 0x2556, "+" },
   { 0x2557, "+" },
   { 0x2558, "+" },
   { 0x2559, "+" },
   { 0x255A, "+" },
   { 0x255B, "+" },
   { 0x255C, "+" },
   { 0x255D, "+" },
   { 0x255E, "+" },
   { 0x255F, "+" },
   { 0x2560, "+" },
   { 0x2561, "+" },
   { 0x2562, "+" },
   { 0x2563, "+" },
   { 0x2564, "+" },
   { 0x2565, "+" },
   { 0x2566, "+" },
   { 0x2567, "+" },
   { 0x2568, "+" },
   { 0x2569, "+" },
   { 0x256A, "+" },
   { 0x256B, "+" },
   { 0x256C, "+" },
   { 0x256D, "+" },
   { 0x256E, "+" },
   { 0x256F, "+" },
   { 0x2570, "+" },
   { 0x2571, "/" },
   { 0x2572, "\\" },
   { 0x2573, "X" },
   { 0x257C, "-" },
   { 0x257D, "|" },
   { 0x257E, "-" },
   { 0x257F, "|" },
   { 0x2580, "#" }, /* added for IBM block graphics */
   { 0x2584, "#" }, /* added for IBM block graphics */
   { 0x2588, "#" }, /* added for IBM block graphics */
   { 0x258c, "#" }, /* added for IBM block graphics */
   { 0x2590, "#" }, /* added for IBM block graphics */
   { 0x2591, "#" }, /* added for IBM block graphics */
   { 0x2592, "#" }, /* added for IBM block graphics */
   { 0x2593, "#" }, /* added for IBM block graphics */
   { 0x25A0, "#" }, /* added for IBM block graphics */
   { 0x25CB, "o" },
   { 0x25E6, "o" },
   { 0x2605, "*" },
   { 0x2606, "*" },
   { 0x2612, "X" },
   { 0x2613, "X" },
   { 0x2639, ":-(" },
   { 0x263A, ":-)" },
   { 0x263B, "(-:" },
   { 0x266D, "b" },
   { 0x266F, "#" },
   { 0x2701, "%<" },
   { 0x2702, "%<" },
   { 0x2703, "%<" },
   { 0x2704, "%<" },
   { 0x270C, "V" },
/* { 0x2713, "√" }, */ /* removed, non-ascii */
/* { 0x2714, "√" }, */ /* removed, non-ascii */
   { 0x2715, "x" },
   { 0x2716, "x" },
   { 0x2717, "X" },
   { 0x2718, "X" },
   { 0x2719, "+" },
   { 0x271A, "+" },
   { 0x271B, "+" },
   { 0x271C, "+" },
   { 0x271D, "+" },
   { 0x271E, "+" },
   { 0x271F, "+" },
   { 0x2720, "+" },
   { 0x2721, "*" },
   { 0x2722, "+" },
   { 0x2723, "+" },
   { 0x2724, "+" },
   { 0x2725, "+" },
   { 0x2726, "+" },
   { 0x2727, "+" },
   { 0x2729, "*" },
   { 0x272A, "*" },
   { 0x272B, "*" },
   { 0x272C, "*" },
   { 0x272D, "*" },
   { 0x272E, "*" },
   { 0x272F, "*" },
   { 0x2730, "*" },
   { 0x2731, "*" },
   { 0x2732, "*" },
   { 0x2733, "*" },
   { 0x2734, "*" },
   { 0x2735, "*" },
   { 0x2736, "*" },
   { 0x2737, "*" },
   { 0x2738, "*" },
   { 0x2739, "*" },
   { 0x273A, "*" },
   { 0x273B, "*" },
   { 0x273C, "*" },
   { 0x273D, "*" },
   { 0x273E, "*" },
   { 0x273F, "*" },
   { 0x2740, "*" },
   { 0x2741, "*" },
   { 0x2742, "*" },
   { 0x2743, "*" },
   { 0x2744, "*" },
   { 0x2745, "*" },
   { 0x2746, "*" },
   { 0x2747, "*" },
   { 0x2748, "*" },
   { 0x2749, "*" },
   { 0x274A, "*" },
   { 0x274B, "*" },
   { 0xFB00, "ff"  },
   { 0xFB01, "fi"  },
   { 0xFB02, "fl"  },
   { 0xFB03, "ffi" },
   { 0xFB04, "ffl" },
   { 0xFB05, "st"  },
   { 0xFB06, "st"  },
   { 0xFEFF, ""  },
   { 0xFFFD, "?" },
   { 0, "" } /* end of table */
};

void mystrncpy(uchar *dest,uchar *src,long len)
{
   if(len == 0)
      return;
      
   strncpy(dest,src,(size_t)len-1);
   dest[len-1]=0;
}

bool mapgetword(uchar *line, ulong *pos, uchar *dest, ulong destlen)
{
   ulong begin;

   while(isspace(line[*pos]) && line[*pos]!=0)
      (*pos)++;

   if(line[*pos] == 0)
      return(FALSE);

   begin=*pos;

   while(line[*pos]!=0 && !isspace(line[*pos]))
      (*pos)++;

   if(line[*pos] != 0)
   {
      line[*pos]=0;
      (*pos)++;
   }

   mystrncpy(dest,&line[begin],destlen);

   return(TRUE);
}

bool readmap(ushort *table,uchar *filename)
{
   uchar line[200],buf1[20],buf2[20];
   ulong pos,c,d,linenum;
   bool res1,res2;
   FILE *fp;

   if(!(fp=fopen(filename,"r")))
   {
      fprintf(stderr,"Could not open %s\n",filename);
      return(FALSE);
   }

   /* Make default table */

   for(c=0;c<256;c++)
      table[c]=c;

   /* Read table */

   linenum=0;

   while(fgets(line,200,fp))
   {
      linenum++;

      if(strchr(line,'#'))
         *strchr(line,'#')=0; /* Remove comments */

      if(line[0] == '\x1a')
         line[0]=0; /* DOS EOF */

      pos=0;

      res1=mapgetword(line,&pos,buf1,20);
      res2=mapgetword(line,&pos,buf2,20);

      if(res1)
      {
         if(strnicmp(buf1,"0x",2)!=0)
         {
            fprintf(stderr,"Syntax error on line %ld in %s",linenum,filename);
            return(FALSE);
         }

         sscanf(&buf1[2],"%lx",&c);

         if(res2)
         {
            if(strnicmp(buf2,"0x",2)!=0)
            {
               fprintf(stderr,"Syntax error on line %ld in %s",linenum,filename);
               return(FALSE);
            }

            sscanf(&buf2[2],"%lx",&d);
         }
         else
         {
            d=0;
         }

         if(c > 0xff || d > 0xffff)
         {
            fprintf(stderr,"Character out of range on line %ld in %s",linenum,filename);
            return(FALSE);
         }

         if(d == 0 && c != 0) table[c]='?';
         else                 table[c]=d;
      }
   }

   fclose(fp);

   return(TRUE);
}

void printbyte(unsigned char b)
{
   if(b == 0)
   {
      printf("\\0");
   }
   else if(b > 127 || !isgraph(b) || b == '\\' || b ==';')
   {
      printf("\\x%02X",b);
   }
   else
   {
      printf("%c",b);
   }
}
     
int main(int argc, char **argv)
{
   ushort fromtable[256],desttable[256];
   int c,d,firstdiff;
   bool createutf;
   ushort uc;
       
   if(argc != 4 && argc!=5)
   {
      fprintf(stderr,"Usage: makechs <fromchrs> <destchrs> <frommap> [<destmap>]\n");
      exit(0);
   }
   
   if(argc == 4) createutf=TRUE;
   else          createutf=FALSE;
   
   if(!(readmap(fromtable,argv[3])))
      exit(0);

   if(!createutf)
   {      
      if(!(readmap(desttable,argv[4])))
         exit(0);
   }

   if(createutf)
   {
      firstdiff=0;
   }
   else
   {
      for(c=0;c<256;c++)
      {
         if(fromtable[c] != desttable[c])
         {
            firstdiff=c;
            break;
         }
       }
   }   
      
   printf(";\n");
   printf("; Generated by makechs 1.0 by Johan Billing. Some tweaking may be required.\n");
   printf(";\n");
      
   if(createutf || firstdiff < 128)
      printf("100000          ; ID number (when >65535, all 255 chars will be translated)\n");
      
   else
      printf("0               ; ID number (when >65535, all 255 chars will be translated)\n");
      
   printf("0               ; version number\n");
   printf(";\n");
   printf("2               ; level number\n");
   printf(";\n");
   printf("%s\n",argv[1]);
   printf("%s\n",argv[2]);
   printf(";\n");
   
   for(c=0;c<256;c++)
   {
      if(createutf)
      {
         uc=fromtable[c];
         
         if(uc < 0x80)
         {
            printbyte(0);
            printf("\t");
            printbyte(uc);
            printf("\n");
         }
         else if(uc < 0x800)
         {
            printbyte(0xC0 | (uc >> 6));
            printf("\t");
            printbyte(0x80 | (uc & 0x3F));
            printf("\n");
         }
         else
         {
            printbyte(0xE0 | (uc >> 12));
            printf("\t");
            printbyte(0x80 | ((uc >> 6) & 0x3F));
            printf("\t");
            printbyte(0x80 | (uc        & 0x3F));
            printf("\n");
         }     
      }
      else if(c>127 || firstdiff<128)
      {
         uc=fromtable[c];
            
         if(uc < 128)
         {
            printbyte(0);
            printf("\t");
            printbyte(uc);
            printf("\n");
         }
         else
         {
            for(d=0;d<256;d++)
               if(desttable[d] == uc) break;

            if(d != 256)
            {
               /* found */

               printbyte(0);
               printf("\t");
               printbyte(d);
               printf("\n");
            }
            else
            {
               /* use fallback */

               for(d=0;transtab[d].u;d++)
                  if(transtab[d].u == uc) break;

               if(transtab[d].u)
               {
                  if(strlen(transtab[d].str) == 1)
                  {
                     printbyte(0); 
                     printf("\t"); 
                     printbyte(transtab[d].str[0]);
                     printf("\n");
                  }
                  else
                  {
                     if(strlen(transtab[d].str) > 0) { printbyte(transtab[d].str[0]); }
                     if(strlen(transtab[d].str) > 1) { printf("\t"); printbyte(transtab[d].str[1]); }
                     if(strlen(transtab[d].str) > 2) { printf("\t"); printbyte(transtab[d].str[2]); }
                     if(strlen(transtab[d].str) > 3) { printf("\t"); printbyte(transtab[d].str[3]); }
                     if(strlen(transtab[d].str) > 4) { fprintf(stderr,"Warning: fallback %s too long\n",transtab[d].str); }
                     printf("\n");
                  }
               }
               else
               {
                  printbyte(0);
                  printf("\t");
                  printbyte('?');
                  printf("\n");
               }
            }
         }
      }
   }

   printf("END\n");
   
   exit(0);           
}










