#include "physics.hpp"


void
mw::inelastic_collision_1d(double CR, double m1, double &v1, double m2,
                           double &v2)
{
  const double u1 = (CR*m2*(v2 - v1) + m1*v1 + m2*v2) / (m1 + m2);
  const double u2 = (CR*m1*(v1 - v2) + m1*v1 + m2*v2) / (m1 + m2);
  v1 = u1;
  v2 = u2;
}

void
mw::inelastic_collision(double CR, const vec2d_d &_n, double m1, vec2d_d &v1,
                        double m2, vec2d_d &v2)
{
  if (mag2(v1) < 1e-6 and mag2(v2) < 1e-6)
    error("[inelastic_collision] buldozer");

  const vec2d_d n = normalized(_n);

  const double Jn = (m1*m2)/(m1 + m2)*(1 + CR)*dot(v2 - v1, n);
  const vec2d_d dv1 = (Jn/m1)*n;
  const vec2d_d dv2 = -(Jn/m2)*n;

  v1 = v1 + dv1;
  v2 = v2 + dv2;
}

void
mw::pushing(double m1, const pt2d_d &o1, vec2d_d &a1,
            double m2, const pt2d_d &o2, vec2d_d &a2)
{
  const vec2d_d n12 = (o2 - o1)/mag(o2 - o1);
  const vec2d_d n21 = -1.*n12;

  const double a1n12 = dot(a1, n12);
  const double a2n21 = dot(a2, n21);

  const vec2d_d F12 = a1n12 > 0 ? m1*a1n12*n12 : vec2d_d(0, 0);
  const vec2d_d F21 = a2n21 > 0 ? m2*a2n21*n21 : vec2d_d(0, 0);

  const vec2d_d da1 = (F21 - F12)/m1 + n21*1e-6;
  const vec2d_d da2 = (F12 - F21)/m2 + n21*1e-6;

  a1 = a1 + da1;
  a2 = a2 + da2;
}
