#ifndef FZ_PREPROCESSOR_CAT_HPP
#define FZ_PREPROCESSOR_CAT_HPP

#define FZ_PP_CAT(a, b) FZ_INTERNAL_PP_CAT(a, b)

#define FZ_INTERNAL_PP_CAT(a, b) a ## b

#endif // FZ_PREPROCESSOR_CAT_HPP