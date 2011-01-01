// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/bes/builder.h
/// \brief add your file description here.

#ifndef MCRL2_BES_BUILDER_H
#define MCRL2_BES_BUILDER_H

#include "mcrl2/bes/boolean_expression.h"
#include "mcrl2/core/builder.h"

namespace mcrl2 {

namespace bes {

  /// \brief Traversal class for boolean expressions
  template <typename Derived>
  class boolean_expression_builder: public core::builder<Derived>
  {
    public:
      typedef core::builder<Derived> super;
      using super::enter;
      using super::leave;
      using super::operator();

#include "mcrl2/boolean/detail/boolean_expression_builder.inc.h"
  };

} // namespace bes

} // namespace mcrl2

#endif // MCRL2_BES_BUILDER_H
