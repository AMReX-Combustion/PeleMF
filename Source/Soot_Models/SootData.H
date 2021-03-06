
#ifndef _SOOTDATA_H_
#define _SOOTDATA_H_

#include "PelePhysics.H"
#include "Constants_Soot.H"

struct SootData
{
  SootConst sc;
  Real nuclVol;
  Real nuclSurf;
  Real condFact;
  Real lambdaCF;
  GpuArray<Real, NUM_SOOT_MOMENTS + 1> unitConv;
  GpuArray<int, NUM_SOOT_GS> refIndx;
  GpuArray<Real, 9> dime6;
  GpuArray<Real, 11> nve3;
  GpuArray<Real, 11> nve6;
  GpuArray<Real, NUM_SOOT_MOMENTS> momFact;
  GpuArray<Real, NUM_SOOT_MOMENTS + 1> ssfmCF;
  GpuArray<Real, NUM_SOOT_MOMENTS> sscnCF;
  GpuArray<Real, NUM_SOOT_MOMENTS + 1> smallOF;
  GpuArray<Real, NUM_SOOT_MOMENTS> fragFact;

  // Convert moments from CGS/SI to mol of C
  AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE void
  convertToMol(Real moments[]) const
  {
    for (int i = 0; i < NUM_SOOT_MOMENTS + 1; ++i)
      moments[i] /= unitConv[i];
  }

  // Convert moments from mol of C to CGS/SI
  AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE void
  convertFromMol(Real mom_src[]) const
  {
    for (int i = 0; i < NUM_SOOT_MOMENTS + 1; ++i)
      mom_src[i] *= unitConv[i];
  }

  // Convert moments from mol of C to CGS/SI
  AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE void
  convertFromMol(Real mom_src[], Real moments[]) const
  {
    for (int i = 0; i < NUM_SOOT_MOMENTS + 1; ++i) {
      mom_src[i] *= unitConv[i];
      moments[i] *= unitConv[i];
    }
  }

  // Initialize moment values to very small
  AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE void
  initialSmallMomVals(Real moments[]) const
  {
    for (int i = 0; i < NUM_SOOT_MOMENTS; ++i) {
      moments[i] =
        sc.smallWeight *
        std::pow(nuclVol, 3. * sc.MomOrderV[i] + 2. * sc.MomOrderS[i]);
    }
    moments[NUM_SOOT_MOMENTS] = sc.smallWeight;
    convertFromMol(moments);
  }

  // Given an exponent numerator, return dimer_vol^(exp/6)
  AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE Real
  getDimerExp6(const int exp) const
  {
    int indx = (exp + 3) / 2;
    return dime6[indx];
  }

  // Given an exponent numerator, return (nucl_vol)^(exp/3)
  AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE Real getNuclExp3(const int exp) const
  {
    int indx = exp + 3;
    return nve3[indx];
  }

  // Given an exponent numerator, return (nucl_vol)^(exp/6)
  AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE Real getNuclExp6(const int exp) const
  {
    int indx = exp + 3;
    return nve6[indx];
  }

  /********************************************************************
    Moment source terms
  ********************************************************************/

  // Nucleation source term
  AMREX_GPU_DEVICE AMREX_FORCE_INLINE void nucleationMomSrc(
    const Real& betaNucl, const Real& dimerConc, Real mom_src[]) const
  {
    const Real dimerConc2 = dimerConc * dimerConc;
    for (int i = 0; i < NUM_SOOT_MOMENTS; ++i)
      mom_src[i] += 0.5 * betaNucl * dimerConc2 * momFact[i];
    mom_src[NUM_SOOT_MOMENTS] += 0.5 * betaNucl * dimerConc2;
  }

  // Condensation source term
  AMREX_GPU_DEVICE AMREX_FORCE_INLINE void condensationMomSrc(
    const Real& colConst,
    const Real& dimerConc,
    const Real momFV[],
    Real mom_src[]) const
  {
    /** Compute condensation source values
        @param colConst Constant for free molecular collisions
        @param dimerConc Concentration of dimer
        @param momFV Vector of factors used in moment interpolation
        @param mom_src Moment source values
    */
    Real weightDelta = momFV[NUM_SOOT_MOMENTS];
    for (int i = 0; i < NUM_SOOT_MOMENTS; ++i) {
      const Real momV = sc.MomOrderV[i];
      const Real momS = sc.MomOrderS[i];
      const Real vv1 = momV + 2. * sc.SootAv;
      const Real vs1 = momS + 2. * sc.SootAs;
      const Real vv2 = momV + sc.SootAv;
      const Real vs2 = momS + sc.SootAs;
      Real volTerm = fracMom(vv1 - 1., vs1, momFV) * getDimerExp6(3) +
                     2. * fracMom(vv2 - 1., vs2, momFV) * getDimerExp6(5) +
                     fracMom(momV - 1., momS, momFV) * getDimerExp6(7) +
                     0.5 * fracMom(vv1 - 2., vs1, momFV) * getDimerExp6(9) +
                     fracMom(vv2 - 2., vs2, momFV) * getDimerExp6(11) +
                     0.5 * fracMom(momV - 2., momS, momFV) * getDimerExp6(13);
      const Real ss3 = momS + 3. * sc.SootFitE;
      const Real sv3 = momV - 2. * sc.SootFitE;
      const Real ss2 = ss3 + sc.SootAs;
      const Real sv2 = sv3 + sc.SootAv;
      const Real ss1 = ss3 + 2. * sc.SootAs;
      const Real sv1 = sv3 + 2. * sc.SootAv;
      const Real surfTerm =
        fracMom(sv1 - 1., ss1, momFV) * getDimerExp6(3) +
        2. * fracMom(sv2 - 1., ss2, momFV) * getDimerExp6(5) +
        fracMom(sv3 - 1., ss3, momFV) * getDimerExp6(7) +
        0.5 * fracMom(sv1 - 2., ss1, momFV) * getDimerExp6(9) +
        fracMom(sv2 - 2., ss2, momFV) * getDimerExp6(11) +
        0.5 * fracMom(sv3 - 2., ss3, momFV) * getDimerExp6(13);
      mom_src[i] +=
        colConst * (momV * volTerm + sc.SootFitC * momS * surfTerm) * dimerConc;
    }
    // Source for the weight of the delta function
    mom_src[NUM_SOOT_MOMENTS] -= condFact * colConst * dimerConc * weightDelta;
  }

  // Surface growth source term
  AMREX_GPU_DEVICE AMREX_FORCE_INLINE void surfaceGrowthMomSrc(
    const Real& k_sg, const Real momFV[], Real mom_src[]) const
  {
    // Index of the weight of the delta function
    const int dwIndx = NUM_SOOT_MOMENTS;
    const Real weightDelta = momFV[dwIndx];
    const Real factor = sc.SootDensityC * sc.dVol * k_sg;
    for (int i = 0; i < NUM_SOOT_MOMENTS; ++i) {
      Real fact1 = fracMom(sc.MomOrderV[i] - 1., sc.MomOrderS[i] + 1., momFV);
      Real fact2 = fracMom(
        sc.MomOrderV[i] - 1. - 2. * sc.SootFitE,
        sc.MomOrderS[i] + 1. + 3. * sc.SootFitE, momFV);
      mom_src[i] +=
        (sc.MomOrderV[i] * fact1 + sc.MomOrderS[i] * sc.SootFitC * fact2) *
        factor;
    }
    // Weight of the delta function
    mom_src[dwIndx] -= nuclSurf * k_sg * sc.SootDensityC * weightDelta;
  }

  // Oxidation and fragmentation source terms
  AMREX_GPU_DEVICE AMREX_FORCE_INLINE void oxidFragMomSrc(
    const Real& k_ox,
    const Real& k_o2,
    const Real momFV[],
    Real mom_src[]) const
  {
    // Index of the weight of the delta function
    const int dwIndx = NUM_SOOT_MOMENTS;
    const Real weightDelta = momFV[dwIndx];
    const Real factOx = k_ox * sc.dVol * sc.SootDensityC;
    const Real factO2 = 2. * k_o2 * sc.dVol * sc.SootDensityC;
    for (int i = 0; i < NUM_SOOT_MOMENTS; ++i) {
      // Oxidation of the small particles
      Real small = -factOx * smallOF[i] * weightDelta;
      // Oxidation of the larger particles
      Real fracLarge =
        fracMomLarge(sc.MomOrderV[i] - 1., sc.MomOrderS[i] + 1., momFV);
      Real large =
        -factOx * (sc.MomOrderV[i] + 2. / 3. * sc.MomOrderS[i]) * fracLarge;
      // Add oxidation source
      mom_src[i] += large + small;
      // Add fragmentation source
      mom_src[i] += fragFact[i] * factO2 * fracLarge;
    }
    Real fracLarge = fracMomLarge(-1., 1., momFV);
    Real small = -factOx * smallOF[dwIndx] * weightDelta;
    Real inter =
      nuclVol / (fracMomLarge(1., 0., momFV) / fracMomLarge(0., 0., momFV));
    Real large = factOx * inter * fracLarge;
    // Add oxidation source for weight of delta function
    mom_src[dwIndx] += (small + large);
    // Add fragmentation source for weight of delta function
    mom_src[dwIndx] += inter * factO2 * fracLarge;
  }

  // Return the dimer concentration
  AMREX_GPU_DEVICE AMREX_FORCE_INLINE Real dimerization(
    const Real& convT,
    const Real& betaNucl,
    const Real& dimerRate,
    const Real momFV[]) const
  {
    // Collision coefficient for condensation
    const Real betaCond = getBetaCond(convT, momFV);
    // Using the following quadratic equation:
    // betaNucl*[DIMER]^2 + betaCond*[DIMER] - dimerRate = 0
    // compute the [DIMER] using the quadratic formula
    // x = -b + sqrt(b^2 - 4ac)/(2a)
    const Real delta = betaCond * betaCond + 4. * betaNucl * dimerRate;
    return (std::sqrt(delta) - betaCond) / (2. * betaNucl);
  }

  // Clip moment values
  AMREX_GPU_DEVICE AMREX_FORCE_INLINE void clipMoments(Real moments[]) const
  {
    Real weightDelta = moments[NUM_SOOT_MOMENTS];
    const Real tolV = sc.smallWeight * nuclVol;
    const Real tolS = sc.smallWeight * nuclSurf;
    // Check for globally small moments
    if (moments[0] < sc.smallWeight || moments[1] < tolV || moments[2] < tolS) {
      moments[1] = amrex::max(moments[1], tolV);
      moments[0] = moments[1] / nuclVol;
      moments[2] = nuclSurf * moments[0];
      weightDelta = moments[0];
    }
    // Check for size of second mode
    if (
      moments[1] < nuclVol * moments[0] || moments[2] < nuclSurf * moments[0]) {
      moments[0] = moments[1] / nuclVol;
      moments[2] = moments[0] * nuclSurf;
    }
#if NUM_SOOT_MOMENTS == 6
    // Check for (co)variance of second mode
    moments[3] = amrex::max(moments[3], moments[1] * moments[1] / moments[0]);
    moments[4] = amrex::max(moments[4], moments[1] * moments[2] / moments[0]);
    moments[5] = amrex::max(moments[5], moments[2] * moments[2] / moments[0]);
#endif
    if (weightDelta < sc.smallWeight) {
      for (int i = 0; i < NUM_SOOT_MOMENTS; ++i)
        moments[i] += (sc.smallWeight - weightDelta) *
                      std::pow(nuclVol, sc.MomOrderV[i]) *
                      std::pow(nuclSurf, sc.MomOrderS[i]);
      weightDelta = sc.smallWeight;
    }
    if (weightDelta > moments[0])
      weightDelta = moments[0];
    moments[NUM_SOOT_MOMENTS] = weightDelta;
  }

  // Convert from CGS/SI to mol, clip moments, and convert back
  AMREX_GPU_DEVICE AMREX_FORCE_INLINE void momConvClipConv(Real mom[]) const
  {
    convertToMol(mom);
    clipMoments(mom);
    convertFromMol(mom);
  }

  // Compute the coagulation source term
  AMREX_GPU_DEVICE AMREX_FORCE_INLINE void coagulationMomSrc(
    const Real& colConst,
    const Real& T,
    const Real& mu,
    const Real& rho,
    const Real& molMass,
    const Real momFV[],
    Real mom_src[]) const
  {
    // Index of the weight of the delta function
    const int dwIndx = NUM_SOOT_MOMENTS;
    // Free molecular collision coefficient with van der Waals enhancements
    const Real C_fm = 2.2 * colConst;
    // Continuum collision coefficient
    const Real C_cn = 8. * pele::physics::Constants::RU * T / (3. * mu);
    // Mean free path for finite nudsen number correction in continuum regimes
    const Real lambda =
      3. * mu / rho *
      std::sqrt(M_PI * molMass / (8. * pele::physics::Constants::RU * T)) *
      lambdaCF;
    Real weightDelta2 = std::pow(momFV[dwIndx], 2);
    for (int i = 0; i < NUM_SOOT_MOMENTS; ++i) {
      // Collisions between two first mode particles
      // Collision model: pure coalescence
      // S_(0+0) = (144*pi)^(1/3)*V0^(2/3)
      // Free molecular regime
      Real ss_fm = C_fm * ssfmCF[i] * weightDelta2;
      // Continuum regime
      Real ss_cn = 4. * C_cn * (1. + 1.257 * lambda * getNuclExp3(-1)) *
                   sscnCF[i] * weightDelta2;
      Real prodss = ss_fm * ss_cn;
      // Harmonic mean for transitional regime
      Real ss = (std::abs(prodss) < 1.E-50) ? 0. : prodss / (ss_fm + ss_cn);

      // Collision between a particle in each mode
      // Collision model: "Splashing"
      // S_(i+0) = S_i + delta S
      // delta S = S*delta V / V *2/3*n_p^(-0.2043)
      // delta V = 2*W_C/rho_soot
      // Free molecular regime
      Real sl_fm = C_fm * FMCoagSL(i, momFV);
      // Continuum regime
      Real sl_cn = C_cn * CNCoagSL(i, lambda, momFV);
      Real prodsl = sl_fm * sl_cn;
      // Harmonic mean for transitional regime
      Real sl = (std::abs(prodsl) < 1.E-50) ? 0. : prodsl / (sl_fm + sl_cn);
      // Collision between two second mode particles
      // Collision model: Pure aggregation
      // S_(i+j) = S_i + S_j
      // Free molecular regime
      Real ll_fm = C_fm * FMCoagLL(i, momFV);
      // Continuum regime
      Real ll_cn = C_cn * CNCoagLL(i, lambda, momFV);
      Real prodll = ll_fm * ll_cn;
      // Harmonic mean for transitional regime
      Real ll = (std::abs(prodll) < 1.E-50) ? 0. : prodll / (ll_fm + ll_cn);
      mom_src[i] += (ss + sl + ll);
    }
    // Free molecular regime
    Real ss_fm = -C_fm * weightDelta2 * ssfmCF[dwIndx];
    // Continuum regime
    Real ss_cn =
      -4. * C_cn * (1. + 1.257 * lambda * getNuclExp3(-1)) * weightDelta2;
    // Harmonic mean for transitional regime
    Real prodss = ss_fm * ss_cn;
    Real ss = (std::abs(prodss) < 1.E-50) ? 0. : prodss / (ss_fm + ss_cn);
    // Free molecular regime
    Real sl_fm = C_fm * FMCoagSL(dwIndx, momFV);
    // Continuum regime
    Real sl_cn = C_cn * CNCoagSL(dwIndx, lambda, momFV);
    // Harmonic mean for transitional regime
    Real prodsl = sl_fm * sl_cn;
    Real sl = (std::abs(prodsl) < 1.E-50) ? 0. : prodsl / (sl_fm + sl_cn);
    mom_src[dwIndx] += (ss + sl);
  }

  /*********************************************************************
    Moment interpolation functions
  *********************************************************************/

  // Compute the moment interpolation array
  /*
    momFV contains factors for interpolating the moments
    It is ordered as the following
    momFV[0-NUM_SOOT_MOMENTS-1] - Corresponding factor for moment interpolation
    momFV[NUM_SOOT_MOMENTS] - Weight of the delta function
    momFV[NUM_SOOT_MOMENTS+1] - modeCoef
    modeCoef signifies the number of modes to be used
    If the moments are effectively zero, modeCoef = 0. and only 1 mode is used
    Otherwise, modeCoef = 1. and both modes are used
  */
  AMREX_GPU_DEVICE AMREX_FORCE_INLINE void
  computeFracMomVect(const Real moments[], Real momFV[]) const
  {
    // See above for description of modeCoef
    Real modeCoef;
    // Copy over the weight of the delta function
    momFV[NUM_SOOT_MOMENTS] = moments[NUM_SOOT_MOMENTS];
#if NUM_SOOT_MOMENTS == 3
    const Real M00 = moments[0] - momFact[0] * moments[3];
    const Real M10 = moments[1] - momFact[1] * moments[3];
    const Real M01 = moments[2] - momFact[2] * moments[3];
    // If moments are effectively zero, only use one mode
    if (M00 < 1.E-36 || M10 < 1.E-36 || M01 < 1.E-36) {
      // Contribution from only one mode
      momFV[0] = moments[0];
      momFV[1] = moments[1];
      momFV[2] = moments[2];
      modeCoef = 0.;
    } else {
      // Contribution from both modes
      momFV[0] = M00;
      momFV[1] = M10;
      momFV[2] = M01;
      modeCoef = 1.;
    }
#elif NUM_SOOT_MOMENTS == 6
    const Real M00 = moments[0] - momFact[0] * moments[6];
    const Real M10 = moments[1] - momFact[1] * moments[6];
    const Real M01 = moments[2] - momFact[2] * moments[6];
    const Real M20 = moments[3] - momFact[3] * moments[6];
    const Real M11 = moments[4] - momFact[4] * moments[6];
    const Real M02 = moments[5] - momFact[5] * moments[6];
    Real minMom = amrex::min(M00, amrex::min(M10, M01));
    minMom = amrex::min(minMom, amrex::min(M20, amrex::min(M11, M02)));
    // If moments are effectively zero, only use one mode
    if (minMom < 1.E-36) {
      const Real c1 = std::pow(moments[0], -1.5);
      const Real c2 = std::pow(moments[0], 0.5);
      momFV[0] = moments[0];
      momFV[1] = std::pow(moments[1], 2.) * c1 * std::pow(moments[3], -0.5);
      momFV[2] = std::pow(moments[2], 2.) * c1 * std::pow(moments[5], -0.5);
      momFV[3] = std::pow(moments[3], 0.5) * c2 * std::pow(moments[1], -1.);
      momFV[4] = moments[4] * moments[0] / (moments[1] * moments[2]);
      momFV[5] = std::pow(moments[5], 0.5) * c2 * std::pow(moments[2], -1.);
      modeCoef = 0.;
    } else {
      const Real c1 = std::pow(M00, -1.5);
      const Real c2 = std::pow(M00, 0.5);
      momFV[0] = M00;
      momFV[1] = std::pow(M10, 2.) * c1 * std::pow(M20, -0.5);
      momFV[2] = std::pow(M01, 2.) * c1 * std::pow(M02, -0.5);
      momFV[3] = std::pow(M20, 0.5) * c2 * std::pow(M10, -1.);
      momFV[4] = M11 * M00 / (M10 * M01);
      momFV[5] = std::pow(M02, 0.5) * c2 * std::pow(M01, -1.);
      modeCoef = 1.;
    }
#endif
    momFV[NUM_SOOT_MOMENTS + 1] = modeCoef;
  }

  // Moment interpolation
  AMREX_GPU_DEVICE AMREX_FORCE_INLINE Real
  fracMomLarge(const Real volOrd, const Real surfOrd, const Real momFV[]) const
  {
    // Weight of the delta function
    Real dwVal = momFV[NUM_SOOT_MOMENTS];
    Real factor = std::pow(nuclVol, volOrd) * std::pow(nuclSurf, surfOrd);
    // Remove the contribution from the first mode
    Real outMom = fracMom(volOrd, surfOrd, momFV) - dwVal * factor;
    // If the moment is negative, return a small (consistent) value
    if (outMom <= 0. || outMom != outMom)
      return factor * 1.E-66;
    return outMom;
  }

  // Moment interpolation
  AMREX_GPU_DEVICE AMREX_FORCE_INLINE Real
  fracMom(const Real volOrd, const Real surfOrd, const Real momFV[]) const
  {
    // If modeCoef = 0.; only first mode is used
    // If modeCoef = 1.; both modes are used
    const Real modeCoef = momFV[NUM_SOOT_MOMENTS + 1];
    Real bothPFact = 0.;
    if (modeCoef > 0.)
      bothPFact = momFV[NUM_SOOT_MOMENTS] * std::pow(nuclVol, volOrd) *
                  std::pow(nuclSurf, surfOrd);
#if NUM_SOOT_MOMENTS == 3
    Real peak = std::pow(momFV[0], 1. - volOrd - surfOrd) *
                std::pow(momFV[1], volOrd) * std::pow(momFV[2], surfOrd);
    return bothPFact + peak;
#elif NUM_SOOT_MOMENTS == 6
    Real prod = momFV[0];
    prod *= std::pow(momFV[1], volOrd);
    prod *= std::pow(momFV[2], surfOrd);
    prod *= std::pow(momFV[3], volOrd * volOrd);
    prod *= std::pow(momFV[4], volOrd * surfOrd);
    prod *= std::pow(momFV[5], surfOrd * surfOrd);
    return bothPFact + prod;
#endif
  }

  // Interpolation for the reduced mass term (square root of sum) in the
  // collision kernel for collision between a particle in each mode
  // Only two grid functions used for all moments
  // Limited sensitivity to increasing the number of grid functions
  AMREX_GPU_DEVICE AMREX_FORCE_INLINE Real psiSL(
    const Real x,
    const Real y,
    const Real a,
    const Real b,
    const Real momFV[]) const
  {
    const Real weightDelta = momFV[NUM_SOOT_MOMENTS];
    const Real factor = weightDelta * std::pow(nuclVol, a + 2. / 3. * b);
    Real VF[3] = {2. * sc.SootAv + x, sc.SootAv + x, x};
    Real SF[3] = {2. * sc.SootAs + y, sc.SootAs + y, y};
    const Real FML_1 = fracMomLarge(VF[0] - 0.5, SF[0], momFV);
    const Real FML_2 = fracMomLarge(VF[1] - 0.5, SF[1], momFV);
    const Real FML_3 = fracMomLarge(VF[2] - 0.5, SF[2], momFV);
    // nuclVolExp6[i] = nuclVol^(2*i - 3)/6
    Real psi1 =
      factor * (getNuclExp6(-3) * FML_1 + 2. * getNuclExp6(-1) * FML_2 +
                getNuclExp6(1) * FML_3);
    const Real FPL_1 = fracMomLarge(VF[0] + 0.5, SF[0], momFV);
    const Real FPL_2 = fracMomLarge(VF[1] + 0.5, SF[1], momFV);
    const Real FPL_3 = fracMomLarge(VF[2] + 0.5, SF[2], momFV);
    Real psi2_1 =
      factor * (getNuclExp6(-3) * FPL_1 + 2. * getNuclExp6(-1) * FPL_2 +
                getNuclExp6(1) * FPL_3);
    Real psi2_2 =
      factor * (getNuclExp6(3) * FML_1 + 2. * getNuclExp6(5) * FML_2 +
                getNuclExp6(7) * FML_3);
    return std::sqrt(psi1 * (psi2_1 + psi2_2));
  }

  // Interpolation for the reduced mass term (square root of sum) in the
  // collision kernel for collision between two particles in the second mode
  // Only two grid functions used for all moments
  // Limited sensitivity to increasing the number of grid functions
  AMREX_GPU_DEVICE AMREX_FORCE_INLINE Real psiLL(
    const Real x,
    const Real y,
    const Real a,
    const Real b,
    const Real momFV[]) const
  {
    Real VF_xy[3] = {2. * sc.SootAv + x, sc.SootAv + x, x};
    Real SF_xy[3] = {2. * sc.SootAs + y, sc.SootAs + y, y};
    Real VF_ab[3] = {a, sc.SootAv + a, 2. * sc.SootAv + a};
    Real SF_ab[3] = {b, sc.SootAs + b, 2. * sc.SootAs + b};
    Real xy_M[3] = {
      fracMomLarge(VF_xy[0] - 0.5, SF_xy[0], momFV),
      fracMomLarge(VF_xy[1] - 0.5, SF_xy[1], momFV),
      fracMomLarge(VF_xy[2] - 0.5, SF_xy[2], momFV)};
    Real xy_P[3] = {
      fracMomLarge(VF_xy[0] + 0.5, SF_xy[0], momFV),
      fracMomLarge(VF_xy[1] + 0.5, SF_xy[1], momFV),
      fracMomLarge(VF_xy[2] + 0.5, SF_xy[2], momFV)};
    Real ab_M[3] = {
      fracMomLarge(VF_ab[0] - 0.5, SF_ab[0], momFV),
      fracMomLarge(VF_ab[1] - 0.5, SF_ab[1], momFV),
      fracMomLarge(VF_ab[2] - 0.5, SF_ab[2], momFV)};
    Real ab_P[3] = {
      fracMomLarge(VF_ab[0] + 0.5, SF_ab[0], momFV),
      fracMomLarge(VF_ab[1] + 0.5, SF_ab[1], momFV),
      fracMomLarge(VF_ab[2] + 0.5, SF_ab[2], momFV)};
    Real psi1 = xy_M[0] * ab_M[0] + 2. * xy_M[1] * ab_M[1] + xy_M[2] * ab_M[2];
    Real psi2_1 =
      xy_P[0] * ab_M[0] + 2. * xy_P[1] * ab_M[1] + xy_P[2] * ab_M[2];
    Real psi2_2 =
      xy_M[0] * ab_P[0] + 2. * xy_M[1] * ab_P[1] + xy_M[2] * ab_P[2];
    return std::sqrt(psi1 * (psi2_1 + psi2_2));
  }

  // Free molecular coagulation source term
  // Small-Large: "Splashing"
  // -Generalized grid function follows terms
  AMREX_GPU_DEVICE AMREX_FORCE_INLINE Real
  FMCoagSL(const int i, const Real momFV[]) const
  {
    // Weight of delta function N0 and M00
    if (i == NUM_SOOT_MOMENTS || i == 0) {
      return -psiSL(0., 0., 0., 0., momFV);
    }
    const Real fact1 = -2. * sc.SootFitE;
    const Real fact2 = 3. * sc.SootFitE;
    switch (i) {
    case 1: // M10
      return 0.;
    case 2: // M01
      return sc.SootFitC * psiSL(fact1 - 1., fact2 + 1., 1., 0., momFV) -
             psiSL(0., 0., 0., 1., momFV);
    case 3: // M20
      return 2. * psiSL(1., 0., 1., 0., momFV);
    case 4: // M11
      return sc.SootFitC * psiSL(fact1, fact2 + 1., 1., 0., momFV) +
             psiSL(0., 1., 1., 0., momFV) +
             sc.SootFitC * psiSL(fact1 - 1., fact2 + 1., 2., 0., momFV) -
             psiSL(0., 0., 1., 1., momFV);
    case 5: // M02
      return 2. * sc.SootFitC * psiSL(fact1 - 1., fact2 + 2., 1., 0., momFV) +
             sc.SootFitC * sc.SootFitC *
               psiSL(2. * fact1 - 2., -3. * fact1 + 2., 2., 0., momFV) -
             psiSL(0., 0., 0., 2., momFV);
    default:
      Abort("SootModel::FMCoagSL: Moment not contained in number of moments!");
    }
    return 0.;
  }

  // Free molecular coagulation source term
  // Large-Large: Pure aggregation
  // -Generalized grid function follows terms
  AMREX_GPU_DEVICE AMREX_FORCE_INLINE Real
  FMCoagLL(const int i, const Real momFV[]) const
  {
    switch (i) {
    case 0: // M00
      return -0.5 * psiLL(0., 0., 0., 0., momFV);
    case 1: // M10
      return 0.;
    case 2: // M01
      return 0.;
    case 3: // M20
      return psiLL(1., 0., 1., 0., momFV);
    case 4: // M11
      return psiLL(1., 0., 0., 1., momFV);
    case 5: // M02
      return psiLL(0., 1., 0., 1., momFV);
    default:
      Abort("SootModel::FMCoagLL: Moment not contained in number of moments!");
    }
    return 0.;
  }

  // Continuum coagulation source terms
  // Small-Large: "Splashing"
  // Large-Large: Pure aggregation
  AMREX_GPU_DEVICE AMREX_FORCE_INLINE Real
  CNCoagSL(const int i, const Real& lambda, const Real momFV[]) const
  {
    const Real weightDelta = momFV[NUM_SOOT_MOMENTS];
    // Mean free path for finite Knudsen number correction in continuum regime
    if (i == NUM_SOOT_MOMENTS || i == 0) { // N0 or M00
      int n[] = {0, 1, -1, -2};
      Real x = 0.;
      Real y = 0.;
      return -weightDelta * CNCoagSLFunc(n, x, y, lambda, momFV);
    }
    switch (i) {
    case 1: // M10
      return 0.;
    case 2: // M01
    {
      Real p1, p2;
      {
        int n[] = {3, 4, 2, 1};
        Real x = -2. * sc.SootFitE - 1.;
        Real y = 3. * sc.SootFitE + 1.;
        p1 = sc.SootFitC * CNCoagSLFunc(n, x, y, lambda, momFV);
      }
      {
        int n[] = {2, 3, 1, 0};
        p2 = -CNCoagSLFunc(n, 0., 0., lambda, momFV);
      }
      return weightDelta * (p1 + p2);
    }
    case 3: // M20
    {
      int n[] = {3, 4, 2, 1};
      return 2. * weightDelta * CNCoagSLFunc(n, 1., 0., lambda, momFV);
    }
    case 4: // M11
    {
      Real p1, p2, p3, p4;
      {
        int n[] = {3, 4, 2, 1};
        Real x = -2. * sc.SootFitE;
        Real y = 3. * sc.SootFitE + 1.;
        p1 = sc.SootFitC * CNCoagSLFunc(n, x, y, lambda, momFV);
      }
      {
        int n[] = {3, 4, 2, 1};
        p2 = CNCoagSLFunc(n, 0., 1., lambda, momFV);
      }
      {
        int n[] = {6, 7, 5, 4};
        Real x = -2. * sc.SootFitE - 1.;
        Real y = 3. * sc.SootFitE + 1.;
        p3 = sc.SootFitC * CNCoagSLFunc(n, x, y, lambda, momFV);
      }
      {
        int n[] = {5, 6, 4, 3};
        p4 = -CNCoagSLFunc(n, 0., 0., lambda, momFV);
      }
      return weightDelta * (p1 + p2 + p3 + p4);
    }
    case 5: // M02
    {
      Real p1, p2, p3;
      {
        int n[] = {3, 4, 2, 1};
        Real x = -2. * sc.SootFitE - 1.;
        Real y = 3. * sc.SootFitE + 2.;
        p1 = 2. * sc.SootFitC * CNCoagSLFunc(n, x, y, lambda, momFV);
      }
      {
        int n[] = {6, 7, 5, 4};
        Real x = -4. * sc.SootFitE - 2.;
        Real y = 6. * sc.SootFitE + 2.;
        p2 = sc.SootFitC * sc.SootFitC * CNCoagSLFunc(n, x, y, lambda, momFV);
      }
      {
        int n[] = {4, 5, 3, 2};
        p3 = -CNCoagSLFunc(n, 0., 0., lambda, momFV);
      }
      return 2. * weightDelta * (p1 + p2 + p3);
    }
    }
    return 0.;
  }

  AMREX_GPU_DEVICE AMREX_FORCE_INLINE Real CNCoagSLFunc(
    int n[4],
    const Real x,
    const Real y,
    const Real& lambda,
    const Real momFV[]) const
  {
    Real xy_1 = fracMomLarge(x, y, momFV);
    Real xy_2 = fracMomLarge(x - sc.SootAv, y - sc.SootAs, momFV);
    Real xy_3 = fracMomLarge(x + sc.SootAv, y + sc.SootAs, momFV);
    Real xy_4 = fracMomLarge(x - 2. * sc.SootAv, y - 2. * sc.SootAs, momFV);
    Real n_1 = getNuclExp3(n[0]);
    Real n_2 = getNuclExp3(n[1]);
    Real n_3 = getNuclExp3(n[2]);
    Real n_4 = getNuclExp3(n[3]);
    return 2. * xy_1 * n_1 + xy_2 * n_2 + xy_3 * n_3 +
           1.257 * lambda * (xy_1 * n_3 + xy_2 * n_1 + xy_3 * n_4 + xy_4 * n_2);
  }

  AMREX_GPU_DEVICE AMREX_FORCE_INLINE Real
  CNCoagLL(const int i, const Real& lambda, const Real momFV[]) const
  {
    switch (i) {
    case 0: // M00
      return -0.5 * CNCoagLLFunc(0., 0., lambda, momFV);
    case 1: // M10
      return 0.;
    case 2: // M01
      return 0.;
    case 3: // M20
      return CNCoagLLFunc(1., 0., lambda, momFV);
    case 4: // M11
      return CNCoagLLFunc(1., 0., 0., 1., lambda, momFV);
    case 5: // M02
      return CNCoagLLFunc(0., 1., lambda, momFV);
    }
    return 0.;
  }

  AMREX_GPU_DEVICE AMREX_FORCE_INLINE Real CNCoagLLFunc(
    const Real x, const Real y, const Real& lambda, const Real momFV[]) const
  {
    const Real stav = sc.SootAv;
    const Real stas = sc.SootAs;
    Real xy_1 = fracMomLarge(x, y, momFV);
    Real xy_2 = fracMomLarge(x - stav, y - stas, momFV);
    Real xy_3 = fracMomLarge(x + stav, y + stas, momFV);
    Real xy_4 = fracMomLarge(x - 2. * stav, y - 2. * stas, momFV);
    return 2. * xy_1 * xy_1 + xy_2 * xy_3 + xy_3 * xy_2 +
           1.257 * lambda *
             (xy_1 * xy_2 + xy_2 * xy_1 + xy_3 * xy_4 + xy_4 * xy_3);
  }

  AMREX_GPU_DEVICE AMREX_FORCE_INLINE Real CNCoagLLFunc(
    const Real x,
    const Real y,
    const Real a,
    const Real b,
    const Real& lambda,
    const Real momFV[]) const
  {
    const Real stav = sc.SootAv;
    const Real stas = sc.SootAs;
    Real xy_1 = fracMomLarge(x, y, momFV);
    Real xy_2 = fracMomLarge(x - stav, y - stas, momFV);
    Real xy_3 = fracMomLarge(x + stav, y + stas, momFV);
    Real xy_4 = fracMomLarge(x - 2. * stav, y - 2. * stas, momFV);
    Real ab_1 = fracMomLarge(a, b, momFV);
    Real ab_2 = fracMomLarge(a - stav, b - stas, momFV);
    Real ab_3 = fracMomLarge(a + stav, b + stas, momFV);
    Real ab_4 = fracMomLarge(a - 2. * stav, b - 2. * stas, momFV);
    return 2. * ab_1 * xy_1 + ab_2 * xy_3 + ab_3 * xy_2 +
           1.257 * lambda *
             (ab_1 * xy_2 + ab_2 * xy_1 + ab_3 * xy_4 + ab_4 * xy_3);
  }

  AMREX_GPU_DEVICE AMREX_FORCE_INLINE Real
  getBetaCond(const Real& convT, const Real momFV[]) const
  {
    // Collision frequency between two dimer in the free
    // molecular regime WITHOUT van der Waals enhancement
    // Units: 1/s
    const Real stav = sc.SootAv;
    const Real stas = sc.SootAs;
    const Real Cfm =
      sc.colFactPi23 * convT * sc.colFact16 * pele::physics::Constants::Avna;
    const Real SN =
      fracMom(2. * stav, 2. * stas, momFV) * getDimerExp6(-3) +
      2. * fracMom(stav, stas, momFV) * getDimerExp6(-1) +
      fracMom(0., 0., momFV) * getDimerExp6(1) +
      0.5 * fracMom(2. * stav - 1., 2. * stas, momFV) * getDimerExp6(3) +
      fracMom(stav - 1., stas, momFV) * getDimerExp6(5) +
      0.5 * fracMom(-1., 0., momFV) * getDimerExp6(7);
    return Cfm * SN;
  }
};

#endif
