#pragma once

//
// Retrieve the type
//
//  ARGTYPE( (ArgType) argName )                         => ArgType
//
// http://stackoverflow.com/questions/41453/how-can-i-add-reflection-to-a-c-application
//
#define ARGTYPE(x) ARGTYPE_PASS2(ARGTYPE_PASS1 x,)
//
//      => ARGTYPE_PASS2(ARGTYPE_PASS1 (ArgType) argName,)
//
#define ARGTYPE_PASS1(...) (__VA_ARGS__),
//
//      => ARGTYPE_PASS2( (ArgType), argName,)
//
#define ARGTYPE_PASS2(...) ARGTYPE_PASS3((__VA_ARGS__))
//
//      => ARGTYPE_PASS2(( (ArgType), argName,))
//
#define ARGTYPE_PASS3(x)   ARGTYPE_PASS4 x
//
//      => ARGTYPE_PASS4 ( (ArgType), argName,)
//
#define ARGTYPE_PASS4(x, ...) REM x
//
//      => REM (ArgType)
//
#define REM(...) __VA_ARGS__
//
//      => ArgType
//

//
// This counts the number of args: (0 is also supported)
//
//
//  NARGS( (ArgType1) argName1, (ArgType2) argName2 )   => 2
//
#define NARGS(...) NARGS_PASS2(NARGS_PASS1(__VA_ARGS__))
//
//  => NARGS_PASS2(NARGS_PASS1( (ArgType1) argName1, (ArgType2) argName2 ) )
//
#define NARGS_PASS1(...) unused, __VA_ARGS__
//
//  => NARGS_PASS2( unused, (ArgType1) argName1, (ArgType2) argName2 )
//
#define NARGS_PASS2(...) NARGS_PASS4(NARGS_PASS3(__VA_ARGS__, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
//
//  => NARGS_PASS4(NARGS_PASS3( unused, (ArgType1) argName1, (ArgType2) argName2 ) , 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0) )
//
#define NARGS_PASS3(_unused,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,VAL, ...) VAL
//
//  => NARGS_PASS4(2)
//
#define NARGS_PASS4(x) x
//
//  => 2
//


//
// Show the type without parenthesis
//
//  ARGPAIR( (ArgType1) argName1 )                         => ArgType1 argName1
//
#define ARGPAIR(x) REM x
//
//  => REM (ArgType1) argName1
//
//  => ArgType1 argName1
//

//
// Show the type without parenthesis
//
//  ARGPAIR( (ArgType1) argName1 )                         => ArgType1 && argName1
//
#define REFARGPAIR(x) REFREM x
//
//  => REFREM (ArgType1) argName1

#define REFREM(...) __VA_ARGS__ &&
//
//  => ArgType1 && argName1
//

//
// Strip off the type
//
//  ARGNAME( (ArgType1) argName1 )                      => argName1
//
#define ARGNAME(x) EAT x
//
//      => EAT (ArgType1) argName1
//
#define EAT(...)
//
//      => argName1
//

//
// Strip off the type
//
//  ARGNAME( (ArgType1) argName1 )                      => argName1
//
#define ARGNAME_AS_STRING(x) TOSTRING(EAT x)
//
//      => EAT (ArgType1) argName1
//
#define TOSTRING2(x)  #x
#define TOSTRING(x)   TOSTRING2(x)


//
// This will call a macro on each argument passed in
//
//  DOFOREACH(typename ARGTYPE, (ArgType1) argName1, (ArgType1) argName2 )
//
#define DOFOREACH(macro, ...) DOFOREACH_PASS1(CAT_TOKENS(DOFOREACH_, NARGS(__VA_ARGS__)), (macro, __VA_ARGS__))
//
//          => DOFOREACH_PASS1(CAT_TOKENS(DOFOREACH_, NARGS( (ArgType1) argName1, (ArgType1) argName2 ) ), (typename ARGTYPE, (ArgType1) argName1, (ArgType1) argName2 ) ))
//          => DOFOREACH_PASS1(CAT_TOKENS(DOFOREACH_, 2), (typename ARGTYPE, (ArgType1) argName1, (ArgType1) argName2 ) ))
//
#define CAT_TOKENS(x, y) CAT_PASS1((x, y))
//
//          => DOFOREACH_PASS1(CAT_PASS1((DOFOREACH_, 2)), (typename ARGTYPE, (ArgType1) argName1, (ArgType1) argName2 ) ))
//
#define CAT_PASS1(x) PRIMITIVE_CAT x
//
//          => DOFOREACH_PASS1(PRIMITIVE_CAT (DOFOREACH_, 2), (typename ARGTYPE, (ArgType1) argName1, (ArgType1) argName2 ) ))
//
#define PRIMITIVE_CAT(x, y) x ## y
//
//          => DOFOREACH_PASS1( DOFOREACH_2 ), (typename ARGTYPE, (ArgType1) argName1, (ArgType1) argName2 ) ))
//
#define DOFOREACH_PASS1(m, x) m x
//
//          => DOFOREACH_2 (typename ARGTYPE, (ArgType1) argName1, (ArgType1) argName2 ) )
//
#define DOFOREACH_0(m)
#define DOFOREACH_1(m, x1) m(x1)
#define DOFOREACH_2(m, x1, x2) m(x1), m(x2)
//
//          => typename ARGTYPE( (ArgType1) argName1 ), typename ARGTYPE( (ArgType1) argName2 ) )
//          => typename ArgType1, typename ArgType2
//
#define DOFOREACH_3(m, x1, x2, x3) m(x1), m(x2), m(x3)
#define DOFOREACH_4(m, x1, x2, x3, x4) m(x1), m(x2), m(x3), m(x4)
#define DOFOREACH_5(m, x1, x2, x3, x4, x5) m(x1), m(x2), m(x3), m(x4), m(x5)
#define DOFOREACH_6(m, x1, x2, x3, x4, x5, x6) m(x1), m(x2), m(x3), m(x4), m(x5), m(x6)
#define DOFOREACH_7(m, x1, x2, x3, x4, x5, x6, x7) m(x1), m(x2), m(x3), m(x4), m(x5), m(x6), m(x7)
#define DOFOREACH_8(m, x1, x2, x3, x4, x5, x6, x7, x8) m(x1), m(x2), m(x3), m(x4), m(x5), m(x6), m(x7), m(x8)
#define DOFOREACH_9(m, x1, x2, x3, x4, x5, x6, x7, x8, x9) m(x1), m(x2), m(x3), m(x4), m(x5), m(x6), m(x7), m(x8), m(x9)
#define DOFOREACH_10(m, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10) m(x1), m(x2), m(x3), m(x4), m(x5), m(x6), m(x7), m(x8), m(x9), m(x10)
#define DOFOREACH_11(m, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11) m(x1), m(x2), m(x3), m(x4), m(x5), m(x6), m(x7), m(x8), m(x9), m(x10), m(x11)
#define DOFOREACH_12(m, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12) m(x1), m(x2), m(x3), m(x4), m(x5), m(x6), m(x7), m(x8), m(x9), m(x10), m(x11), m(x12)
#define DOFOREACH_13(m, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13) m(x1), m(x2), m(x3), m(x4), m(x5), m(x6), m(x7), m(x8), m(x9), m(x10), m(x11), m(x12), m(x13)
#define DOFOREACH_14(m, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14) m(x1), m(x2), m(x3), m(x4), m(x5), m(x6), m(x7), m(x8), m(x9), m(x10), m(x11), m(x12), m(x13), m(x14)
#define DOFOREACH_15(m, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15) m(x1), m(x2), m(x3), m(x4), m(x5), m(x6), m(x7), m(x8), m(x9), m(x10), m(x11), m(x12), m(x13), m(x14), m(x15)


//
//  Same version as DOFOREACH, except this one meant to be used for appending list of arguments. If 0 arguments, then list is not appended, otherwise additional command is added in front.
//
#define DOFOREACH2(macro, ...) DOFOREACH_PASS1(CAT_TOKENS(DOFOREACH2_, NARGS(__VA_ARGS__)), (macro, __VA_ARGS__))

#define DOFOREACH2_0(m)
#define DOFOREACH2_1(m, x1) ,m(x1)
#define DOFOREACH2_2(m, x1, x2) ,m(x1), m(x2)
#define DOFOREACH2_3(m, x1, x2, x3) ,m(x1), m(x2), m(x3)
#define DOFOREACH2_4(m, x1, x2, x3, x4) ,m(x1), m(x2), m(x3), m(x4)
#define DOFOREACH2_5(m, x1, x2, x3, x4, x5) ,m(x1), m(x2), m(x3), m(x4), m(x5)
#define DOFOREACH2_6(m, x1, x2, x3, x4, x5, x6) ,m(x1), m(x2), m(x3), m(x4), m(x5), m(x6)
#define DOFOREACH2_7(m, x1, x2, x3, x4, x5, x6, x7) ,m(x1), m(x2), m(x3), m(x4), m(x5), m(x6), m(x7)
#define DOFOREACH2_8(m, x1, x2, x3, x4, x5, x6, x7, x8) ,m(x1), m(x2), m(x3), m(x4), m(x5), m(x6), m(x7), m(x8)
#define DOFOREACH2_9(m, x1, x2, x3, x4, x5, x6, x7, x8, x9) ,m(x1), m(x2), m(x3), m(x4), m(x5), m(x6), m(x7), m(x8), m(x9)
#define DOFOREACH2_10(m, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10) ,m(x1), m(x2), m(x3), m(x4), m(x5), m(x6), m(x7), m(x8), m(x9), m(x10)
#define DOFOREACH2_11(m, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11) ,m(x1), m(x2), m(x3), m(x4), m(x5), m(x6), m(x7), m(x8), m(x9), m(x10), m(x11)
#define DOFOREACH2_12(m, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12) ,m(x1), m(x2), m(x3), m(x4), m(x5), m(x6), m(x7), m(x8), m(x9), m(x10), m(x11), m(x12)
#define DOFOREACH2_13(m, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13) ,m(x1), m(x2), m(x3), m(x4), m(x5), m(x6), m(x7), m(x8), m(x9), m(x10), m(x11), m(x12), m(x13)
#define DOFOREACH2_14(m, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14) ,m(x1), m(x2), m(x3), m(x4), m(x5), m(x6), m(x7), m(x8), m(x9), m(x10), m(x11), m(x12), m(x13), m(x14)
#define DOFOREACH2_15(m, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15) ,m(x1), m(x2), m(x3), m(x4), m(x5), m(x6), m(x7), m(x8), m(x9), m(x10), m(x11), m(x12), m(x13), m(x14), m(x15)


//
//  Same version as DOFOREACH, uses ';' separation for fields. Can be used to reflect fields or function calls.
//
#define DOFOREACH_SEMICOLON(macro, ...) DOFOREACH_PASS1(CAT_TOKENS(DOFOREACH_SEMICOLON_, NARGS(__VA_ARGS__)), (macro, __VA_ARGS__))

#define DOFOREACH_SEMICOLON_0(m)
#define DOFOREACH_SEMICOLON_1(m, x1) m(x1);
#define DOFOREACH_SEMICOLON_2(m, x1, x2) m(x1); m(x2);
#define DOFOREACH_SEMICOLON_3(m, x1, x2, x3) m(x1); m(x2); m(x3);
#define DOFOREACH_SEMICOLON_4(m, x1, x2, x3, x4) m(x1); m(x2); m(x3); m(x4);
#define DOFOREACH_SEMICOLON_5(m, x1, x2, x3, x4, x5) m(x1); m(x2); m(x3); m(x4); m(x5);
#define DOFOREACH_SEMICOLON_6(m, x1, x2, x3, x4, x5, x6)  m(x1); m(x2); m(x3); m(x4); m(x5); m(x6);
#define DOFOREACH_SEMICOLON_7(m, x1, x2, x3, x4, x5, x6, x7) m(x1); m(x2); m(x3); m(x4); m(x5); m(x6); m(x7);
#define DOFOREACH_SEMICOLON_8(m, x1, x2, x3, x4, x5, x6, x7, x8) m(x1); m(x2); m(x3); m(x4); m(x5); m(x6); m(x7); m(x8);
#define DOFOREACH_SEMICOLON_9(m, x1, x2, x3, x4, x5, x6, x7, x8, x9) m(x1); m(x2); m(x3); m(x4); m(x5); m(x6); m(x7); m(x8); m(x9);
#define DOFOREACH_SEMICOLON_10(m, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10) m(x1); m(x2); m(x3); m(x4); m(x5); m(x6); m(x7); m(x8); m(x9); m(x10);
#define DOFOREACH_SEMICOLON_11(m, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11) m(x1); m(x2); m(x3); m(x4); m(x5); m(x6); m(x7); m(x8); m(x9); m(x10); m(x11);
#define DOFOREACH_SEMICOLON_12(m, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12) m(x1); m(x2); m(x3); m(x4); m(x5); m(x6); m(x7); m(x8); m(x9); m(x10); m(x11); m(x12);
#define DOFOREACH_SEMICOLON_13(m, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13) m(x1); m(x2); m(x3); m(x4); m(x5); m(x6); m(x7); m(x8); m(x9); m(x10); m(x11); m(x12); m(x13);
#define DOFOREACH_SEMICOLON_14(m, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14) m(x1); m(x2); m(x3); m(x4); m(x5); m(x6); m(x7); m(x8); m(x9); m(x10); m(x11); m(x12); m(x13); m(x14);
#define DOFOREACH_SEMICOLON_15(m, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15) m(x1); m(x2); m(x3); m(x4); m(x5); m(x6); m(x7); m(x8); m(x9); m(x10); m(x11); m(x12); m(x13); m(x14); m(x15);




//
//  ARGX(1)     =>      (Arg1) arg1
//
#define ARGX(index) (Arg##index) arg##index

//
//  Defines same function with different amount of arguments.
//
#define DEFINE_MULTIARG_FUNC(macro) \
    macro ( ARGX(1) ); \
    macro ( ARGX(1), ARGX(2) ); \
    macro ( ARGX(1), ARGX(2), ARGX(3) ); \
    macro ( ARGX(1), ARGX(2), ARGX(3), ARGX(4) ); \
    macro ( ARGX(1), ARGX(2), ARGX(3), ARGX(4), ARGX(5) ); \
    macro ( ARGX(1), ARGX(2), ARGX(3), ARGX(4), ARGX(5), ARGX(6) ); \
    macro ( ARGX(1), ARGX(2), ARGX(3), ARGX(4), ARGX(5), ARGX(6), ARGX(7) ); \
    macro ( ARGX(1), ARGX(2), ARGX(3), ARGX(4), ARGX(5), ARGX(6), ARGX(7), ARGX(8) ); \
    macro ( ARGX(1), ARGX(2), ARGX(3), ARGX(4), ARGX(5), ARGX(6), ARGX(7), ARGX(8), ARGX(9) ); \
    macro ( ARGX(1), ARGX(2), ARGX(3), ARGX(4), ARGX(5), ARGX(6), ARGX(7), ARGX(8), ARGX(9), ARGX(10) ); \
    macro ( ARGX(1), ARGX(2), ARGX(3), ARGX(4), ARGX(5), ARGX(6), ARGX(7), ARGX(8), ARGX(9), ARGX(10), ARGX(11) ); \
    macro ( ARGX(1), ARGX(2), ARGX(3), ARGX(4), ARGX(5), ARGX(6), ARGX(7), ARGX(8), ARGX(9), ARGX(10), ARGX(11), ARGX(12) ); \
    macro ( ARGX(1), ARGX(2), ARGX(3), ARGX(4), ARGX(5), ARGX(6), ARGX(7), ARGX(8), ARGX(9), ARGX(10), ARGX(11), ARGX(12), ARGX(13)  ); \
    macro ( ARGX(1), ARGX(2), ARGX(3), ARGX(4), ARGX(5), ARGX(6), ARGX(7), ARGX(8), ARGX(9), ARGX(10), ARGX(11), ARGX(12), ARGX(13), ARGX(14)  ); \
    macro ( ARGX(1), ARGX(2), ARGX(3), ARGX(4), ARGX(5), ARGX(6), ARGX(7), ARGX(8), ARGX(9), ARGX(10), ARGX(11), ARGX(12), ARGX(13), ARGX(14), ARGX(15)  ); \


//
//  Same as previous, except add support also of function with no arguments.
//  (template functions normally requires at least one template parameter (so you write template<> in front of function and won't get error), that's why separate define)
//
#define DEFINE_MULTIARG_FUNC2(macro) \
    macro ( ); \
    macro ( ARGX(1) ); \
    macro ( ARGX(1), ARGX(2) ); \
    macro ( ARGX(1), ARGX(2), ARGX(3) ); \
    macro ( ARGX(1), ARGX(2), ARGX(3), ARGX(4) ); \
    macro ( ARGX(1), ARGX(2), ARGX(3), ARGX(4), ARGX(5) ); \
    macro ( ARGX(1), ARGX(2), ARGX(3), ARGX(4), ARGX(5), ARGX(6) ); \
    macro ( ARGX(1), ARGX(2), ARGX(3), ARGX(4), ARGX(5), ARGX(6), ARGX(7) ); \
    macro ( ARGX(1), ARGX(2), ARGX(3), ARGX(4), ARGX(5), ARGX(6), ARGX(7), ARGX(8) ); \
    macro ( ARGX(1), ARGX(2), ARGX(3), ARGX(4), ARGX(5), ARGX(6), ARGX(7), ARGX(8), ARGX(9) ); \
    macro ( ARGX(1), ARGX(2), ARGX(3), ARGX(4), ARGX(5), ARGX(6), ARGX(7), ARGX(8), ARGX(9), ARGX(10) ); \
    macro ( ARGX(1), ARGX(2), ARGX(3), ARGX(4), ARGX(5), ARGX(6), ARGX(7), ARGX(8), ARGX(9), ARGX(10), ARGX(11) ); \
    macro ( ARGX(1), ARGX(2), ARGX(3), ARGX(4), ARGX(5), ARGX(6), ARGX(7), ARGX(8), ARGX(9), ARGX(10), ARGX(11), ARGX(12) ); \
    macro ( ARGX(1), ARGX(2), ARGX(3), ARGX(4), ARGX(5), ARGX(6), ARGX(7), ARGX(8), ARGX(9), ARGX(10), ARGX(11), ARGX(12), ARGX(13)  ); \
    macro ( ARGX(1), ARGX(2), ARGX(3), ARGX(4), ARGX(5), ARGX(6), ARGX(7), ARGX(8), ARGX(9), ARGX(10), ARGX(11), ARGX(12), ARGX(13), ARGX(14)  ); \
    macro ( ARGX(1), ARGX(2), ARGX(3), ARGX(4), ARGX(5), ARGX(6), ARGX(7), ARGX(8), ARGX(9), ARGX(10), ARGX(11), ARGX(12), ARGX(13), ARGX(14), ARGX(15)  ); \

/*
    Here is simple example of usage of these macros:

    #define MKFUNC_make_unique(...) \
    template <class T  DOFOREACH2(typename ARGTYPE, __VA_ARGS__) > \
    std::unique_ptr<T> make_unique(  DOFOREACH(REFARGPAIR, __VA_ARGS__) ) \
    { \
        return std::unique_ptr<T>( new T( DOFOREACH(ARGNAME, __VA_ARGS__) ) ); \
    }

    DEFINE_MULTIARG_FUNC2(MKFUNC_make_unique);      //<--
    #undef MKFUNC_make_unique

    Debugger will stall in "<--" this line, so it makes sense to keep amount of line absolute minimal.
*/

