/*******************************************************************************
 * This file is part of the "https://github.com/blackmatov/meta.hpp"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2021-2023, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#pragma once

#include "meta_base.hpp"

namespace meta_hpp::detail
{
    template <
        typename To,
        typename From,
        typename ToDT = std::remove_pointer_t<To>,
        typename FromDT = std::remove_pointer_t<From> >
    concept pointer_ucast_kind                                                      //
        = (std::is_pointer_v<From> && std::is_class_v<FromDT>)                      //
       &&(std::is_pointer_v<To> && (std::is_class_v<ToDT> || std::is_void_v<ToDT>)) //
       && (!std::is_const_v<FromDT> || std::is_const_v<ToDT>)                       //
       &&(!std::is_volatile_v<FromDT> || std::is_volatile_v<ToDT>);                 //

    template <
        typename To,
        typename From,
        typename ToDT = std::remove_reference_t<To>,
        typename FromDT = std::remove_reference_t<From> >
    concept lvalue_reference_ucast_kind                                 //
        = (std::is_lvalue_reference_v<From> && std::is_class_v<FromDT>) //
        &&(std::is_lvalue_reference_v<To> && std::is_class_v<ToDT>)     //
        &&(!std::is_const_v<FromDT> || std::is_const_v<ToDT>)           //
        &&(!std::is_volatile_v<FromDT> || std::is_volatile_v<ToDT>);    //
}

namespace meta_hpp
{
    template < typename To, typename From >
        requires detail::pointer_ucast_kind<To, From>
    To ucast(From from);

    template < typename To, typename From >
        requires detail::lvalue_reference_ucast_kind<To, From>
    To ucast(From&& from);
}
