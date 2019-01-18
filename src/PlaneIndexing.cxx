#include "WireCellImg/PlaneIndexing.h"

using namespace WireCell;


Img::PlaneIndexing::PlaneIndexing(const wire_pair_vector_t& zero_one_wires, size_t normal_axis)
    : m_nplanes(zero_one_wires.size())
    , m_pitch_mag(m_nplanes, 0.0)
    , m_pitch_dir(m_nplanes)
    , m_center(m_nplanes)
    , m_zero_crossing(m_nplanes, m_nplanes)
    , m_wire_jump(m_nplanes, m_nplanes)
    , m_a(boost::extents[m_nplanes][m_nplanes][m_nplanes])
    , m_b(boost::extents[m_nplanes][m_nplanes][m_nplanes])
    , m_c(boost::extents[m_nplanes][m_nplanes][m_nplanes])
{



    // really we are working in 2D space, so project all vectors into the plane.
    auto project = [normal_axis](Vector v) { v[normal_axis]=0.0; return v; };

    const size_t nplanes = zero_one_wires.size();

    // must go through 1, 2 and 3 combonations

    // First, find the per-plane things
    for (plane_index_t iplane=0; iplane<nplanes; ++iplane) {
        const auto& wpair = zero_one_wires[iplane];

        const auto r0 = wpair.first->ray();
        const auto r1 = wpair.second->ray();

        // vector of closest approach between the two parallel wires
        const auto rpitch = ray_pitch(r0, r1);

        // relative pitch vector
        auto rpv = project(rpitch.second - rpitch.first);

        m_pitch_mag[iplane] = rpv.magnitude();
        m_pitch_dir[iplane] = rpv.norm();

        // center point of wire 0
        m_center[iplane] = 0.5*(project(r0.first + r0.second));
    }

    // Next find cross-plane things
    for (plane_index_t il=0; il<nplanes; ++il) {
        for (plane_index_t im=0; im<nplanes; ++im) {

            // wire pairs for plane l and m
            const auto& wpl = zero_one_wires[il];
            const auto& wpm = zero_one_wires[im];

            // zero wires for plane l and m
            const IWire::pointer& w0l = wpl.first;
            const IWire::pointer& w0m = wpm.first;
            const IWire::pointer& w1l = wpl.second;
            const IWire::pointer& w1m = wpm.second;

            // Iterate only over triangle to avoid extra work.
            if (il < im) { 
                const auto r00 = ray_pitch(w0l->ray(), w0m->ray());
                m_zero_crossing(il,im) = project(r00.first); // on l wire
                m_zero_crossing(im,il) = project(r00.second); // on m wire

                // along l-plane wire 0, crossing of m-plane wire 1.
                {
                    const auto ray = ray_pitch(w0l->ray(), w1m->ray());
                    m_wire_jump(il, im) = project(ray.first - r00.first);
                }
                // along m-plane wire 0, crossing of l-plane wire 1.
                {
                    const auto ray = ray_pitch(w0m->ray(), w1l->ray());
                    m_wire_jump(im, il) = project(ray.first - r00.second);
                }

            }
            if (il == im) {
                m_zero_crossing(il,im).invalidate();
                m_wire_jump(il,im).invalidate();
            }
        }
    }

    // Finally, find triple-plane things (coefficients for
    // P^{lmn}_{ij}).  Needs some of the above completed.
    for (plane_index_t in=0; in<nplanes; ++in) {
        const auto& pn = m_pitch_dir[in];
        const double cp = m_center[in].dot(pn);

        for (plane_index_t il=0; il<nplanes; ++il) {
            if (il == in) { continue; }

            for (plane_index_t im=0; im<nplanes; ++im) {
                if (im == in or im == il) { continue; }

                const double rlmpn = m_zero_crossing(il,im).dot(pn);
                const double wlmpn = m_zero_crossing(im,il).dot(pn);
                const double wmlpn = m_zero_crossing(im,il).dot(pn);

                m_a[il][im][in] = wlmpn;
                m_b[il][im][in] = wmlpn;
                m_c[il][im][in] = rlmpn - cp;
            }
        }
    }
}

Vector Img::PlaneIndexing::zero_crossing(plane_pair_t plane) const
{
    return m_zero_crossing(plane.first, plane.second);
}


Vector Img::PlaneIndexing::wire_crossing(const plane_pair_t& plane, const wip_pair_t& wips) const
{
    const plane_index_t l = plane.first, m = plane.second;
    const wip_index_t i=wips.first, j=wips.second;
    const auto& r00 = m_zero_crossing(l,m);
    const auto& wlm = m_wire_jump(l,m);
    const auto& wml = m_wire_jump(m,l);
    return r00 + i*wlm + j*wml;
}

double Img::PlaneIndexing::pitch_location(const plane_triple_t& ind, wip_index_t w1ind, wip_index_t w2ind) const
{
    tensor_t::index il=get<0>(ind), im=get<1>(ind), in=get<2>(ind);

    return m_c[il][im][in] + m_a[il][im][in]*w1ind + m_b[il][im][in]*w2ind;
}
