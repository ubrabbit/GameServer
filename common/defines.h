#pragma once

#ifndef NDEBUG

#define assert_at_compile_time(condition, st) \
{\
    typedef char st[(condition) ? 1 : -1];\
    st a;\
    memset(a, 0, sizeof(st));\
}

#define ASSERT_STRUCT_SIZE_EQUAL_MACRO(st, Macro)\
{\
    assert_at_compile_time(sizeof(st) == Macro, Struct##_size_check_##Macro);\
}

#define ASSERT_VALUE_EQUAL_MACRO(Value, Macro)\
{\
    assert_at_compile_time(Value, Struct##_value_check_##Macro);\
}

#else // #ifndef NDEBUG

#define ASSERT_STRUCT_SIZE_EQUAL_MACRO(Macro1, Macro2);

#define ASSERT_VALUE_EQUAL_MACRO(Value, Macro);

#endif // #ifndef NDEBUG
