#ifndef MOXITREL_COGO_UTILS_H_
#define MOXITREL_COGO_UTILS_H_

// Get nth element of padding. (BUG)
// e.g. COGO_ARG_GET1((p1)      , (PADDING))  -> P1
//      COGO_ARG_GET1((a1,b2)   , (PADDING))  -> P2
//      COGO_ARG_GET1((x1,y1,z1), (PADDING))  -> P3
// BUG: COGO_ARG_GET1(()        , (PADDING))  -> P1, expect P0
#define COGO_ARG_GET1(PAREN_ARG, PAREN_PADDING)                     \
    COGO_ARG_PATT_GET(COGO_REMOVE_PAREN PAREN_ARG, COGO_REMOVE_PAREN PAREN_PADDING)
#define COGO_ARG_PATT_GET(...)  COGO_ARG_PATT_GET1(__VA_ARGS__)
#define COGO_ARG_PATT_GET1(                                         \
     _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9, _10,               \
    _11, _12, _13, _14, _15, _16, _17, _18, _19,   N, ...)  N
#define COGO_REMOVE_PAREN(...)  __VA_ARGS__

// Get nth element of padding. (Solved the COGO_ARG_GET1(...)'s BUG)
//
// e.g. COGO_ARG_GET(()     , (PADDING)) -> COGO_ARG_GET_01N(0,1,P0,P1,P1) -> ARG_GET_01(P0,P1,P1) -> P0
//      COGO_ARG_GET((1)    , (PADDING)) -> COGO_ARG_GET_01N(0,0,P0,P1,P1) -> ARG_GET_00(P0,P1,P1) -> P1
//      COGO_ARG_GET((x,?)  , (PADDING)) -> COGO_ARG_GET_01N(1,1,P0,P1,P2) -> ARG_GET_11(P0,P1,P2) -> P2
//      COGO_ARG_GET((a,b,_), (PADDING)) -> COGO_ARG_GET_01N(1,1,P0,P1,P3) -> ARG_GET_11(P0,P1,P3) -> P3
//      COGO_ARG_GET((...)  , (PADDING)) -> COGO_ARG_GET_01N(1,1,P0,P1,PN) -> ARG_GET_11(P0,P1,PN) -> PN
//
// See: https://stackoverflow.com/questions/11317474/macro-to-count-number-of-arguments
//      http://p99.gforge.inria.fr/p99-html/p99__args_8h_source.html
#define COGO_ARG_GET(PAREN_ARG, PAREN_PADDING)                          \
    COGO_ARG_GET_01N(                                                   \
        COGO_HAS_COMMA(COGO_REMOVE_PAREN PAREN_ARG),                    \
        COGO_HAS_COMMA(COGO_GET_COMMA COGO_REMOVE_PAREN PAREN_ARG ()),  \
        COGO_ARG_PATT_GET( COGO_REMOVE_PAREN PAREN_PADDING),            \
        COGO_ARG_PATT_GET(,COGO_REMOVE_PAREN PAREN_PADDING),            \
        COGO_ARG_GET1(PAREN_ARG, PAREN_PADDING)                         \
    )
#define COGO_ARG_GET_01N(...)            COGO_ARG_GET_01N1(__VA_ARGS__)
#define COGO_ARG_GET_01N1(D1, D2, ...)   COGO_ARG_GET_##D1##D2(__VA_ARGS__)
#define COGO_ARG_GET_01(O,I,N)           O
#define COGO_ARG_GET_00(O,I,N)           I
#define COGO_ARG_GET_11(O,I,N)           N
#define COGO_HAS_COMMA(...)              COGO_ARG_PATT_GET(__VA_ARGS__, COGO_HAS_COMMA_PADDING)
#define COGO_GET_COMMA(...)              ,
#define COGO_HAS_COMMA_PADDING                              \
      1,   1,   1,   1,   1,   1,   1,   1,   1,   1,       \
      1,   1,   1,   1,   1,   1,   1,   1,   0,   0



// Count the number of arguments.
#define COGO_ARG_COUNT(...)      COGO_ARG_COUNT_(__VA_ARGS__)
#define COGO_ARG_COUNT_(...)     COGO_ARG_GET((__VA_ARGS__),(COGO_ARG_COUNT_PADDING))
#define COGO_ARG_COUNT_PADDING                              \
     19,  18,  17,  16,  15,  14,  13,  12,  11,  10,       \
      9,   8,   7,   6,   5,   4,   3,   2,   1,   0

// Is arg list empty?
#define COGO_ARG_EMPTY(...)      COGO_ARG_EMPTY1(__VA_ARGS__)
#define COGO_ARG_EMPTY1(...)     COGO_ARG_GET((__VA_ARGS__),(COGO_ARG_EMPTY1PADDING))
#define COGO_ARG_EMPTY1PADDING                              \
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       \
      0,   0,   0,   0,   0,   0,   0,   0,   0,   1

//e.g. COGO_MAP(;, CASE_GOTO, 10, 20, 30)
//     -> COGO_MAP_3(;, CASE_GOTO, 10, 20, 30)
//     -> CASE_GOTO(10);
//        CASE_GOTO(20);
//        CASE_GOTO(30)
#define COGO_MAP(SEP, F, ...)    COGO_MAP_N(COGO_ARG_COUNT(__VA_ARGS__), SEP, F, __VA_ARGS__)
#define COGO_MAP_N(...)          COGO_MAP_N1(__VA_ARGS__)
#define COGO_MAP_N1(N, ...)      COGO_MAP_##N(__VA_ARGS__)
#define COGO_MAP_0(...)
#define COGO_MAP_1( SEP, F, ...)      F(__VA_ARGS__)
#define COGO_MAP_2( SEP, F, X, ...)   F(X) SEP COGO_MAP_1( SEP, F, __VA_ARGS__)
#define COGO_MAP_3( SEP, F, X, ...)   F(X) SEP COGO_MAP_2( SEP, F, __VA_ARGS__)
#define COGO_MAP_4( SEP, F, X, ...)   F(X) SEP COGO_MAP_3( SEP, F, __VA_ARGS__)
#define COGO_MAP_5( SEP, F, X, ...)   F(X) SEP COGO_MAP_4( SEP, F, __VA_ARGS__)
#define COGO_MAP_6( SEP, F, X, ...)   F(X) SEP COGO_MAP_5( SEP, F, __VA_ARGS__)
#define COGO_MAP_7( SEP, F, X, ...)   F(X) SEP COGO_MAP_6( SEP, F, __VA_ARGS__)
#define COGO_MAP_8( SEP, F, X, ...)   F(X) SEP COGO_MAP_7( SEP, F, __VA_ARGS__)
#define COGO_MAP_9( SEP, F, X, ...)   F(X) SEP COGO_MAP_8( SEP, F, __VA_ARGS__)
#define COGO_MAP_10(SEP, F, X, ...)   F(X) SEP COGO_MAP_9( SEP, F, __VA_ARGS__)
#define COGO_MAP_11(SEP, F, X, ...)   F(X) SEP COGO_MAP_10(SEP, F, __VA_ARGS__)
#define COGO_MAP_12(SEP, F, X, ...)   F(X) SEP COGO_MAP_11(SEP, F, __VA_ARGS__)
#define COGO_MAP_13(SEP, F, X, ...)   F(X) SEP COGO_MAP_12(SEP, F, __VA_ARGS__)
#define COGO_MAP_14(SEP, F, X, ...)   F(X) SEP COGO_MAP_13(SEP, F, __VA_ARGS__)
#define COGO_MAP_15(SEP, F, X, ...)   F(X) SEP COGO_MAP_14(SEP, F, __VA_ARGS__)
#define COGO_MAP_16(SEP, F, X, ...)   F(X) SEP COGO_MAP_15(SEP, F, __VA_ARGS__)
#define COGO_MAP_17(SEP, F, X, ...)   F(X) SEP COGO_MAP_16(SEP, F, __VA_ARGS__)
#define COGO_MAP_18(SEP, F, X, ...)   F(X) SEP COGO_MAP_17(SEP, F, __VA_ARGS__)
#define COGO_MAP_19(SEP, F, X, ...)   F(X) SEP COGO_MAP_18(SEP, F, __VA_ARGS__)

// COGO_IFNIL(ID)(SK,FK): Expand to <SK> if ID defined as empty, i.e. "#define ID", else FK
#define COGO_IFNIL(...)                  COGO_IFNIL_BOOL(COGO_ARG_EMPTY(__VA_ARGS__))
#define COGO_IFNIL_BOOL(...)             COGO_IFNIL_BOOL1(__VA_ARGS__)
#define COGO_IFNIL_BOOL1(BOOL)           COGO_IFNIL_BOOL_##BOOL
#define COGO_IFNIL_BOOL_1(...)           COGO_IFNIL_BOOL_1_(__VA_ARGS__)
#define COGO_IFNIL_BOOL_0(...)           COGO_IFNIL_BOOL_0_(__VA_ARGS__)
#define COGO_IFNIL_BOOL_1_(SK,...)       SK
#define COGO_IFNIL_BOOL_0_(SK,...)       COGO_IFNIL_BOOL_0_BOOL(COGO_ARG_EMPTY(__VA_ARGS__), __VA_ARGS__)
#define COGO_IFNIL_BOOL_0_BOOL(...)      COGO_IFNIL_BOOL_0_BOOL_(__VA_ARGS__)
#define COGO_IFNIL_BOOL_0_BOOL_(N,...)   COGO_IFNIL_BOOL_0_BOOL_##N(__VA_ARGS__)
#define COGO_IFNIL_BOOL_0_BOOL_1(...)
#define COGO_IFNIL_BOOL_0_BOOL_0(FK)     FK

#define COGO_ID(X)   X

#endif // MOXITREL_COGO_UTILS_H_
