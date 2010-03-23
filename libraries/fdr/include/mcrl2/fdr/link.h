// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/process/link.h
/// \brief add your file description here.

#ifndef MCRL2_FDR_LINK_H
#define MCRL2_FDR_LINK_H

#include "mcrl2/atermpp/aterm_access.h"
#include "mcrl2/atermpp/aterm_appl.h"
#include "mcrl2/fdr/detail/term_functions.h"
#include "mcrl2/fdr/detail/constructors.h"

namespace mcrl2 {

namespace fdr {

  /// \brief Link
  class link: public atermpp::aterm_appl
  {
    public:
      /// \brief Constructor.
      link()
        : atermpp::aterm_appl(fdr::detail::constructLink())
      {}

      /// \brief Constructor.
      /// \param term A term
      link(atermpp::aterm_appl term)
        : atermpp::aterm_appl(term)
      {
        assert(fdr::detail::check_rule_Link(m_term));
      }
  };

//--- start generated classes ---//
/// \brief A link
class link: public link
{
  public:
    /// \brief Default constructor.
    link()
      : link(fdr::detail::constructLink())
    {}

    /// \brief Constructor.
    /// \param term A term
    link(atermpp::aterm_appl term)
      : link(term)
    {
      assert(fdr::detail::check_term_Link(m_term));
    }

    /// \brief Constructor.
    link(const dotted_expression& left, const dotted_expression& right)
      : link(fdr::detail::gsMakeLink(left, right))
    {}

    dotted_expression left() const
    {
      return atermpp::arg1(*this);
    }

    dotted_expression right() const
    {
      return atermpp::arg2(*this);
    }
};
//--- end generated classes ---//

//--- start generated is-functions ---//

    /// \brief Test for a link expression
    /// \param t A term
    /// \return True if it is a link expression
    inline
    bool is_link(const link& t)
    {
      return fdr::detail::gsIsLink(t);
    }
//--- end generated is-functions ---//

} // namespace fdr

} // namespace mcrl2

#endif // MCRL2_FDR_LINK_H
