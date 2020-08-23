#ifndef _PROB_PARM_H_
#define _PROB_PARM_H_

#include <AMReX_REAL.H>
#include <AMReX_GpuQualifiers.H>
#include <AMReX_IntVect.H>

namespace ProbParm {
extern AMREX_GPU_DEVICE_MANAGED amrex::Real mach;
extern AMREX_GPU_DEVICE_MANAGED amrex::Real p0;
extern AMREX_GPU_DEVICE_MANAGED amrex::Real T0;
extern AMREX_GPU_DEVICE_MANAGED amrex::Real v0;
extern AMREX_GPU_DEVICE_MANAGED amrex::Real Y_O2;
extern AMREX_GPU_DEVICE_MANAGED amrex::Real Y_N2;
extern AMREX_GPU_DEVICE_MANAGED amrex::IntVect partNum;
extern AMREX_GPU_DEVICE_MANAGED amrex::Real partTemp;
extern AMREX_GPU_DEVICE_MANAGED amrex::Real partRho;
extern AMREX_GPU_DEVICE_MANAGED amrex::Real partDia;
extern AMREX_GPU_DEVICE_MANAGED amrex::RealVect partVel;
} // namespace ProbParm

#endif