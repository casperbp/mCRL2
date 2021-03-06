// Copyright (c) 2009-2013 University of Twente
// Copyright (c) 2009-2013 Michael Weber <michaelw@cs.utwente.nl>
// Copyright (c) 2009-2013 Maks Verver <maksverver@geocities.com>
// Copyright (c) 2009-2013 Eindhoven University of Technology
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MCRL2_PG_GRAPH_ORDERING_H
#define MCRL2_PG_GRAPH_ORDERING_H

#include "mcrl2/pg/Graph.h"

/*! \file GraphOrdering.h
    \brief Functions to analyze vertex order in a static graph.
    \ingroup ParityGameData

    This file declares functions that are useful to determine the ordering of
    vertices in a graph, and find ways to reorder them (for example, to obtain
    mostly forward-oriented edges).

    @see ParityGame::shuffle
*/


/*! Traverses the graph in breadth-first search order, and returns the result
    in `perm`, such that perm[v] = i if v is the i-th visited vertex. */
void get_bfs_order(const StaticGraph &graph, std::vector<verti> &perm);

/*! Traverses the graph in depth-first search order, and returns the result
    in `perm`, such that perm[v] = i if v is the i-th visited vertex. */
void get_dfs_order(const StaticGraph &graph, std::vector<verti> &perm);

/*! Count the number of edges with a specific ordering of vertices.
    An edge (i,j) is counted if sign(j - i) == sign(dir).*/
edgei count_ordered_edges(const StaticGraph &g, int dir);

/*! Returns the number of edges (i,j) such that i &lt; j */
inline edgei count_forward_edges(const StaticGraph &g) {
    return count_ordered_edges(g, +1);
}

/*! Returns the number of edges (i,j) such that i &gt; j */
inline edgei count_backward_edges(const StaticGraph &g) {
    return count_ordered_edges(g, -1);
}

/*! Returns the number of edges (i,j) such that i = j */
inline edgei counts_self_edges(const StaticGraph &g) {
    return count_ordered_edges(g, 0);
}

#endif /* ndef MCRL2_PG_GRAPH_ORDERING_H */
