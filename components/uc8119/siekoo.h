#pragma once
// =============================================================================
// Siekoo 7-Segment Alphabet by Alexander Fakoó
// https://fakoo.de/en/siekoo.html
//
// SPDX-License-Identifier: CC-BY-NC-ND-4.0
// Copyright (c) Alexander Fakoó
//
// This file is licensed separately from the rest of the project (which is MIT).
// It may NOT be used commercially or distributed in modified form.
// This file is OPTIONAL — use font: classic to avoid it.
//
// A confusion-free alpha-numeric 7-segment character set where every letter
// and digit has a UNIQUE representation — no ambiguity between similar
// characters like 0/O, 1/I, 2/Z, 5/S, 6/G, 8/B.
//
// Encoding: bit6=A(top), bit5=B(top-right), bit4=C(bot-right),
//           bit3=D(bottom), bit2=E(bot-left), bit1=F(top-left), bit0=G(middle)
// =============================================================================

namespace esphome {
namespace uc8119 {

// ─── Digits ─────────────────────────────────────────────────────
//  (standard 7-segment, same as non-Siekoo)

static const uint8_t SK_0 = 0x7E;  // ABCDEF      ─┐  ┌─
static const uint8_t SK_1 = 0x30;  // BC            │  │
static const uint8_t SK_2 = 0x6D;  // ABDEG        ─┤  ├─
static const uint8_t SK_3 = 0x79;  // ABCDG         │  │
static const uint8_t SK_4 = 0x33;  // BCFG         ─┘  └─
static const uint8_t SK_5 = 0x5B;  // ACDFG
static const uint8_t SK_6 = 0x5F;  // ACDEFG
static const uint8_t SK_7 = 0x70;  // ABC
static const uint8_t SK_8 = 0x7F;  // ABCDEFG
static const uint8_t SK_9 = 0x7B;  // ABCDFG

// ─── Letters ────────────────────────────────────────────────────
//  Siekoo-specific: each letter differs from every digit and
//  from every other letter. No O/0, I/1, S/5, Z/2 confusion.

static const uint8_t SK_A = 0x7D;  // ABCDEG       ─┐  ┌─    (differs from 8: no F)
static const uint8_t SK_B = 0x1F;  // CDEFG         │  │     (differs from 8: no A,B)
static const uint8_t SK_C = 0x0D;  // DEG          ─┘  └─    (differs from ( : no A,F)
static const uint8_t SK_D = 0x3D;  // BCDEG
static const uint8_t SK_E = 0x4F;  // ADEFG
static const uint8_t SK_F = 0x47;  // AEFG
static const uint8_t SK_G = 0x5E;  // ACDEF                  (differs from 6: no G)
static const uint8_t SK_H = 0x17;  // CEFG                   (differs from std H: no B,F)
static const uint8_t SK_I = 0x44;  // AE                     (differs from 1: no B,C)
static const uint8_t SK_J = 0x58;  // ACD                    (differs from std J: has A)
static const uint8_t SK_K = 0x57;  // ACEFG
static const uint8_t SK_L = 0x0E;  // DEF
static const uint8_t SK_M = 0x55;  // ACEG
static const uint8_t SK_N = 0x15;  // CEG
static const uint8_t SK_O = 0x1D;  // CDEG                   (differs from 0: no A,B,F)
static const uint8_t SK_P = 0x67;  // ABEFG
static const uint8_t SK_Q = 0x73;  // ABCFG
static const uint8_t SK_R = 0x05;  // EG
static const uint8_t SK_S = 0x5A;  // ACDF                   (differs from 5: no G)
static const uint8_t SK_T = 0x0F;  // DEFG
static const uint8_t SK_U = 0x1C;  // CDE                    (differs from std U: no B,F)
static const uint8_t SK_V = 0x2A;  // BDF
static const uint8_t SK_W = 0x2B;  // BDFG
static const uint8_t SK_X = 0x14;  // CE
static const uint8_t SK_Y = 0x3B;  // BCDFG
static const uint8_t SK_Z = 0x6C;  // ABDE                   (differs from 2: no G)

// ─── Special Characters ─────────────────────────────────────────

static const uint8_t SK_DOT       = 0x04;  // E           .
static const uint8_t SK_COMMA     = 0x18;  // CD          ,
static const uint8_t SK_SEMICOLON = 0x28;  // BD          ;
static const uint8_t SK_COLON     = 0x48;  // AD          :
static const uint8_t SK_EXCLAIM   = 0x6B;  // ABDFG       !
static const uint8_t SK_QUESTION  = 0x69;  // ABDG        ?
static const uint8_t SK_PLUS      = 0x31;  // BCG         +
static const uint8_t SK_MINUS     = 0x01;  // G           -
static const uint8_t SK_EQUAL     = 0x09;  // DG          =
static const uint8_t SK_ASTERISK  = 0x49;  // ADG         *
static const uint8_t SK_HASH      = 0x36;  // BCEF        #
static const uint8_t SK_SLASH     = 0x25;  // BEG         /
static const uint8_t SK_BACKSLASH = 0x13;  // CFG         \.
static const uint8_t SK_OVERLINE  = 0x40;  // A           ¯
static const uint8_t SK_UNDERSCR  = 0x08;  // D           _
static const uint8_t SK_LPAREN    = 0x4E;  // ADEF        (
static const uint8_t SK_RPAREN    = 0x78;  // ABCD        )
static const uint8_t SK_SQUOTE    = 0x02;  // F           '
static const uint8_t SK_DQUOTE    = 0x22;  // BF          "
static const uint8_t SK_DEGREE    = 0x63;  // ABFG        °
static const uint8_t SK_LESS      = 0x42;  // AF          <
static const uint8_t SK_GREATER   = 0x60;  // AB          >
static const uint8_t SK_PERCENT   = 0x12;  // CF          %
static const uint8_t SK_AT        = 0x74;  // ABCE        @
static const uint8_t SK_BLANK     = 0x00;  //             (space)

// ─── Lookup tables ──────────────────────────────────────────────

static const uint8_t SK_DIGITS[] = {
  SK_0, SK_1, SK_2, SK_3, SK_4, SK_5, SK_6, SK_7, SK_8, SK_9
};

static const uint8_t SK_LETTERS[] = {
  SK_A, SK_B, SK_C, SK_D, SK_E, SK_F, SK_G, SK_H, SK_I, SK_J,
  SK_K, SK_L, SK_M, SK_N, SK_O, SK_P, SK_Q, SK_R, SK_S, SK_T,
  SK_U, SK_V, SK_W, SK_X, SK_Y, SK_Z
};

/// Convert ASCII char to Siekoo 7-segment code
/// Returns SK_BLANK for unsupported characters
inline uint8_t siekoo_encode(char c) {
  if (c >= '0' && c <= '9') return SK_DIGITS[c - '0'];
  if (c >= 'A' && c <= 'Z') return SK_LETTERS[c - 'A'];
  if (c >= 'a' && c <= 'z') return SK_LETTERS[c - 'a'];
  switch (c) {
    case ' ': return SK_BLANK;
    case '.': return SK_DOT;
    case ',': return SK_COMMA;
    case ';': return SK_SEMICOLON;
    case ':': return SK_COLON;
    case '!': return SK_EXCLAIM;
    case '?': return SK_QUESTION;
    case '+': return SK_PLUS;
    case '-': return SK_MINUS;
    case '=': return SK_EQUAL;
    case '*': return SK_ASTERISK;
    case '#': return SK_HASH;
    case '/': return SK_SLASH;
    case '\\': return SK_BACKSLASH;
    case '_': return SK_UNDERSCR;
    case '(': return SK_LPAREN;
    case ')': return SK_RPAREN;
    case '\'': return SK_SQUOTE;
    case '"': return SK_DQUOTE;
    case '<': return SK_LESS;
    case '>': return SK_GREATER;
    case '%': return SK_PERCENT;
    case '@': return SK_AT;
    default:  return SK_BLANK;
  }
}

}  // namespace uc8119
}  // namespace esphome
