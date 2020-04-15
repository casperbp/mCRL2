// Author(s): Maurice Laveaux
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "cleave.h"

#include "lpscleave_utility.h"
#include "split_condition.h"

#include <optional>
#include <vector>

using namespace mcrl2;

/// \brief Construct the summand for the given parameters.
static
std::optional<lps::stochastic_action_summand> make_summand(
  const mcrl2::lps::stochastic_action_summand& summand,
  std::set<mcrl2::data::variable>& synchronized,
  const mcrl2::data::variable_list& right_parameters,
  const mcrl2::data::variable_list& left_parameters, 
  mcrl2::process::action_list& internal_action,
  mcrl2::process::action& left_tag, 
  mcrl2::process::action_label& left_sync,
  const cleave_condition& left_condition,
  mcrl2::data::data_expression_list& left_values,
  bool generate_left, 
  bool is_independent)
{
  // Add a summation for every parameter of the other process, and for every summation variable, that we depend on.
  data::variable_list left_variables = summand.summation_variables();
  for (const data::variable& variable : right_parameters)
  {
    if (synchronized.count(variable) > 0 && std::find(left_parameters.begin(), left_parameters.end(), variable) == left_parameters.end())
    {
      left_variables.push_front(variable);
    }
  }

  lps::multi_action left_action;
  if (generate_left)
  {
    // The resulting multi-action (either alpha or internal).
    process::action_list actions = (summand.multi_action() == lps::multi_action()) ? internal_action : summand.multi_action().actions();

    left_action.actions() = actions;
  }

  if (is_independent)
  {
    left_action.actions().push_front(process::action(left_tag));
  }
  else
  {
    // Need to synchronize.
    left_action.actions().push_front(process::action(left_sync, left_values));
  }

  if (generate_left || !is_independent)
  {
    // Only update our parameters.
    data::assignment_list left_assignments = project(summand.assignments(), left_parameters);
    return lps::stochastic_action_summand(left_variables, left_condition.expression, left_action, left_assignments, summand.distribution());
  }

  return {};
}

std::pair<lps::stochastic_specification, lps::stochastic_specification> mcrl2::cleave(
  const lps::stochastic_specification& spec,
  const data::variable_list& left_parameters,
  const data::variable_list& right_parameters,
  std::list<std::size_t>& indices
  )
{
  // Check sanity conditions, no timed or stochastic processes.
  const lps::stochastic_linear_process& process = spec.process();

  if (process.has_time())
  {
    throw runtime_error("Cleave does not support timed processes");
  }

  // Add the tags for the left and right processes
  process::action_label tagright_label("tagright", {});
  process::action_label tagleft_label("tagleft", {});
  process::action_label internal_label("internal", {});

  // Change the summands to include the parameters of the other process and added the sync action.

  // Extend the action specification with an actsync (that is unique) for every summand with the correct sorts, a tag and an intern action.
  std::vector<process::action_label> labels;
  labels.push_back(tagright_label);
  labels.push_back(tagleft_label);
  labels.push_back(internal_label);

  // Create actions to be used within the summands.
  process::action tagleft(tagleft_label, {});
  process::action tagright(tagright_label, {});
  process::action taginternal(internal_label, {});

  process::action_list internal_action;
  internal_action.push_front(taginternal);

  // The summands of the two specifications.
  lps::stochastic_action_summand_vector left_summands;
  lps::stochastic_action_summand_vector right_summands;

  // Add the summands that generate the action label.
  for (std::size_t index = 0; index < process.action_summands().size(); ++index)
  {
    // The original summand.
    const auto& summand = process.action_summands()[index];

    // Convert tau actions to a visible action.
    process::action_list internal;
    internal.push_front(taginternal);

    // The dependencies for the multi-action.
    std::set<data::variable> action_dependencies;
    for (const process::action& action : summand.multi_action().actions())
    {
      auto dependencies = data::find_free_variables(action.arguments());
      action_dependencies.insert(dependencies.begin(), dependencies.end());
    }

    // Indicates that the multi-action is (for now completely) generated in the left component.
    bool generate_left = (std::find(indices.begin(), indices.end(), index) != indices.end());

    // Remove dependencies on our own parameters (depending on where the action is generated).
    if (generate_left)
    {
      for (const data::variable& var : left_parameters)
      {
        action_dependencies.erase(var);
      }
    }
    else
    {
      for (const data::variable& var : right_parameters)
      {
        action_dependencies.erase(var);
      }
    }

    // Remove the summation variables from the action dependencies.
    for (const data::variable& var : summand.summation_variables())
    {
      action_dependencies.erase(var);
    }

    // The dependencies for the update expressions (on other parameters).
    std::set<data::variable> left_update_dependencies;
    std::set<data::variable> right_update_dependencies;

    for (const data::assignment& assignment : summand.assignments())
    {
      auto dependencies = data::find_free_variables(assignment.rhs());
      if (std::find(left_parameters.begin(), left_parameters.end(), assignment.lhs()) != left_parameters.end())
      {
        // This is an assignment for the left component's parameters.
        left_update_dependencies.insert(dependencies.begin(), dependencies.end());
      }

      if (std::find(right_parameters.begin(), right_parameters.end(), assignment.lhs()) != right_parameters.end())
      {
        // This is an assignment for the right component's parameters.
        right_update_dependencies.insert(dependencies.begin(), dependencies.end());
      }
    }

    // The variables on which the condition depends (which is just duplicated for now which always satisfies condition 1).
    auto [left_condition, right_condition] = split_condition(summand.condition(), left_parameters, right_parameters, summand.summation_variables());

    mCRL2log(log::verbose) << "Split conditions into " << left_condition.expression << ", and " << right_condition.expression << "\n";

    std::set<data::variable> left_condition_dependencies = data::find_free_variables(left_condition.expression);
    std::set<data::variable> right_condition_dependencies = data::find_free_variables(right_condition.expression);

    // Add the dependencies of the clauses as well.
    for (const data::data_expression& clause : left_condition.implicit)
    {
      std::set<data::variable> dependencies = data::find_free_variables(clause);
      left_condition_dependencies.insert(dependencies.begin(), dependencies.end());
    }

    for (const data::data_expression& clause : right_condition.implicit)
    {
      std::set<data::variable> dependencies = data::find_free_variables(clause);
      right_condition_dependencies.insert(dependencies.begin(), dependencies.end());
    }

    // Remove dependencies on our own parameter, because these are not required.
    for (const data::variable& var : left_parameters)
    {
      left_update_dependencies.erase(var);
      left_condition_dependencies.erase(var);
    }

    for (const data::variable& var : right_parameters)
    {
      right_update_dependencies.erase(var);
      right_condition_dependencies.erase(var);
    }

    data::assignment_list other_assignments;
    if (generate_left)
    {
      other_assignments = project(summand.assignments(), right_parameters);
    }
    else
    {
      other_assignments = project(summand.assignments(), left_parameters);
    }

    // Indicates that each assignment is the identity (lhs == lhs) so only trivial updates.
    bool is_update_trivial = std::find_if(other_assignments.begin(),
      other_assignments.end(),
      [](const data::assignment& assignment)
      {
        return assignment.lhs() != assignment.rhs();
      }) == other_assignments.end();


    // Compute the synchronization vector (the values of h without functions)
    std::set<data::variable> synchronized;
    synchronized.insert(left_condition_dependencies.begin(), left_condition_dependencies.end());
    synchronized.insert(left_update_dependencies.begin(), left_update_dependencies.end());
    synchronized.insert(action_dependencies.begin(), action_dependencies.end());

    std::set<data::variable> right_synchronized;
    right_synchronized.insert(right_condition_dependencies.begin(), right_condition_dependencies.end());
    right_synchronized.insert(right_update_dependencies.begin(), right_update_dependencies.end());

    // Only keep summation variables if they occur in both conditions.
    for (const data::variable& var : summand.summation_variables())
    {
      if (std::find(synchronized.begin(), synchronized.end(), var) == synchronized.end())
      {
        right_synchronized.erase(var);
      }

      if (std::find(right_synchronized.begin(), right_synchronized.end(), var) == right_synchronized.end())
      {
        synchronized.erase(var);
      }
    }

    synchronized.insert(right_synchronized.begin(), right_synchronized.end());

    // If the other component is update trivial then it does not depend on summation parameters.
    atermpp::term_list<data::variable> our_parameters = (generate_left ? left_parameters : right_parameters);
    our_parameters = our_parameters + summand.summation_variables();

    bool is_independent = is_update_trivial && is_subset(synchronized, our_parameters);

    mCRL2log(log::verbose) << std::boolalpha << "Summand " << index
                           << " (generate_left: " << generate_left
                           << ", is_update_trivial: " << is_update_trivial
                           << ", is_independent: " << is_independent << ").\n";
    print_names("Dependencies", synchronized);

    // Create the actsync(p, e_i) action for our dependencies on p and e_i
    data::data_expression_list left_values;
    data::data_expression_list right_values;
    data::sort_expression_list sorts;

    for (const data::variable& dependency : synchronized)
    {
      left_values.push_front(data::data_expression(dependency));
      right_values.push_front(data::data_expression(dependency));
      sorts.push_front(dependency.sort());
    }

    for (const data::data_expression& expression : left_condition.implicit)
    {
      left_values.push_front(expression);
      sorts.push_front(expression.sort());
    }

    for (const data::data_expression& expression : right_condition.implicit)
    {
      right_values.push_front(expression);
    }

    // Add summand to the left specification.
    {
      process::action_label left_sync_label(std::string("syncleft_") += std::to_string(index), sorts);

      std::optional<lps::stochastic_action_summand> new_summand = make_summand(summand,
        synchronized,
        right_parameters,
        left_parameters,
        internal_action,
        tagleft,
        left_sync_label,
        left_condition,
        left_values,
        generate_left,
        is_independent);

      if (new_summand)
      {
        labels.push_back(left_sync_label);

        left_summands.push_back(new_summand.value());
      }
    }

    // The right specification.
    {
      process::action_label right_sync_label(std::string("syncright_") += std::to_string(index), sorts);

      std::optional<lps::stochastic_action_summand> new_summand = make_summand(summand,
        synchronized,
        left_parameters,
        right_parameters,
        internal_action,
        tagleft,
        right_sync_label,
        right_condition,
        right_values,
        !generate_left,
        is_independent);

      if (new_summand)
      {
        labels.push_back(right_sync_label);

        right_summands.push_back(new_summand.value());
      }
    }
  }

  // Add the labels to the LPS action specification.
  auto action_labels = spec.action_labels();
  for (const process::action_label& label : labels)
  {
    action_labels.push_front(label);
  }

  lps::deadlock_summand_vector no_deadlock_summands;
  lps::stochastic_linear_process left_process(left_parameters, no_deadlock_summands, left_summands);
  lps::stochastic_linear_process right_process(right_parameters, no_deadlock_summands, right_summands);

  lps::stochastic_process_initializer left_initial(
    project_values(data::make_assignment_list(spec.process().process_parameters(), spec.initial_process().expressions()), left_parameters),
    spec.initial_process().distribution());

  lps::stochastic_process_initializer right_initial(
    project_values(data::make_assignment_list(spec.process().process_parameters(), spec.initial_process().expressions()), right_parameters),
    spec.initial_process().distribution());

  // Create the new LPS and return it.
  return std::make_pair(
    lps::stochastic_specification(spec.data(), action_labels, spec.global_variables(), left_process, left_initial),
    lps::stochastic_specification(spec.data(), action_labels, spec.global_variables(), right_process, right_initial));
}

