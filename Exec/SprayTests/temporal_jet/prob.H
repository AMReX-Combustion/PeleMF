#ifndef _PROB_H_
#define _PROB_H_

#include <AMReX_Print.H>
#include <AMReX_ParmParse.H>
#include <AMReX_Geometry.H>
#include <AMReX_FArrayBox.H>
#include <AMReX_IntVect.H>

#include "mechanism.H"

#include "IndexDefines.H"
#include "Constants.H"
#include "PelePhysics.H"
#include "Tagging.H"
#include "Transport.H"
#include "ProblemDerive.H"
#include "prob_parm.H"

AMREX_GPU_DEVICE
AMREX_FORCE_INLINE
void
pc_initdata(
  int i,
  int j,
  int k,
  amrex::Array4<amrex::Real> const& state,
  amrex::GeometryData const& geomdata,
  ProbParmDevice const& prob_parm)
{

  // Geometry
  const amrex::Real* prob_lo = geomdata.ProbLo();
  const amrex::Real* prob_hi = geomdata.ProbHi();
  const amrex::Real xshift = prob_lo[0] + 0.5 * (prob_hi[0] - prob_lo[0]);
  const amrex::Real* dx = geomdata.CellSize();
  amrex::Real x = 0.;
  amrex::Real y = 0.;
  amrex::Real z = 0.;
  AMREX_D_TERM(x = prob_lo[0] + (i + 0.5) * dx[0];
               , y = prob_lo[1] + (j + 0.5) * dx[1];
               , z = prob_lo[2] + (k + 0.5) * dx[2];);

  amrex::Real u[3] = {0.0};
  amrex::Real xn = x - xshift;
  amrex::Real invSmooth = 1. / prob_parm.velSmooth;
  amrex::Real jetLen = prob_parm.L_jet / 2.;
  amrex::Real func =
    0.5 * (tanh((xn + jetLen) * invSmooth) - tanh((xn - jetLen) * invSmooth));
  u[1] = func * prob_parm.v0;

  const amrex::Real p = prob_parm.p0;
  amrex::Real rho, eint;
  amrex::Real massfrac[NUM_SPECIES] = {0.0};
  massfrac[O2_ID] = prob_parm.Y_O2;
  massfrac[N2_ID] = prob_parm.Y_N2;
  auto eos = pele::physics::PhysicsType::eos();
  eos.PYT2RE(p, massfrac, prob_parm.T0, rho, eint);

  // Set the state
  state(i, j, k, URHO) = rho;
  state(i, j, k, UMX) = rho * u[0];
  state(i, j, k, UMY) = rho * u[1];
  state(i, j, k, UMZ) = rho * u[2];
  state(i, j, k, UEINT) = rho * eint;
  state(i, j, k, UEDEN) =
    rho * (eint + 0.5 * (u[0] * u[0] + u[1] * u[1] + u[2] * u[2]));
  state(i, j, k, UTEMP) = prob_parm.T0;
  for (int n = 0; n < NUM_SPECIES; n++)
    state(i, j, k, UFS + n) = rho * massfrac[n];
}

AMREX_GPU_DEVICE
AMREX_FORCE_INLINE
void
bcnormal(
  const amrex::Real x[AMREX_SPACEDIM],
  const amrex::Real s_int[NVAR],
  amrex::Real s_ext[NVAR],
  const int idir,
  const int sgn,
  const amrex::Real time,
  amrex::GeometryData const& geomdata,
  ProbParmDevice const& prob_parm)
{
  for (int i = 0; i != NVAR; ++i)
    s_ext[i] = s_int[i];
}

struct MyProbTagStruct
{
  AMREX_GPU_HOST_DEVICE
  AMREX_FORCE_INLINE
  static void set_problem_tags(
    const int i,
    const int j,
    const int k,
    amrex::Array4<char> const& tag,
    amrex::Array4<amrex::Real const> const& field,
    char tagval,
    const amrex::GpuArray<amrex::Real, AMREX_SPACEDIM> dx,
    const amrex::GpuArray<amrex::Real, AMREX_SPACEDIM> prob_lo,
    const amrex::Real time,
    const int level) noexcept
  {
    // could do problem specific tagging here
  }
};

using ProblemTags = MyProbTagStruct;

struct MyProbDeriveStruct
{
  static void
  add(amrex::DeriveList& derive_lst, amrex::DescriptorList& desc_lst)
  {
    // Add derives as follows and define the derive function below:
    // derive_lst.add(
    //  "varname", amrex::IndexType::TheCellType(), 1, pc_varname,
    //  the_same_box);
    // derive_lst.addComponent("varname", desc_lst, State_Type, 0, NVAR);
  }

  static void pc_varname(
    const amrex::Box& bx,
    amrex::FArrayBox& derfab,
    int dcomp,
    int /*ncomp*/,
    const amrex::FArrayBox& datfab,
    const amrex::Geometry& geomdata,
    amrex::Real /*time*/,
    const int* /*bcrec*/,
    int level)
  {
    auto const dat = datfab.array();
    auto arr = derfab.array();
    amrex::ParallelFor(bx, [=] AMREX_GPU_DEVICE(int i, int j, int k) noexcept {
      // do something with arr
    });
  }
};

using ProblemDerives = MyProbDeriveStruct;

void pc_prob_close();

#endif
