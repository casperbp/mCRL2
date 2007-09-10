// Author(s): Wieger Wesselink
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/data/utility.h
/// \brief Add your file description here.

#ifndef MCRL2_DATA_UTILITY_H
#define MCRL2_DATA_UTILITY_H

#include <algorithm>
#include <functional>
#include <iterator>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "boost/format.hpp"
#include "mcrl2/data/data.h"
#include "mcrl2/lps/detail/data_utility.h"
#include "atermpp/algorithm.h"
#include "atermpp/aterm.h"
#include "atermpp/set.h"
#include "atermpp/utility.h"

namespace lps {

using atermpp::aterm;
using atermpp::aterm_traits;

/// Returns true if the expressions x and y have the same sort.
struct equal_data_expression_sort
{
  bool operator()(const data_expression& x, const data_expression& y) const
  {
    return x.sort() == y.sort();
  }
};

/// \brief Function object that returns the name of a data variable
struct variable_name: public std::unary_function<data_variable, identifier_string>
{
  identifier_string operator()(const data_variable& v) const
  {
    return v.name();
  }
};

/// \brief Returns the set of all identifier strings occurring in the term t
template <typename Term>
std::set<identifier_string> identifiers(Term t)
{
  std::set<identifier_string> result;
  find_all_if(aterm_traits<Term>::term(t), is_identifier_string, std::inserter(result, result.end()));
  return result;
}

/// Returns a copy of t, but with a common postfix added to each variable name,
/// and such that the new names do not appear in context.
inline
data_variable_list fresh_variables(data_variable_list t, const std::set<std::string>& context, std::string postfix_format = "_%02d")
{
  std::vector<std::string> ids = detail::variable_strings(t);
  std::string postfix;
  for (int i = 0; ; i++)
  {
    postfix = str(boost::format(postfix_format) % i);
    std::vector<std::string>::iterator j = ids.begin();
    for ( ; j != ids.end(); j++)
    {
      if (context.find(*j + postfix) != context.end())
        break;
    }
    if (j == ids.end()) // success!
      break;
  }
  data_variable_list result;
  for (data_variable_list::iterator k = t.begin(); k != t.end(); ++k)
  {
    identifier_string name(std::string(k->name()) + postfix);
    result = push_front(result, data_variable(gsMakeDataVarId(name, k->sort())));
  }
  return atermpp::reverse(result);
}

/// Creates an identifier built from name and index.
struct default_identifier_creator
{
  std::string operator()(const std::string& name, int index) const
  {
    if (index <= 0)
      return name;
    return str(boost::format(name + "%02d") % index++);
  }
};

/// Creates an identifier built from name and index.
struct postfix_identifier_creator
{
  std::string postfix_;
  
  postfix_identifier_creator(const std::string& postfix)
    : postfix_(postfix)
  { }   
  
  std::string operator()(const std::string& name, int index) const
  {
    if (index <= 0)
      return name + postfix_;
    return str(boost::format(name + "%02d" + postfix_) % index++);
  }
};

/// \brief Returns an identifier that doesn't appear in the term context
template <typename IdentifierCreator>
identifier_string fresh_identifier(const std::set<identifier_string>& context, const std::string& hint, IdentifierCreator id_creator = IdentifierCreator())
{
  int index = 0;
  identifier_string s;
  do
  {
    s = identifier_string(id_creator(hint, index++));
  }
  while(context.find(s) != context.end());
  return s;
}

/// \brief Returns an identifier that doesn't appear in the term context
template <typename Term, class IdentifierCreator>
identifier_string fresh_identifier(Term context, const std::string& hint, IdentifierCreator id_creator = IdentifierCreator())
{
  return fresh_identifier(identifiers(context), hint, id_creator);
}

/// \brief Returns an identifier that doesn't appear in the term context
template <typename Term>
identifier_string fresh_identifier(const Term& context, const std::string& hint)
{
  return fresh_identifier(context, hint, default_identifier_creator());
}

/// \brief Returns a variable that doesn't appear in context
template <typename Term>
data_variable fresh_variable(Term context, lps::sort s, std::string hint)
{
  identifier_string id = fresh_identifier(context, hint);
  return data_variable(gsMakeDataVarId(id, s));
}

/// \brief Returns all data variables that occur in the term t
template <typename Term>
std::set<data_variable> find_variables(Term t)
{
  // find all data variables in t
  std::set<data_variable> variables;
  atermpp::find_all_if(t, is_data_variable, std::inserter(variables, variables.end()));
  return variables;
}

/// \brief Returns all names data variables that occur in the term t
template <typename Term>
std::set<std::string> find_variable_names(Term t)
{
  // find all data variables in t
  std::set<data_variable> variables;
  atermpp::find_all_if(t, is_data_variable, std::inserter(variables, variables.end()));

  std::set<std::string> result;
  for (std::set<data_variable>::iterator j = variables.begin(); j != variables.end(); ++j)
  {
    result.insert(j->name());
  }
  return result;
}

/// Generates identifiers that do not appear in the given context.
class fresh_identifier_generator
{
  protected:
    atermpp::set<identifier_string> m_identifiers;
    std::string m_hint; // used as a hint for operator()()

  public:
    fresh_identifier_generator()
     : m_hint("t")
    { }

    template <typename Term>
    fresh_identifier_generator(Term context, std::string hint = "t")
    {
      m_identifiers = identifiers(context);
      m_hint = hint;
    }

    /// Set a new hint.
    void set_hint(std::string hint)
    {
      m_hint = hint;
    }

    /// Returns the current hint.
    std::string hint() const
    {
      return m_hint;
    }

    /// Set a new context.
    template <typename Term>
    void set_context(Term context)
    {
      m_identifiers = identifiers(context);
    }

    /// Add term t to the context.
    template <typename Term>
    void add_to_context(Term t)
    {
      std::set<identifier_string> ids = identifiers(t);
      std::copy(ids.begin(), ids.end(), std::inserter(m_identifiers, m_identifiers.end()));
    }

    /// Returns a unique identifier, with the stored hint as prefix.
    /// The returned identifier is added to the context.
    identifier_string operator()()
    {
      identifier_string id(m_hint);
      int index = 0;
      while (m_identifiers.find(id) != m_identifiers.end())
      {   
        std::string name = str(boost::format(m_hint + "%02d") % index++);
        id = identifier_string(name);
      }
      m_identifiers.insert(id);
      return id;
    }

    /// Returns a unique identifier, with the given hint as prefix.
    /// The returned identifier is added to the context.
    identifier_string operator()(const std::string& hint)
    {
      identifier_string id(hint);
      int index = 0;
      while (m_identifiers.find(id) != m_identifiers.end())
      {   
        std::string name = str(boost::format(hint + "%02d") % index++);
        id = identifier_string(name);
      }
      m_identifiers.insert(id);
      return id;
    }
};

/// Fresh variable generator that generates data variables with
/// names that do not appear in the given context.
class fresh_variable_generator
{
  protected:
    atermpp::set<identifier_string> m_identifiers;
    lps::sort m_sort;                    // used for operator()()
    std::string m_hint;                  // used as a hint for operator()()

  public:
    fresh_variable_generator()
     : m_sort(sort_expr::real()), m_hint("t")
    { }

    template <typename Term>
    fresh_variable_generator(Term context, lps::sort s = sort_expr::real(), std::string hint = "t")
    {
      m_identifiers = identifiers(context);
      m_hint = hint;
      m_sort = s;
    }

    /// Set a new hint.
    ///
    void set_hint(std::string hint)
    {
      m_hint = hint;
    }

    /// Returns the current hint.
    ///
    std::string hint() const
    {
      return m_hint;
    }

    /// Set a new context.
    ///
    template <typename Term>
    void set_context(Term context)
    {
      m_identifiers = identifiers(context);
    }

    /// Set a new sort.
    ///
    void set_sort(lps::sort s)
    {
      m_sort = s;
    }

    /// Returns the current sort.
    ///
    lps::sort sort() const
    {
      return m_sort;
    }

    /// Add term t to the context.
    ///
    template <typename Term>
    void add_to_context(Term t)
    {
      std::set<identifier_string> ids = identifiers(t);
      std::copy(ids.begin(), ids.end(), std::inserter(m_identifiers, m_identifiers.end()));
    }

    /// Returns a unique variable of the given sort, with the given hint as prefix.
    /// The returned variable is added to the context.
    ///
    data_variable operator()()
    {
      identifier_string id(m_hint);
      int index = 0;
      while (m_identifiers.find(id) != m_identifiers.end())
      {   
        std::string name = str(boost::format(m_hint + "%02d") % index++);
        id = identifier_string(name);
      }
      m_identifiers.insert(id);
      return data_variable(gsMakeDataVarId(id, m_sort));
    }

    /// Returns a unique variable with the same sort as the variable v, and with
    /// the same prefix. The returned variable is added to the context.
    ///
    data_variable operator()(data_variable v)
    {
      std::string hint = v.name();
      identifier_string id(hint);
      int index = 0;
      while (m_identifiers.find(id) != m_identifiers.end())
      {   
        std::string name = str(boost::format(hint + "%02d") % index++);
        id = identifier_string(name);
      }
      m_identifiers.insert(id);
      return data_variable(gsMakeDataVarId(id, v.sort()));
    }
};

} // namespace lps

#endif // MCRL2_DATA_UTILITY_H
