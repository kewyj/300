/*******************************************************************************
 * This file is part of the "https://github.com/blackmatov/meta.hpp"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2021-2023, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#pragma once

#include "base.hpp"

namespace meta_hpp::detail
{
    template < typename From >
    struct cvref_traits {
        static constexpr bool is_lvalue = std::is_lvalue_reference_v<From>;
        static constexpr bool is_rvalue = std::is_rvalue_reference_v<From>;
        static constexpr bool is_const = std::is_const_v<std::remove_reference_t<From>>;
        static constexpr bool is_volatile = std::is_volatile_v<std::remove_reference_t<From>>;

        template < bool yesno, template < typename > typename Q, typename V >
        using apply_t_if = std::conditional_t<yesno, Q<V>, V>;

        // clang-format off
        template < typename To >
        using add_to =
            apply_t_if<is_lvalue, std::add_lvalue_reference_t,
            apply_t_if<is_rvalue, std::add_rvalue_reference_t,
            apply_t_if<is_const, std::add_const_t,
            apply_t_if<is_volatile, std::add_volatile_t,
            To>>>>;
        // clang-format on

        template < typename To >
        using copy_to = add_to<std::remove_cvref_t<To>>;
    };

    template < typename From, typename To >
    struct add_cvref {
        using type = typename cvref_traits<From>::template add_to<To>;
    };

    template < typename From, typename To >
    struct copy_cvref {
        using type = typename cvref_traits<From>::template copy_to<To>;
    };

    template < typename From, typename To >
    using add_cvref_t = typename add_cvref<From, To>::type;

    template < typename From, typename To >
    using copy_cvref_t = typename copy_cvref<From, To>::type;
}
