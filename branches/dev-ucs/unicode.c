// unicode.c (part of MinTTY)
// Copyright 2008-09  Andy Koppe
// Based on code from PuTTY-0.60 by Simon Tatham and team.
// Licensed under the terms of the GNU General Public License v3 or later.

#include "unicode.h"

#include "config.h"

#include <winbase.h>
#include <winnls.h>

unicode_data ucsdata;

struct cp_list_item {
  int codepage;
  char *name;
};

static const struct cp_list_item cp_list[] = {
  {CP_UTF8, "UTF-8"},
  {28591, "ISO-8859-1:1998 (Latin-1, West Europe)"},
  {28592, "ISO-8859-2:1999 (Latin-2, East Europe)"},
  {28593, "ISO-8859-3:1999 (Latin-3, South Europe)"},
  {28594, "ISO-8859-4:1998 (Latin-4, North Europe)"},
  {28595, "ISO-8859-5:1999 (Latin/Cyrillic)"},
  {28596, "ISO-8859-6:1999 (Latin/Arabic)"},
  {28597, "ISO-8859-7:1987 (Latin/Greek)"},
  {28598, "ISO-8859-8:1999 (Latin/Hebrew}"},
  {28599, "ISO-8859-9:1999 (Latin-5, Turkish)"},
  {28603, "ISO-8859-13:1998 (Latin-7, Baltic)"},
  {28605, "ISO-8859-15:1999 (Latin-9, \"euro\")"},
  {1250, "Win1250 (Central European)"},
  {1251, "Win1251 (Cyrillic)"},
  {1252, "Win1252 (Western)"},
  {1253, "Win1253 (Greek)"},
  {1254, "Win1254 (Turkish)"},
  {1255, "Win1255 (Hebrew)"},
  {1256, "Win1256 (Arabic)"},
  {1257, "Win1257 (Baltic)"},
  {1258, "Win1258 (Vietnamese)"},
  {20866, "Russian (KOI8-R)"},
  {21866, "Ukrainian (KOI8-U)"},
  {0, 0}
};

void
init_ucs(void)
{
  char tbuf[256];
  
  for (int i = 0; i < 256; i++)
    tbuf[i] = i;

 /* Decide on the Line and Font codepages */
  ucsdata.codepage = decode_codepage(cfg.codepage);
}

int
decode_codepage(char *cp_name)
{
  char *s, *d;
  const struct cp_list_item *cpi;
  int codepage = -1;
  CPINFO cpinfo;

  if (!*cp_name)
    return unicode_codepage;

  if (cp_name && *cp_name)
    for (cpi = cp_list; cpi->name; cpi++) {
      s = cp_name;
      d = cpi->name;
      for (;;) {
        while (*s && !isalnum((uchar)*s) && *s != ':')
          s++;
        while (*d && !isalnum((uchar)*d) && *d != ':')
          d++;
        if (*s == 0) {
          codepage = cpi->codepage;
          if (codepage == CP_UTF8)
            goto break_break;
          if (codepage == -1)
            return -1;
          if (GetCPInfo(codepage, &cpinfo) != 0)
            goto break_break;
        }
        if (tolower((uchar)*s++) != tolower((uchar)*d++))
          break;
      }
    }

  if (cp_name && *cp_name) {
    d = cp_name;
    if (tolower((uchar)d[0]) == 'c' && 
	    tolower((uchar)d[1]) == 'p')
      d += 2;
    if (tolower((uchar)d[0]) == 'i' && 
	    tolower((uchar)d[1]) == 'b' && 
	    tolower((uchar)d[2]) == 'm')
      d += 3;
    for (s = d; *s >= '0' && *s <= '9'; s++);
    if (*s == 0 && s != d)
      codepage = atoi(d);       /* CP999 or IBM999 */

    if (codepage == CP_ACP)
      codepage = GetACP();
    if (codepage == CP_OEMCP)
      codepage = GetOEMCP();
  }

break_break:;
  if (codepage != -1) {
    if (codepage != CP_UTF8) {
      if (GetCPInfo(codepage, &cpinfo) == 0) {
        codepage = -2;
      }
      else if (cpinfo.MaxCharSize > 1)
        codepage = -3;
    }
  }
  if (codepage == -1 && *cp_name)
    codepage = -2;
  return codepage;
}

const char *
cp_name(int codepage)
{
  const struct cp_list_item *cpi;
  static char buf[32];

  if (codepage == -1) {
    sprintf(buf, "Use font encoding");
    return buf;
  }

  if (codepage > 0)
    sprintf(buf, "CP%03d", codepage);
  else
    *buf = 0;

  for (cpi = cp_list; cpi->name; cpi++) {
    if (codepage == cpi->codepage)
      return cpi->name;
  }

  return buf;
}

/*
 * Return the nth code page in the list, for use in the GUI
 * configurer.
 */
const char *
cp_enumerate(int index)
{
  if (index < 0 || index >= (int) lengthof(cp_list))
    return null;
  return cp_list[index].name;
}

void
get_unitab(int codepage, wchar * unitab, int ftype)
{
  char tbuf[4];
  int i, max = 256, flg = MB_ERR_INVALID_CHARS;

  if (ftype)
    flg |= MB_USEGLYPHCHARS;
  if (ftype == 2)
    max = 128;

  if (codepage == CP_UTF8) {
    for (i = 0; i < max; i++)
      unitab[i] = i;
    return;
  }

  if (codepage == CP_ACP)
    codepage = GetACP();
  else if (codepage == CP_OEMCP)
    codepage = GetOEMCP();

  if (codepage > 0) {
    for (i = 0; i < max; i++) {
      tbuf[0] = i;
      if (mb_to_wc(codepage, flg, tbuf, 1, unitab + i, 1) != 1)
        unitab[i] = 0xFFFD;
    }
  }
}

int
wc_to_mb(int codepage, int flags, const wchar * wcstr, int wclen,
                                  char *mbstr, int mblen)
{
  return WideCharToMultiByte(codepage, flags, wcstr, wclen, mbstr, mblen, 0, 0);
}

int
mb_to_wc(int codepage, int flags, const char *mbstr, int mblen,
                                  wchar * wcstr, int wclen)
{
  return MultiByteToWideChar(codepage, flags, mbstr, mblen, wcstr, wclen);
}

int
is_dbcs_leadbyte(int codepage, char byte)
{
  return IsDBCSLeadByteEx(codepage, byte);
}

int
wordtype(int c)
{
  struct ucsword {
    wchar start, end; 
    uchar ctype;
  };
  static const struct ucsword ucs_words[] = {
    {128, 160, 0},
    {161, 191, 1},
    {215, 215, 1},
    {247, 247, 1},
    {0x037e, 0x037e, 1},        /* Greek question mark */
    {0x0387, 0x0387, 1},        /* Greek ano teleia */
    {0x055a, 0x055f, 1},        /* Armenian punctuation */
    {0x0589, 0x0589, 1},        /* Armenian full stop */
    {0x0700, 0x070d, 1},        /* Syriac punctuation */
    {0x104a, 0x104f, 1},        /* Myanmar punctuation */
    {0x10fb, 0x10fb, 1},        /* Georgian punctuation */
    {0x1361, 0x1368, 1},        /* Ethiopic punctuation */
    {0x166d, 0x166e, 1},        /* Canadian Syl. punctuation */
    {0x17d4, 0x17dc, 1},        /* Khmer punctuation */
    {0x1800, 0x180a, 1},        /* Mongolian punctuation */
    {0x2000, 0x200a, 0},        /* Various spaces */
    {0x200b, 0x27ff, 1},        /* punctuation and symbols */
    {0x3000, 0x3000, 0},        /* ideographic space */
    {0x3001, 0x3020, 1},        /* ideographic punctuation */
    {0x303f, 0x309f, 3},        /* Hiragana */
    {0x30a0, 0x30ff, 3},        /* Katakana */
    {0x3300, 0x9fff, 3},        /* CJK Ideographs */
    {0xac00, 0xd7a3, 3},        /* Hangul Syllables */
    {0xf900, 0xfaff, 3},        /* CJK Ideographs */
    {0xfe30, 0xfe6b, 1},        /* punctuation forms */
    {0xff00, 0xff0f, 1},        /* half/fullwidth ASCII */
    {0xff1a, 0xff20, 1},        /* half/fullwidth ASCII */
    {0xff3b, 0xff40, 1},        /* half/fullwidth ASCII */
    {0xff5b, 0xff64, 1},        /* half/fullwidth ASCII */
    {0xfff0, 0xffff, 0},        /* half/fullwidth ASCII */
    {0, 0, 0}
  };
  if (c < 0x80) {
    if (c <= ' ' || c == 0x7f)
      return 0;
    else if (isalnum(c) || strchr("#-./\\_~", c))
      return 2;
    else
      return 1;
  }
  for (const struct ucsword *wptr = ucs_words; wptr->start; wptr++) {
    if (c >= wptr->start && c <= wptr->end)
      return wptr->ctype;
  }
  return 2;
}