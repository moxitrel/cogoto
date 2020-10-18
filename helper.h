/*

MAP(F, ...):

*/
#ifndef MOXITREL_COGO_HELPER_H_
#define MOXITREL_COGO_HELPER_H_

// Get nth element of padding. (BUG)
// e.g. ARG_GET1((p1)      , (PADDING))  -> P1
//      ARG_GET1((a1,b2)   , (PADDING))  -> P2
//      ARG_GET1((x1,y1,z1), (PADDING))  -> P3
// BUG: ARG_GET1(()        , (PADDING))  -> P1, expect P0
#define ARG_GET1(PAREN_ARG, PAREN_PADDING)                  \
    ARG_PATT_GET(REMOVE_PAREN PAREN_ARG, REMOVE_PAREN PAREN_PADDING)
#define ARG_PATT_GET(...)  ARG_PATT_GET_(__VA_ARGS__)
#define ARG_PATT_GET_(                                      \
     _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9, _10,       \
    _11, _12, _13, _14, _15, _16, _17, _18, _19,   N, ...)  N
#define REMOVE_PAREN(...)  __VA_ARGS__

// Get nth element of padding. (Solved the ARG_GET1(...)'s BUG)
//
// e.g. ARG_GET(()     , (PADDING)) -> ARG_GET_01N(0,1,P0,P1,P1) -> ARG_GET_01(P0,P1,P1) -> P0
//      ARG_GET((1)    , (PADDING)) -> ARG_GET_01N(0,0,P0,P1,P1) -> ARG_GET_00(P0,P1,P1) -> P1
//      ARG_GET((x,?)  , (PADDING)) -> ARG_GET_01N(1,1,P0,P1,P2) -> ARG_GET_11(P0,P1,P2) -> P2
//      ARG_GET((a,b,_), (PADDING)) -> ARG_GET_01N(1,1,P0,P1,P3) -> ARG_GET_11(P0,P1,P3) -> P3
//      ARG_GET((...)  , (PADDING)) -> ARG_GET_01N(1,1,P0,P1,PN) -> ARG_GET_11(P0,P1,PN) -> PN
//
// See: https://stackoverflow.com/questions/11317474/macro-to-count-number-of-arguments
//      http://p99.gforge.inria.fr/p99-html/p99__args_8h_source.html
#define ARG_GET(PAREN_ARG, PAREN_PADDING)                   \
    ARG_GET_01N(                                            \
        HAS_COMMA(REMOVE_PAREN PAREN_ARG),                  \
        HAS_COMMA(GET_COMMA REMOVE_PAREN PAREN_ARG ()),     \
        ARG_PATT_GET( REMOVE_PAREN PAREN_PADDING),          \
        ARG_PATT_GET(,REMOVE_PAREN PAREN_PADDING),          \
        ARG_GET1(PAREN_ARG, PAREN_PADDING)                  \
    )
#define ARG_GET_01N(...)            ARG_GET_01N_(__VA_ARGS__)
#define ARG_GET_01N_(D1, D2, ...)   ARG_GET_##D1##D2(__VA_ARGS__)
#define ARG_GET_01(O,I,N)           O
#define ARG_GET_00(O,I,N)           I
#define ARG_GET_11(O,I,N)           N
#define HAS_COMMA(...)              ARG_PATT_GET(__VA_ARGS__, HAS_COMMA_PADDING)
#define GET_COMMA(...)              ,
#define HAS_COMMA_PADDING                                   \
      1,   1,   1,   1,   1,   1,   1,   1,   1,   1,       \
      1,   1,   1,   1,   1,   1,   1,   1,   0,   0



// Count the number of arguments.
#define ARG_COUNT(...)      ARG_COUNT_(__VA_ARGS__)
#define ARG_COUNT_(...)     ARG_GET((__VA_ARGS__),(ARG_COUNT_PADDING))
#define ARG_COUNT_PADDING                                   \
     19,  18,  17,  16,  15,  14,  13,  12,  11,  10,       \
      9,   8,   7,   6,   5,   4,   3,   2,   1,   0

// Is arg list empty?
#define ARG_EMPTY(...)      ARG_EMPTY_(__VA_ARGS__)
#define ARG_EMPTY_(...)     ARG_GET((__VA_ARGS__),(ARG_EMPTY_PADDING))
#define ARG_EMPTY_PADDING                                   \
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       \
      0,   0,   0,   0,   0,   0,   0,   0,   0,   1

//e.g. MAP(;, CASE_GOTO, 10, 20, 30)
//     -> MAP_3(;, CASE_GOTO, 10, 20, 30)
//     -> CASE_GOTO(10);
//        CASE_GOTO(20);
//        CASE_GOTO(30)
#define MAP(SEP, F, ...)    MAP_N(ARG_COUNT(__VA_ARGS__), SEP, F, __VA_ARGS__)
#define MAP_N(...)          MAP_N_(__VA_ARGS__)
#define MAP_N_(N, ...)      MAP_##N(__VA_ARGS__)
#define MAP_0(...)
#define MAP_1( SEP, F, ...)      F(__VA_ARGS__)
#define MAP_2( SEP, F, X, ...)   F(X) SEP MAP_1( SEP, F, __VA_ARGS__)
#define MAP_3( SEP, F, X, ...)   F(X) SEP MAP_2( SEP, F, __VA_ARGS__)
#define MAP_4( SEP, F, X, ...)   F(X) SEP MAP_3( SEP, F, __VA_ARGS__)
#define MAP_5( SEP, F, X, ...)   F(X) SEP MAP_4( SEP, F, __VA_ARGS__)
#define MAP_6( SEP, F, X, ...)   F(X) SEP MAP_5( SEP, F, __VA_ARGS__)
#define MAP_7( SEP, F, X, ...)   F(X) SEP MAP_6( SEP, F, __VA_ARGS__)
#define MAP_8( SEP, F, X, ...)   F(X) SEP MAP_7( SEP, F, __VA_ARGS__)
#define MAP_9( SEP, F, X, ...)   F(X) SEP MAP_8( SEP, F, __VA_ARGS__)
#define MAP_10(SEP, F, X, ...)   F(X) SEP MAP_9( SEP, F, __VA_ARGS__)
#define MAP_11(SEP, F, X, ...)   F(X) SEP MAP_10(SEP, F, __VA_ARGS__)
#define MAP_12(SEP, F, X, ...)   F(X) SEP MAP_11(SEP, F, __VA_ARGS__)
#define MAP_13(SEP, F, X, ...)   F(X) SEP MAP_12(SEP, F, __VA_ARGS__)
#define MAP_14(SEP, F, X, ...)   F(X) SEP MAP_13(SEP, F, __VA_ARGS__)
#define MAP_15(SEP, F, X, ...)   F(X) SEP MAP_14(SEP, F, __VA_ARGS__)
#define MAP_16(SEP, F, X, ...)   F(X) SEP MAP_15(SEP, F, __VA_ARGS__)
#define MAP_17(SEP, F, X, ...)   F(X) SEP MAP_16(SEP, F, __VA_ARGS__)
#define MAP_18(SEP, F, X, ...)   F(X) SEP MAP_17(SEP, F, __VA_ARGS__)
#define MAP_19(SEP, F, X, ...)   F(X) SEP MAP_18(SEP, F, __VA_ARGS__)

// IFNIL(ID)(SK,FK): Expand to <SK> if ID defined as empty, i.e. "#define ID", else FK
#define IFNIL(...)                  IFNIL_BOOL(ARG_EMPTY(__VA_ARGS__))
#define IFNIL_BOOL(...)             IFNIL_BOOL_(__VA_ARGS__)
#define IFNIL_BOOL_(BOOL)           IFNIL_BOOL_##BOOL
#define IFNIL_BOOL_1(...)           IFNIL_BOOL_1_(__VA_ARGS__)
#define IFNIL_BOOL_0(...)           IFNIL_BOOL_0_(__VA_ARGS__)
#define IFNIL_BOOL_1_(SK,...)       SK
#define IFNIL_BOOL_0_(SK,...)       IFNIL_BOOL_0_BOOL(ARG_EMPTY(__VA_ARGS__), __VA_ARGS__)
#define IFNIL_BOOL_0_BOOL(...)      IFNIL_BOOL_0_BOOL_(__VA_ARGS__)
#define IFNIL_BOOL_0_BOOL_(N,...)   IFNIL_BOOL_0_BOOL_##N(__VA_ARGS__)
#define IFNIL_BOOL_0_BOOL_1(...)
#define IFNIL_BOOL_0_BOOL_0(FK)     FK

#define ID(X)   X

#endif // MOXITREL_COGO_HELPER_H_
