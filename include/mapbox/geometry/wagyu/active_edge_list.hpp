#pragma once

#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/exception.hpp>
#include <mapbox/geometry/wagyu/local_minimum.hpp>
#include <mapbox/geometry/wagyu/ring.hpp>
#include <mapbox/geometry/wagyu/scanbeam.hpp>
#include <mapbox/geometry/wagyu/util.hpp>

namespace mapbox { namespace geometry { namespace wagyu {

template <typename T>
inline bool e2_inserts_before_e1(edge<T> const& e1,
                                 edge<T> const& e2)
{
    if (e2.curr.x == e1.curr.x) 
    {
        if (e2.top.y > e1.top.y)
        {
            return e2.top.x < get_current_x(e1, e2.top.y);
        }
        else
        {
            return e1.top.x > get_current_x(e2, e1.top.y);
        }
    } 
    else
    {
        return e2.curr.x < e1.curr.x;
    }
}

template <typename T>
void insert_edge_into_AEL(edge_ptr<T> edge,
                          edge_ptr<T> start_edge,
                          edge_ptr<T> & active_edges)
{
    if (!active_edges)
    {
        edge->prev_in_AEL = nullptr;
        edge->next_in_AEL = nullptr;
        active_edges = edge;
    }
    else if (!start_edge && 
             e2_inserts_before_e1(*active_edges, *edge))
    {
        edge->prev_in_AEL = nullptr;
        edge->next_in_AEL = active_edges;
        active_edges->prev_in_AEL = edge;
        active_edges = edge;
    } 
    else
    {
        if(!start_edge)
        {
            start_edge = active_edges;
        }
        while (start_edge->next_in_AEL &&
               !e2_inserts_before_e1(*start_edge->next_in_AEL , *edge))
        {
            start_edge = start_edge->next_in_AEL;
        }
        edge->next_in_AEL = start_edge->next_in_AEL;
        if (start_edge->next_in_AEL)
        {
            start_edge->next_in_AEL->prev_in_AEL = edge;
        }
        edge->prev_in_AEL = start_edge;
        start_edge->next_in_AEL = edge;
    }
}

template <typename T>
void delete_from_AEL(edge_ptr<T> e,
                     edge_ptr<T> & active_edges)
{
    edge_ptr<T> Aelprev = e->prev_in_AEL;
    edge_ptr<T> Aelnext = e->next_in_AEL;
    if (!Aelprev && !Aelnext && (e != active_edges))
    {
        return; //already deleted
    }
    if (Aelprev)
    {
        Aelprev->next_in_AEL = Aelnext;
    }
    else
    {
        active_edges = Aelnext;
    }
    if (Aelnext)
    {
        Aelnext->prev_in_AEL = Aelprev;
    }
    e->next_in_AEL = nullptr;
    e->prev_in_AEL = nullptr;
}

template <typename T>
void swap_positions_in_AEL(edge_ptr<T> Edge1,
                           edge_ptr<T> Edge2,
                           edge_ptr<T> & active_edges)
{
    //check that one or other edge hasn't already been removed from AEL ...
    if (Edge1->next_in_AEL == Edge1->prev_in_AEL ||
        Edge2->next_in_AEL == Edge2->prev_in_AEL)
    {
        return;
    }

    if (Edge1->next_in_AEL == Edge2)
    {
        edge_ptr<T> next = Edge2->next_in_AEL;
        if (next)
        {
            next->prev_in_AEL = Edge1;
        }
        edge_ptr<T> prev = Edge1->prev_in_AEL;
        if (prev)
        {
            prev->next_in_AEL = Edge2;
        }
        Edge2->prev_in_AEL = prev;
        Edge2->next_in_AEL = Edge1;
        Edge1->prev_in_AEL = Edge2;
        Edge1->next_in_AEL = next;
    }
    else if (Edge2->next_in_AEL == Edge1)
    {
        edge_ptr<T> next = Edge1->next_in_AEL;
        if (next)
        {
            next->prev_in_AEL = Edge2;
        }
        edge_ptr<T> prev = Edge2->prev_in_AEL;
        if (prev)
        {
            prev->next_in_AEL = Edge1;
        }
        Edge1->prev_in_AEL = prev;
        Edge1->next_in_AEL = Edge2;
        Edge2->prev_in_AEL = Edge1;
        Edge2->next_in_AEL = next;
    }
    else
    {
        edge_ptr<T> next = Edge1->next_in_AEL;
        edge_ptr<T> prev = Edge1->prev_in_AEL;
        Edge1->next_in_AEL = Edge2->next_in_AEL;
        if (Edge1->next_in_AEL)
        {
            Edge1->next_in_AEL->prev_in_AEL = Edge1;
        }
        Edge1->prev_in_AEL = Edge2->prev_in_AEL;
        if (Edge1->prev_in_AEL)
        {
            Edge1->prev_in_AEL->next_in_AEL = Edge1;
        }
        Edge2->next_in_AEL = next;
        if (Edge2->next_in_AEL)
        {
            Edge2->next_in_AEL->prev_in_AEL = Edge2;
        }
        Edge2->prev_in_AEL = prev;
        if (Edge2->prev_in_AEL)
        {
            Edge2->prev_in_AEL->next_in_AEL = Edge2;
        }
    }

    if (!Edge1->prev_in_AEL)
    {
        active_edges = Edge1;
    }
    else if (!Edge2->prev_in_AEL)
    {
        active_edges = Edge2;
    }
}

template <typename T>
void update_edge_into_AEL(edge_ptr<T> &e, 
                          edge_ptr<T> & active_edges,
                          scanbeam_list<T> & scanbeam)
{
    if (!e->next_in_LML)
    {
        throw clipper_exception("UpdateEdgeIntoAEL: invalid call");
    }

    e->next_in_LML->index = e->index;
    TEdge* Aelprev = e->prev_in_AEL;
    TEdge* Aelnext = e->next_in_AEL;
    if (Aelprev)
    {
        Aelprev->next_in_AEL = e->next_in_LML;
    }
    else
    {
        active_edges = e->next_in_LML;
    }
    if (Aelnext)
    {
        Aelnext->prev_in_AEL = e->next_in_LML;
    }
    e->next_in_LML->Side = e->Side;
    e->next_in_LML->winding_delta = e->winding_delta;
    e->next_in_LML->winding_count = e->winding_count;
    e->next_in_LML->winding_count2 = e->winding_count2;
    e = e->next_in_LML;
    e->curr = e->bot;
    e->prev_in_AEL = Aelprev;
    e->next_in_AEL = Aelnext;
    if (!IsHorizontal(*e))
    {
        scanbeam.push(e->top.y);
    }
}

template <typename T>
bool is_even_odd_alt_fill_type(edge<T> const& edge,
                               fill_type subject_fill_type,
                               fill_type clip_fill_type)
{
    if (edge.PolyTyp == polygon_type_subject)
    {
        return m_ClipFillType == fill_type_even_odd;
    }
    else
    {
        return m_SubjFillType == fill_type_even_odd;
    }
}

template <typename T>
void set_winding_count(edge<T> & edge,
                       clip_type cliptype,
                       fill_type subject_fill_type,
                       fill_type clip_fill_type,
                       edge_ptr<T> active_edges)
{
    using value_type = T;
    edge_ptr<value_type> e = edge.prev_in_AEL;
    //find the edge of the same polytype that immediately preceeds 'edge' in AEL
    while (e  && 
           ((e->PolyTyp != edge.PolyTyp) || (e->winding_delta == 0)))
    {
        e = e->prev_in_AEL;
    }
    if (!e)
    {
        if (edge.winding_delta == 0)
        {
            fill_type pft = (edge.PolyTyp == polygon_type_subject ? subject_fill_type : clip_fill_type);
            edge.winding_count = (pft == fill_type_negative ? -1 : 1);
        }
        else
        {
            edge.winding_count = edge.winding_delta;
        }
        edge.winding_count2 = 0;
        e = active_edges; //ie get ready to calc winding_count2
    }   
    else if (edge.winding_delta == 0 && cliptype != clip_type_union)
    {
        edge.winding_count = 1;
        edge.winding_count2 = e->winding_count2;
        e = e->next_in_AEL; //ie get ready to calc winding_count2
    }
    else if (IsEvenOddFillType(edge))
    {
        //EvenOdd filling ...
        if (edge.winding_delta == 0)
        {
            //are we inside a subj polygon ...
            bool Inside = true;
            edge_ptr<value_type> e2 = e->prev_in_AEL;
            while (e2)
            {
                if (e2->PolyTyp == e->PolyTyp && e2->winding_delta != 0)
                {
                    Inside = !Inside;
                }
                e2 = e2->prev_in_AEL;
            }
            edge.winding_count = (Inside ? 0 : 1);
        }
        else
        {
            edge.winding_count = edge.winding_delta;
        }
        edge.winding_count2 = e->winding_count2;
        e = e->next_in_AEL; //ie get ready to calc winding_count2
    } 
    else
    {
        //nonZero, Positive or Negative filling ...
        if (e->winding_count * e->winding_delta < 0)
        {
            //prev edge is 'decreasing' WindCount (WC) toward zero
            //so we're outside the previous polygon ...
            if (std::abs(e->winding_count) > 1)
            {
                //outside prev poly but still inside another.
                //when reversing direction of prev poly use the same WC 
                if (e->winding_delta * edge.winding_delta < 0)
                {
                    edge.winding_count = e->winding_count;
                }
                else
                {
                    //otherwise continue to 'decrease' WC ...
                    edge.winding_count = e->winding_count + edge.winding_delta;
                }
            } 
            else
            {
                //now outside all polys of same polytype so set own WC ...
                edge.winding_count = (edge.winding_delta == 0 ? 1 : edge.winding_delta);
            }
        }
        else
        {
            //prev edge is 'increasing' WindCount (WC) away from zero
            //so we're inside the previous polygon ...
            if (edge.winding_delta == 0)
            {
                edge.winding_count = (e->winding_count < 0 ? e->winding_count - 1 : e->winding_count + 1);
            }
            else if (e->winding_delta * edge.winding_delta < 0)
            {
                //if wind direction is reversing prev then use same WC
                edge.winding_count = e->winding_count;
            }
            else 
            {
                //otherwise add to WC ...
                edge.winding_count = e->winding_count + edge.winding_delta;
            }
        }
        edge.winding_count2 = e->winding_count2;
        e = e->next_in_AEL; //ie get ready to calc winding_count2
    }

    //update winding_count2 ...
    if (is_even_odd_alt_fill_type(edge))
    {
        //EvenOdd filling ...
        while (e != &edge)
        {
            if (e->winding_delta != 0)
            {
                edge.winding_count2 = (edge.winding_count2 == 0 ? 1 : 0);
            }
            e = e->next_in_AEL;
        }
    }
    else
    {
        //nonZero, Positive or Negative filling ...
        while ( e != &edge )
        {
            edge.winding_count2 += e->winding_delta;
            e = e->next_in_AEL;
        }
    }
}

template <typename T>
bool is_contributing(edge<T> const& edge,
                     clip_type cliptype,
                     fill_type subject_fill_type,
                     fill_type clip_fill_type)
{
    fill_type pft = subject_fill_type;
    fill_type pft2 = clip_fill_type;
    if (edge.PolyTyp != polygon_type_subject)
    {
        pft = clip_fill_type;
        pft2 = subject_fill_type;
    }

    switch (pft)
    {
        case fill_type_even_odd: 
            //return false if a subj line has been flagged as inside a subj polygon
            if (edge.winding_delta == 0 && edge.winding_count != 1)
            {
                return false;
            }
            break;
        case fill_type_non_zero:
            if (std::abs(edge.winding_count) != 1)
            {
                return false;
            }
            break;
        case fill_type_positive: 
            if (edge.winding_count != 1)
            {
                return false;
            }
            break;
        case fill_type_negative:
        default:
            if (edge.winding_count != -1)
            {
                return false;
            }
    }

    switch (cliptype)
    {
        case clip_type_intersection:
            switch (pft2)
            {
                case fill_type_even_odd: 
                case fill_type_non_zero: 
                    return (edge.winding_count2 != 0);
                case fill_type_positive: 
                    return (edge.winding_count2 > 0);
                case fill_type_negative:
                default: 
                    return (edge.winding_count2 < 0);
            }
            break;
        case clip_type_union:
            switch(pft2)
            {
                case fill_type_even_odd: 
                case fill_type_non_zero: 
                    return (edge.winding_count2 == 0);
                case fill_type_positive: 
                    return (edge.winding_count2 <= 0);
                case fill_type_negative: 
                default: 
                    return (edge.winding_count2 >= 0);
            }
            break;
        case clip_type_difference:
            if (edge.PolyTyp == polygon_type_subject)
            {
                switch(pft2)
                {
                    case fill_type_even_odd: 
                    case fill_type_non_zero: 
                        return (edge.winding_count2 == 0);
                    case fill_type_positive: 
                        return (edge.winding_count2 <= 0);
                    case fill_type_negative: 
                    default: 
                        return (edge.winding_count2 >= 0);
                }
            }
            else
            {
                switch(pft2)
                {
                    case fill_type_even_odd: 
                    case fill_type_non_zero: 
                        return (edge.winding_count2 != 0);
                    case fill_type_positive: 
                        return (edge.winding_count2 > 0);
                    case fill_type_negative: 
                    default: 
                        return (edge.winding_count2 < 0);
                }
            }
            break;
        case ctXor:
            if (edge.winding_delta == 0) 
            {
                //XOr always contributing unless open
                switch(pft2)
                {
                    case fill_type_even_odd: 
                    case fill_type_non_zero: 
                        return (edge.winding_count2 == 0);
                    case fill_type_positive: 
                        return (edge.winding_count2 <= 0);
                    case fill_type_negative:
                    default: 
                        return (edge.winding_count2 >= 0);
                }
            }
            else
            {
                return true;
            }
            break;
        default:
            return true;
    }
}

template <typename T>
void insert_local_minima_into_AEL(T const botY,
                                  local_minimum_itr<T> & current_local_min,
                                  local_minimum_list<T> & minima_list,
                                  edge_ptr<T> & active_edges,
                                  ring_list<T> & rings,
                                  scanbeam_list<T> & scanbeam,
                                  clip_type cliptype,
                                  fill_type subject_fill_type,
                                  fill_type clip_fill_type)
{
    using value_type = T;
    local_minimum_ptr<T> lm;
    while (pop_local_minima(botY, lm, current_local_min, minima_list))
    {
        edge_ptr<value_type> lb = lm->left_bound;
        edge_ptr<value_type> rb = lm->right_bound;
        
        point_ptr<T> p1 = nullptr;
        if (!lb)
        {
            //nb: don't insert LB into either AEL or SEL
            insert_edge_into_AEL(rb, nullptr, active_edges);
            set_winding_count(*rb, cliptype, subject_fill_type, clip_fill_type, active_edges);
            if (is_contributing(*rb, cliptype, subject_fill_type, clip_fill_type))
            {
                p1 = add_point(rb, rb->bot, rings); 
                edge_ptr<value_type> eprev = rb->prev_in_AEL;
                if (rb->index >= 0 && 
                    rb->winding_delta != 0 && 
                    eprev &&
                    eprev->index >= 0 &&
                    eprev->curr.x == rb->curr.x &&
                    eprev->winding_delta != 0)
                {
                    add_point(eprev, rb->curr, rings);
                }
                edge_ptr<value_type> enext = rb->next_in_AEL;
                if (rb->index >= 0 &&
                    rb->winding_delta != 0 &&
                    enext &&
                    enext->index >= 0 &&
                    enext->curr.x == rb->curr.x && 
                    enext->winding_delta != 0)
                {
                    add_point(enext, rb->curr, rings);
                }
            }
        } 
        else if (!rb)
        {
            insert_edge_into_AEL(lb, nullptr, active_edges);
            set_winding_count(*lb, cliptype, subject_fill_type, clip_fill_type, active_edges);
            if (is_contributing(*lb, cliptype, subject_fill_type, clip_fill_type))
            {
                p1 = add_point(lb, lb->bot, rings);
                edge_ptr<value_type> eprev = lb->prev_in_AEL;
                if (lb->index >= 0 && 
                    lb->winding_delta != 0 && 
                    eprev && 
                    eprev->index >= 0 &&
                    eprev->curr.x == lb->curr.x && 
                    eprev->winding_delta != 0)
                {
                    add_point(eprev, lb->curr, rings);
                }
                edge_ptr<value_type> enext = lb->next_in_AEL;
                if (lb->index >= 0 &&
                    lb->winding_delta != 0 && 
                    enext &&
                    enext->index >= 0 &&
                    enext->curr.x == lb->curr.x &&
                    enext->winding_delta != 0)
                {
                    add_point(enext, lb->curr, rings);
                }
            }
            scanbeam.push(lb->top.y);
        }
        else
        {
            insert_edge_into_AEL(lb, nullptr, active_edges);
            insert_edge_into_AEL(rb, lb, active_edges);
            set_winding_count(*lb, cliptype, subject_fill_type, clip_fill_type, active_edges);
            rb->winding_count = lb->winding_count;
            rb->winding_count2 = lb->winding_count2;
            if (is_contributing(*lb, cliptype, subject_fill_type, clip_fill_type))
            {
                p1 = AddLocalMinPoly(lb, rb, lb->bot);      
                edge_ptr<value_type> eprev = lb->prev_in_AEL;
                if (lb->index >= 0 &&
                    lb->winding_delta != 0 &&
                    eprev &&
                    eprev->index >= 0 &&
                    eprev->curr.x == lb->curr.x &&
                    eprev->winding_delta != 0)
                {
                    add_point(eprev, lb->curr, rings);
                }
                edge_ptr<value_type> enext = rb->next_in_AEL;
                if (rb->index >= 0 &&
                    rb->winding_delta != 0 &&
                    enext &&
                    enext->index >= 0 &&
                    enext->curr.x == rb->curr.x &&
                    enext->winding_delta != 0)
                {
                    add_point(enext, lb->curr, rings);
                }
            }
            scanbeam.push(lb->top.y);
        }

        if (rb)
        {
            if (IsHorizontal(*rb))
            {
                AddEdgeToSEL(rb);
                if (rb->next_in_LML)
                {
                    InsertScanbeam(rb->next_in_LML->top.y);
                }
            }
            else
            {
                InsertScanbeam(rb->top.y);
            }
        }

        if (!lb || !rb)
        {
            continue;
        }

        //if any output polygons share an edge, they'll need joining later ...
        if (p1 && IsHorizontal(*rb) && 
          !m_GhostJoins.empty() && (rb->winding_delta != 0))
        {
          for (JoinList::size_type i = 0; i < m_GhostJoins.size(); ++i)
          {
            Join* jr = m_GhostJoins[i];
            //if the horizontal Rb and a 'ghost' horizontal overlap, then convert
            //the 'ghost' join to a real join ready for later ...
            if (HorzSegmentsOverlap(jr->OutPt1->Pt.x, jr->OffPt.x, rb->bot.x, rb->top.x))
              AddJoin(jr->OutPt1, p1, jr->OffPt);
          }
        }

        if (lb->index >= 0 && lb->prev_in_AEL && 
          lb->prev_in_AEL->curr.x == lb->bot.x &&
          lb->prev_in_AEL->index >= 0 &&
          slopes_equal(lb->prev_in_AEL->bot, lb->prev_in_AEL->top, lb->curr, lb->top, m_UseFullRange) &&
          (lb->winding_delta != 0) && (lb->prev_in_AEL->winding_delta != 0))
        {
            OutPt *p2 = AddOutPt(lb->prev_in_AEL, lb->bot);
            AddJoin(p1, p2, lb->top);
        }

        if(lb->next_in_AEL != rb)
        {

          if (rb->index >= 0 && rb->prev_in_AEL->index >= 0 &&
            slopes_equal(rb->prev_in_AEL->curr, rb->prev_in_AEL->top, rb->curr, rb->top, m_UseFullRange) &&
            (rb->winding_delta != 0) && (rb->prev_in_AEL->winding_delta != 0))
          {
              OutPt *p2 = AddOutPt(rb->prev_in_AEL, rb->bot);
              AddJoin(p1, p2, rb->top);
          }

          edge_ptr<value_type> e = lb->next_in_AEL;
          if (e)
          {
            while( e != rb )
            {
              //nb: For calculating winding counts etc, IntersectEdges() assumes
              //that param1 will be to the Right of param2 ABOVE the intersection ...
              IntersectEdges(rb , e , lb->curr); //order important here
              e = e->next_in_AEL;
            }
          }
        }
    }
}

}}}
