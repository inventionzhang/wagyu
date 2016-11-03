#pragma once

#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/local_minimum.hpp>

#ifdef DEBUG
#include <mapbox/geometry/wagyu/exceptions.hpp>
#endif

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
inline void reverse_horizontal(edge<T>& e) {
    // swap horizontal edges' top and bottom x's so they follow the natural
    // progression of the bounds - ie so their xbots will align with the
    // adjoining lower edge. [Helpful in the process_horizontal() method.]
    std::swap(e.top.x, e.bot.x);
}

// Make a list start on a local maximum by
// shifting all the points not on a local maximum to the
template <typename T>
void start_list_on_local_maximum(edge_list<T>& edges) {
    if (edges.size() <= 2) {
        return;
    }
    // Find the first local maximum going forward in the list
    auto prev_edge = edges.end();
    --prev_edge;
    bool prev_edge_is_horizontal = is_horizontal(*prev_edge);
    auto edge = edges.begin();
    bool edge_is_horizontal;
    bool y_decreasing_before_last_horizontal = false; // assume false at start

    while (edge != edges.end()) {
        edge_is_horizontal = is_horizontal(*edge);
        if ((!prev_edge_is_horizontal && !edge_is_horizontal && edge->top == prev_edge->top)) {
            break;
        }
        if (!edge_is_horizontal && prev_edge_is_horizontal) {
            if (y_decreasing_before_last_horizontal &&
                (edge->top == prev_edge->bot || edge->top == prev_edge->top)) {
                break;
            }
        } else if (!y_decreasing_before_last_horizontal && !prev_edge_is_horizontal &&
                   edge_is_horizontal &&
                   (prev_edge->top == edge->top || prev_edge->top == edge->bot)) {
            y_decreasing_before_last_horizontal = true;
        }
        prev_edge_is_horizontal = edge_is_horizontal;
        prev_edge = edge;
        ++edge;
    }
    if (edge != edges.end() && edge != edges.begin()) {
        edges.splice(edges.end(), edges, edges.begin(), edge);
    } else if (edges.begin()->top.y < prev_edge->bot.y) {
        // This should only happen in lines not in rings
        std::reverse(edges.begin(), edges.end());
    }
}

template <typename T>
bound<T> create_bound_towards_minimum(edge_list<T>& edges) {
    if (edges.size() == 1) {
        if (is_horizontal(edges.front())) {
            reverse_horizontal(edges.front());
        }
        bound<T> bnd;
        bnd.edges.splice(bnd.edges.end(), edges, edges.begin(), edges.end());
        return bnd;
    }
    auto next_edge = edges.begin();
    auto edge = next_edge;
    ++next_edge;
    bool edge_is_horizontal = is_horizontal(*edge);
    if (edge_is_horizontal) {
        reverse_horizontal(*edge);
    }
    bool next_edge_is_horizontal;
    bool y_increasing_before_last_horizontal = false; // assume false at start

    while (next_edge != edges.end()) {
        next_edge_is_horizontal = is_horizontal(*next_edge);
        if ((!next_edge_is_horizontal && !edge_is_horizontal && edge->bot == next_edge->bot)) {
            break;
        }
        if (!next_edge_is_horizontal && edge_is_horizontal) {
            if (y_increasing_before_last_horizontal &&
                (next_edge->bot == edge->bot || next_edge->bot == edge->top)) {
                break;
            }
        } else if (!y_increasing_before_last_horizontal && !edge_is_horizontal &&
                   next_edge_is_horizontal &&
                   (edge->bot == next_edge->top || edge->bot == next_edge->bot)) {
            y_increasing_before_last_horizontal = true;
        }
        edge_is_horizontal = next_edge_is_horizontal;
        edge = next_edge;
        if (edge_is_horizontal) {
            reverse_horizontal(*edge);
        }
        ++next_edge;
    }
    bound<T> bnd;
    bnd.edges.splice(bnd.edges.end(), edges, edges.begin(), next_edge);
    std::reverse(bnd.edges.begin(), bnd.edges.end());
    return bnd;
}

template <typename T>
bound<T> create_bound_towards_maximum(edge_list<T>& edges) {
    if (edges.size() == 1) {
        bound<T> bnd;
        bnd.edges.splice(bnd.edges.end(), edges, edges.begin(), edges.end());
        return bnd;
    }
    auto next_edge = edges.begin();
    auto edge = next_edge;
    ++next_edge;
    bool edge_is_horizontal = is_horizontal(*edge);
    bool next_edge_is_horizontal;
    bool y_decreasing_before_last_horizontal = false; // assume false at start

    while (next_edge != edges.end()) {
        next_edge_is_horizontal = is_horizontal(*next_edge);
        if ((!next_edge_is_horizontal && !edge_is_horizontal && edge->top == next_edge->top)) {
            break;
        }
        if (!next_edge_is_horizontal && edge_is_horizontal) {
            if (y_decreasing_before_last_horizontal &&
                (next_edge->top == edge->bot || next_edge->top == edge->top)) {
                break;
            }
        } else if (!y_decreasing_before_last_horizontal && !edge_is_horizontal &&
                   next_edge_is_horizontal &&
                   (edge->top == next_edge->top || edge->top == next_edge->bot)) {
            y_decreasing_before_last_horizontal = true;
        }
        edge_is_horizontal = next_edge_is_horizontal;
        edge = next_edge;
        ++next_edge;
    }
    bound<T> bnd;
    bnd.edges.splice(bnd.edges.end(), edges, edges.begin(), next_edge);
    return bnd;
}

template <typename T>
void set_edge_data(edge_list<T>& edges, bound_ptr<T> bound) {
    for (auto& e : edges) {
        e.bound = bound;
    }
}

template <typename T>
void fix_horizontals(bound<T>& bnd) {

    auto edge_itr = bnd.edges.begin();
    auto next_itr = std::next(edge_itr);
    if (next_itr == bnd.edges.end()) {
        return;
    }
    if (is_horizontal(*edge_itr) && next_itr->bot != edge_itr->top) {
        reverse_horizontal(*edge_itr);
    }
    auto prev_itr = edge_itr++;
    while (edge_itr != bnd.edges.end()) {
        if (is_horizontal(*edge_itr) && prev_itr->top != edge_itr->bot) {
            reverse_horizontal(*edge_itr);
        }
        prev_itr = edge_itr;
        ++edge_itr;
    }
}

template <typename T>
void move_horizontals_on_left_to_right(bound<T>& left_bound, bound<T>& right_bound) {
    // We want all the horizontal segments that are at the same Y as the minimum to be on the right
    // bound
    auto edge_itr = left_bound.edges.begin();
    while (edge_itr != left_bound.edges.end()) {
        if (!is_horizontal(*edge_itr)) {
            break;
        }
        reverse_horizontal(*edge_itr);
        ++edge_itr;
    }
    if (edge_itr == left_bound.edges.begin()) {
        return;
    }
    auto original_first = right_bound.edges.begin();
    right_bound.edges.splice(original_first, left_bound.edges, left_bound.edges.begin(), edge_itr);
    std::reverse(right_bound.edges.begin(), original_first);
}

template <typename T>
void add_line_to_local_minima_list(edge_list<T>& edges, local_minimum_list<T>& minima_list) {

    if (edges.empty()) {
        return;
    }
    // Adjust the order of the ring so we start on a local maximum
    // therefore we start right away on a bound.
    start_list_on_local_maximum(edges);
    bound_ptr<T> last_maximum = nullptr;
    while (!edges.empty()) {
        bool lm_minimum_has_horizontal = false;
        auto to_minimum = create_bound_towards_minimum(edges);
        assert(!to_minimum.edges.empty());
        fix_horizontals(to_minimum);
        to_minimum.poly_type = polygon_type_subject;
        to_minimum.maximum_bound = last_maximum;
        to_minimum.winding_delta = 0;
        auto to_min_first_non_horizontal = to_minimum.edges.begin();
        while (to_min_first_non_horizontal != to_minimum.edges.end() &&
               is_horizontal(*to_min_first_non_horizontal)) {
            lm_minimum_has_horizontal = true;
            ++to_min_first_non_horizontal;
        }
        if (edges.empty()) {
            if (to_min_first_non_horizontal != to_minimum.edges.end() &&
                to_min_first_non_horizontal->dx > 0.0) {
                to_minimum.side = edge_left;
                bound<T> right_bound;
                right_bound.winding_delta = 0;
                right_bound.side = edge_right;
                right_bound.poly_type = polygon_type_subject;
                move_horizontals_on_left_to_right(to_minimum, right_bound);
                set_edge_data(to_minimum.edges, &to_minimum);
                auto const& min_front = to_minimum.edges.front();
                if (!right_bound.empty()) {
                    set_edge_data(right_bound.edges, &right_bound);
                }
                minima_list.emplace_back(std::move(to_minimum), std::move(right_bound), min_front.y,
                                         lm_minimum_has_horizontal);
                if (last_maximum) {
                    last_maximum->maximum_bound = &(minima_list.back().left_bound);
                    last_maximum = nullptr;
                }
            } else {
                to_minimum.side = edge_right;
                bound<T> left_bound;
                left_bound.winding_delta = 0;
                left_bound.side = edge_left;
                left_bound.poly_type = polygon_type_subject;
                auto const& min_front = to_minimum.edges.front();
                minima_list.emplace_back(std::move(left_bound), std::move(to_minimum), min_front.y);
                if (last_maximum) {
                    last_maximum->maximum_bound = &(minima_list.back().right_bound);
                    last_maximum = nullptr;
                }
            }
            break;
        }
        bool minimum_is_left = true;
        auto to_maximum = create_bound_towards_maximum(edges);
        assert(!to_maximum.edges.empty());
        fix_horizontals(to_maximum);
        auto to_max_first_non_horizontal = to_minimum.edges.begin();
        while (to_max_first_non_horizontal != to_maximum.edges.end() &&
               is_horizontal(*to_max_first_non_horizontal)) {
            lm_minimum_has_horizontal = true;
            ++to_max_first_non_horizontal;
        }
        if (to_max_first_non_horizontal != to_maximum.edges.end() &&
            (to_min_first_non_horizontal == to_minimum.edges.end() ||
             to_max_first_non_horizontal->dx > to_min_first_non_horizontal->dx)) {
            minimum_is_left = false;
            move_horizontals_on_left_to_right(to_maximum, to_minimum);
        } else {
            minimum_is_left = true;
            move_horizontals_on_left_to_right(to_minimum, to_maximum);
        }
        auto const& min_front = to_minimum.edges.front();
        to_maximum.poly_type = polygon_type_subject;
        to_maximum.winding_delta = 0;
        set_edge_data(to_minimum.edges, &to_minimum);
        set_edge_data(to_maximum.edges, &to_maximum);
        if (!minimum_is_left) {
            to_minimum.side = edge_right;
            to_maximum.side = edge_left;
            minima_list.emplace_back(std::move(to_maximum), std::move(to_minimum), min_front.bot.y,
                                     lm_minimum_has_horizontal);
            if (last_maximum) {
                last_maximum->maximum_bound = &(minima_list.back().right_bound);
            }
            last_maximum = &(minima_list.back().left_bound);
        } else {
            to_minimum.side = edge_left;
            to_maximum.side = edge_right;
            minima_list.emplace_back(std::move(to_minimum), std::move(to_maximum), min_front.bot.y,
                                     lm_minimum_has_horizontal);
            if (last_maximum) {
                last_maximum->maximum_bound = &(minima_list.back().left_bound);
            }
            last_maximum = &(minima_list.back().right_bound);
        }
    }
}

template <typename T>
void add_ring_to_local_minima_list(edge_list<T>& edges,
                                   local_minimum_list<T>& minima_list,
                                   polygon_type poly_type) {

    if (edges.empty()) {
        return;
    }
    // Adjust the order of the ring so we start on a local maximum
    // therefore we start right away on a bound.
    start_list_on_local_maximum(edges);

    bound_ptr<T> first_minimum = nullptr;
    bound_ptr<T> last_maximum = nullptr;
    while (!edges.empty()) {
        bool lm_minimum_has_horizontal = false;
        auto to_minimum = create_bound_towards_minimum(edges);
        if (edges.empty()) {
            throw std::runtime_error("Edges is empty after only creating a single bound.");
        }
        auto to_maximum = create_bound_towards_maximum(edges);
        fix_horizontals(to_minimum);
        fix_horizontals(to_maximum);
        auto to_max_first_non_horizontal = to_maximum.edges.begin();
        auto to_min_first_non_horizontal = to_minimum.edges.begin();
        bool minimum_is_left = true;
        while (to_max_first_non_horizontal != to_maximum.edges.end() &&
               is_horizontal(*to_max_first_non_horizontal)) {
            lm_minimum_has_horizontal = true;
            ++to_max_first_non_horizontal;
        }
        while (to_min_first_non_horizontal != to_minimum.edges.end() &&
               is_horizontal(*to_min_first_non_horizontal)) {
            lm_minimum_has_horizontal = true;
            ++to_min_first_non_horizontal;
        }
#ifdef DEBUG
        if (to_max_first_non_horizontal == to_maximum.edges.end() ||
            to_min_first_non_horizontal == to_minimum.edges.end()) {
            throw clipper_exception("should not have a horizontal only bound for a ring");
        }
#endif
        if (lm_minimum_has_horizontal) {
            if (to_max_first_non_horizontal->bot.x > to_min_first_non_horizontal->bot.x) {
                minimum_is_left = true;
                move_horizontals_on_left_to_right(to_minimum, to_maximum);
            } else {
                minimum_is_left = false;
                move_horizontals_on_left_to_right(to_maximum, to_minimum);
            }
        } else {
            if (to_max_first_non_horizontal->dx > to_min_first_non_horizontal->dx) {
                minimum_is_left = false;
            } else {
                minimum_is_left = true;
            }
        }
        assert(!to_minimum.edges.empty());
        assert(!to_maximum.edges.empty());
        auto const& min_front = to_minimum.edges.front();
        if (last_maximum) {
            to_minimum.maximum_bound = last_maximum;
        }
        to_minimum.poly_type = poly_type;
        to_maximum.poly_type = poly_type;
        set_edge_data(to_minimum.edges, &to_minimum);
        set_edge_data(to_maximum.edges, &to_maximum);
        if (!minimum_is_left) {
            to_minimum.side = edge_right;
            to_maximum.side = edge_left;
            to_minimum.winding_delta = -1;
            to_maximum.winding_delta = 1;
            minima_list.emplace_back(std::move(to_maximum), std::move(to_minimum), min_front.bot.y,
                                     lm_minimum_has_horizontal);
            if (!last_maximum) {
                first_minimum = &(minima_list.back().right_bound);
            } else {
                last_maximum->maximum_bound = &(minima_list.back().right_bound);
            }
            last_maximum = &(minima_list.back().left_bound);
        } else {
            to_minimum.side = edge_left;
            to_maximum.side = edge_right;
            to_minimum.winding_delta = -1;
            to_maximum.winding_delta = 1;
            minima_list.emplace_back(std::move(to_minimum), std::move(to_maximum), min_front.bot.y,
                                     lm_minimum_has_horizontal);
            if (!last_maximum) {
                first_minimum = &(minima_list.back().left_bound);
            } else {
                last_maximum->maximum_bound = &(minima_list.back().left_bound);
            }
            last_maximum = &(minima_list.back().right_bound);
        }
    }
    last_maximum->maximum_bound = first_minimum;
    first_minimum->maximum_bound = last_maximum;
}

template <typename T>
void initialize_lm(local_minimum_ptr_list_itr<T>& lm) {
    if (!(*lm)->left_bound.edges.empty()) {
        (*lm)->left_bound.current_edge = (*lm)->left_bound.edges.begin();
        (*lm)->left_bound.curr.x = static_cast<double>((*lm)->left_bound.current_edge->bot.x);
        (*lm)->left_bound.curr.y = static_cast<double>((*lm)->left_bound.current_edge->bot.y);
        (*lm)->left_bound.winding_count = 0;
        (*lm)->left_bound.winding_count2 = 0;
        (*lm)->left_bound.side = edge_left;
        (*lm)->left_bound.ring = nullptr;
    }
    if (!(*lm)->right_bound.edges.empty()) {
        (*lm)->right_bound.current_edge = (*lm)->right_bound.edges.begin();
        (*lm)->right_bound.curr.x = static_cast<double>((*lm)->right_bound.current_edge->bot.x);
        (*lm)->right_bound.curr.y = static_cast<double>((*lm)->right_bound.current_edge->bot.y);
        (*lm)->right_bound.winding_count = 0;
        (*lm)->right_bound.winding_count2 = 0;
        (*lm)->right_bound.side = edge_right;
        (*lm)->right_bound.ring = nullptr;
    }
}
}
}
}
