// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/new_data/rewriter.h
/// \brief The class rewriter.

#ifndef MCRL2_DATA_REWRITER_H
#define MCRL2_DATA_REWRITER_H

#include <functional>
#include <algorithm>
#include <sstream>
#include <boost/shared_ptr.hpp>
#include "mcrl2/new_data/expression_traits.h"
#include "mcrl2/new_data/detail/data_expression_with_variables.h"
#include "mcrl2/new_data/detail/rewrite.h"
#include "mcrl2/new_data/detail/implement_data_types.h"
#include "mcrl2/new_data/detail/data_reconstruct.h"
#include "mcrl2/new_data/data_equation.h"
#include "mcrl2/new_data/parser.h"
#include "mcrl2/new_data/replace.h"
#include "mcrl2/new_data/find.h"

namespace mcrl2 {

namespace new_data {

  /// \brief Rewriter class for the mCRL2 Library. It only works for terms of type data_expression
  /// and data_expression_with_variables.
  template < typename Expression >
  class basic_rewriter
  {
    template < typename CompatibleExpression >
    friend class basic_rewriter;
    friend class enumerator;

    protected:
      /// \brief for data implementation/reconstruction
      mutable atermpp::aterm_appl         m_specification;

      /// \brief for data implementation
      mutable atermpp::aterm_list         m_substitution_context;

      /// \brief The wrapped Rewriter.
      boost::shared_ptr<detail::Rewriter> m_rewriter;

    protected:

      /// \brief Copy constructor for conversion between derived types
      template < typename CompatibleExpression >
      basic_rewriter(basic_rewriter< CompatibleExpression > const& other) :
                       m_specification(other.m_specification),
                       m_substitution_context(other.m_substitution_context),
                       m_rewriter(other.m_rewriter)
      {
      }

      /// \brief Performs data implementation before rewriting
      ATermAppl implement(data_expression expression) const
      {
        ATermList substitution_context = m_substitution_context;
        ATermList new_data_equations   = ATmakeList0();

        core::detail::t_data_decls declarations = core::detail::get_data_decls(m_specification);

        ATermAppl implemented = detail::impl_exprs_appl(expression,
                        &substitution_context, &declarations, &new_data_equations);

        if (!ATisEmpty(new_data_equations)) {
          atermpp::term_list< new_data::data_equation > new_equations(new_data_equations);

          for (atermpp::term_list< new_data::data_equation >::const_iterator i = new_equations.begin();
                                                                        i != new_equations.end(); ++i) {
            m_rewriter->addRewriteRule(*i);
          }

          m_specification = core::detail::add_data_decls(m_specification, declarations);
        }

        m_substitution_context = substitution_context;

        return implemented;
      }

      /// \brief Performs data reconstruction after rewriting
      data_expression reconstruct(ATermAppl expression) const
      {
        ATermAppl reconstructed(reinterpret_cast< ATermAppl >(
           detail::reconstruct_exprs(reinterpret_cast< ATerm >(
           static_cast< ATermAppl >(expression)), m_specification)));

        return atermpp::aterm_appl(reconstructed);
      }

    public:
      /// \brief The variable type of the rewriter.
      typedef typename core::term_traits< Expression >::variable_type variable_type;

      /// \brief The term type of the rewriter.
      typedef Expression term_type;

      /// \brief The strategy of the rewriter.
      enum strategy
      {
        innermost                  = detail::GS_REWR_INNER   ,  /** \brief Innermost */
#ifdef MCRL2_INNERC_AVAILABLE
        innermost_compiling        = detail::GS_REWR_INNERC  ,  /** \brief Compiling innermost */
#endif
        jitty                      = detail::GS_REWR_JITTY   ,  /** \brief JITty */
#ifdef MCRL2_JITTYC_AVAILABLE
        jitty_compiling            = detail::GS_REWR_JITTYC  ,  /** \brief Compiling JITty */
#endif
        innermost_prover           = detail::GS_REWR_INNER_P ,  /** \brief Innermost + Prover */
#ifdef MCRL2_INNERC_AVAILABLE
        innermost_compiling_prover = detail::GS_REWR_INNERC_P,  /** \brief Compiling innermost + Prover*/
#endif
#ifdef MCRL2_JITTYC_AVAILABLE
        jitty_prover               = detail::GS_REWR_JITTY_P ,  /** \brief JITty + Prover */
        jitty_compiling_prover     = detail::GS_REWR_JITTYC_P   /** \brief Compiling JITty + Prover*/
#else
        jitty_prover               = detail::GS_REWR_JITTY_P    /** \brief JITty + Prover */
#endif
      };

      /// \brief Constructor.
      /// \param r A rewriter
      basic_rewriter(basic_rewriter const& r)
        : m_specification(r.m_specification),
          m_substitution_context(r.m_substitution_context),
          m_rewriter(r.m_rewriter)
      {
        m_substitution_context.protect();
        m_specification.protect();
      }

      /// \brief Constructor.
      /// \param d A data specification
      /// \param s A rewriter strategy.
      basic_rewriter(data_specification d = data_specification(), strategy s = jitty)
        : m_substitution_context(ATmakeList0())
      {
        ATermList substitution_context = m_substitution_context;
        m_specification = detail::implement_data_specification(d, &substitution_context);
        m_rewriter.reset(detail::createRewriter(m_specification, static_cast< detail::RewriteStrategy >(s)));
        m_substitution_context = substitution_context;
        m_substitution_context.protect();
        m_specification.protect();
      }

      /// \brief Adds an equation to the rewrite rules.
      /// \param eq The equation that is added.
      /// \return Returns true if the operation succeeded.
      bool add_rule(const data_equation& eq)
      {
        return m_rewriter->addRewriteRule(eq);
      }

      /// \brief Removes an equation from the rewrite rules.
      /// \param eq The equation that is removed.
      void remove_rule(const data_equation& eq)
      {
        m_rewriter->removeRewriteRule(eq);
      }

      /// \brief Returns a pointer to the Rewriter object that is used for the implementation.
      /// \return A pointer to the wrapped Rewriter object.
      /// \deprecated
      detail::Rewriter* get_rewriter()
      {
        return m_rewriter.get();
      }

      ~basic_rewriter() {
        m_specification.unprotect();
      }
  };

  /// \brief Rewriter that operates on data expressions.
  class rewriter: public basic_rewriter<data_expression>
  {
    public:
      /// \brief Constructor.
      /// \param d A data specification
      /// \param s A rewriter strategy.
      rewriter(data_specification const& d = data_specification(), strategy s = jitty)
        : basic_rewriter<data_expression>(d, s)
      { }

      /// \brief Constructor.
      /// \param d A data specification
      /// \param s A rewriter strategy.
      rewriter(rewriter const& r)
        : basic_rewriter<data_expression>(static_cast< basic_rewriter< data_expression > const& >(r))
      { }

      /// \brief Rewrites a data expression.
      /// \param d A data expression
      /// \return The normal form of d.
      data_expression operator()(const data_expression& d) const
      {
        return reconstruct(m_rewriter->rewrite(implement(d)));
      }

      /// \brief Rewrites the data expression d, and on the fly applies a substitution function
      /// to data variables.
      /// \param d A data expression
      /// \param sigma A substitution function
      /// \return The normal form of the term.
      template <typename SubstitutionFunction>
      data_expression operator()(const data_expression& d, SubstitutionFunction sigma) const
      {
        return reconstruct(m_rewriter->rewrite(implement(replace_variables(d, sigma))));
      }
  };

  /// \brief Rewriter that operates on data expressions.
  class rewriter_with_variables: public basic_rewriter<data_expression_with_variables>
  {
    public:

      /// \brief Constructor.
      /// \param d A data specification
      /// \param s A rewriter strategy.
      rewriter_with_variables(data_specification d = data_specification(), strategy s = jitty)
        : basic_rewriter<data_expression_with_variables>(d, s)
      { }

      /// \brief Constructor. The Rewriter object that is used internally will be shared with \p r.
      /// \param r A data rewriter
      rewriter_with_variables(basic_rewriter< data_expression_with_variables > const& r)
        : basic_rewriter<data_expression_with_variables>(r)
      {}

      /// \brief Constructor. The Rewriter object that is used internally will be shared with \p r.
      /// \param r A data rewriter
      rewriter_with_variables(basic_rewriter< data_expression > const& r)
        : basic_rewriter<data_expression_with_variables>(r)
      {}

      /// \brief Rewrites a data expression.
      /// \param d The term to be rewritten.
      /// \return The normal form of d.
      data_expression_with_variables operator()(const data_expression_with_variables& d) const
      {
        data_expression t = reconstruct(m_rewriter.get()->rewrite(implement(d)));
        std::set<variable> v = find_all_variables(t);
        return data_expression_with_variables(t, variable_list(v.begin(), v.end()));
      }

      /// \brief Rewrites the data expression d, and on the fly applies a substitution function
      /// to data variables.
      /// \param d A term.
      /// \param sigma A substitution function
      /// \return The normal form of the term.
      template <typename SubstitutionFunction>
      data_expression_with_variables operator()(const data_expression_with_variables& d, SubstitutionFunction sigma) const
      {
        data_expression t = this->operator()(replace_variables(d, sigma));
        std::set<variable> v = find_all_variables(t);
        data_expression_with_variables result(t, variable_list(v.begin(), v.end()));
        return result;
      }
  };

  /// \brief Function object that turns a map of substitutions to variables into a substitution function.
  template <typename SubstitutionMap>
  class rewriter_map: public SubstitutionMap
  {
    public:
      /// \brief The mapped type.
      typedef typename SubstitutionMap::mapped_type term_type;

      /// \brief The key type.
      typedef typename SubstitutionMap::key_type variable_type;

      /// \brief Constructor.
      rewriter_map()
      {}

      /// \brief Constructor.
      /// \param m A rewriter map.
      rewriter_map(const rewriter_map<SubstitutionMap>& m)
        : SubstitutionMap(m)
      {}

      /// \brief Constructor.
      /// \param start The start of a range of substitutions.
      /// \param end The end of a range of substitutions.
      template <typename Iter>
      rewriter_map(Iter start, Iter end)
        : SubstitutionMap(start, end)
      {}

      /// \brief Function application.
      /// \param v A variable
      /// \return The corresponding value.
      term_type operator()(const variable_type& v) const
      {
        typename SubstitutionMap::const_iterator i = this->find(v);
        return i == this->end() ? core::term_traits<term_type>::variable2term(v) : i->second;
      }

      /// \brief Returns a string representation of the map, for example [a := 3, b := true].
      /// \return A string representation of the map.
      std::string to_string() const
      {
        std::stringstream result;
        result << "[";
        for (typename SubstitutionMap::const_iterator i = this->begin(); i != this->end(); ++i)
        {
          result << (i == this->begin() ? "" : "; ") << core::pp(i->first) << ":" << core::pp(i->first.sort()) << " := " << core::pp(i->second);
        }
        result << "]";
        return result.str();
      }
  };

} // namespace data

} // namespace mcrl2

#endif // MCRL2_DATA_REWRITER_H
