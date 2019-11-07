
#ifndef _SootModel_H
#define _SootModel_H

// Standard libray includes
#include <array>

// AMReX include statements
#include <AMReX_REAL.H>
#include <AMReX_Box.H>
#include <AMReX_Vector.H>
#include <AMReX_Derive.H>

// PeleC include statements

using namespace amrex;

// Assume first-order moments by default
#ifndef NUM_SOOT_MOMENTS
#define NUM_SOOT_MOMENTS 3
#endif

// Local gas species index, not the same as what comes from PelePhysics
// Use gasIndxtoPeleP to convert between
enum GasSpecIndx
  {
    // Hardcoded indices for species in m_specSootRefs
    indxH2 = 0,
    indxH,
    indxOH,
    indxH2O,
    indxCO,
    indxC2H2,
    indxO2,
    indxPAH, // PAH species
    numGasSpecs
  };

enum SootIndx
  {
    indxSootS = 0, // Soot-*
    indxSootH,     // Soot-H
    numSootSpecs
  };

// Moment orders for M_x,y
// TODO: Add more moments if possible
#if NUM_SOOT_MOMENTS == 3
  /// Volume moment orders
  constexpr Real MomOrderV[] = {0., 1., 0.};
  /// Surface areas moment orders
  constexpr Real MomOrderS[] = {0., 0., 1.};
#elif NUM_SOOT_MOMENTS == 6
  /// Volume moment orders
  constexpr Real MomOrderV[] = {0., 1., 0., 2., 1., 0.};
  /// Surface areas moment orders
  constexpr Real MomOrderS[] = {0., 0., 1., 0., 1., 2.};
#endif

class SootModel
{
public:
  //
  // Default constructor.
  //
  SootModel ();

  //
  // Destructor
  //
  ~SootModel () {};
  //
  // Define instance of class
  //
  void define ();

  //
  // Read soot parameters from input file (SootModel_setup.cpp)
  //
  void readSootParams ();

  //
  // Define member data (SootModel_setup.cpp)
  //
  void defineMemberData ();

  //
  // Define the derived variable list (SootModel_setup.cpp)
  //
  void addSootDerivePlotVars(DeriveList&           derive_lst,
			     const DescriptorList& desc_lst);

  //
  // Compute HMOM source term
  //
  void addSootSourceTerm (const Box&       sbox,
			  const FArrayBox& Qfab,
			  const FArrayBox& coeff_cc,
			  FArrayBox&       Sfab,
			  Real&            dt) const;

  /***********************************************************************
    Moment source term functions (SootModel.cpp)
  ***********************************************************************/
  //
  // Nucleation
  //
  void nucleationMomSrc (const Real&   betaNucl,
			 const Real&   dimerConc,
			 Vector<Real>& mom_src) const;

  //
  // Condensation
  //
  void condensationMomSrc (const Real&         colConst,
			   const Real&         dimerConc,
			   const Vector<Real>& momFV,
			   Vector<Real>&       mom_src) const;

  //
  // Coagulation
  //
  void coagulationMomSrc (const Real&         colConst,
			  const Real&         T,
			  const Real&         mu,
			  const Real&         rho,
			  const Real&         molMass,
			  const Vector<Real>& momFV,
			  Vector<Real>&       mom_src) const;

  //
  // Compute the dimerization rate
  //
  Real dimerization (const Real&         convT,
		     const Real&         betaNucl,
		     const Real&         betaDimer,
		     const Real&         xi_PAH,
		     const Vector<Real>& momFV) const;

  //
  // Compute the surface growth source terms
  //
  void surfaceGrowthMomSrc (const Real&         k_sg,
			    const Vector<Real>& momFV,
			    Vector<Real>&       mom_src) const;

  //
  // Compute oxidation and fragmentation source term
  //
  void oxidFragMomSrc (const Real&         k_ox,
		       const Real&         k_o2,
		       const Vector<Real>& momFV,
		       Vector<Real>&       mom_src) const;

  /***********************************************************************
    Reaction functions (SootModel_react.cpp)
  ***********************************************************************/
  //
  // Compute surface and gas phase chemistry
  //
  void chemicalSrc (const Real&         T,
		    const Vector<Real>& xi_n,
		    const Vector<Real>& moments,
		    const Vector<Real>& momFV,
		    Real&               k_sg,
		    Real&               k_ox,
		    Real&               k_o2,
		    Vector<Real>&       omega_src,
		    Real&               rho_src) const;

  //
  // Initialize reaction and species data
  //
  void initializeReactData();

  //
  // Fill surface reaction data
  //
  void fillReactionData();

  //
  // Compute quasi-steady state concentration of radical sites
  //
  Real computeRadSiteConc (const Vector<Real>& xi_n,
			   const Vector<Real>& k_fwd,
			   const Vector<Real>& k_bkwd) const;

  //
  // Compute surface growth and oxication rates
  //
  void computeSurfRates (const Vector<Real>& xi_n,
			 const Vector<Real>& w_fwd,
			 const Vector<Real>& w_bkwd,
			 const Real&         fSootStar,
			 Real&               k_sg,
			 Real&               k_ox,
			 Real&               k_o2) const;

  /***********************************************************************
    Moment interpolation functions (SootModel.cpp)
  ***********************************************************************/

  void computeFracMomVect (const Vector<Real>& moments,
			   Vector<Real>&       momFV) const;

  Real fracMom (const Real          volOrd,
		const Real          surfOrd,
		const Vector<Real>& momFV) const;

  Real fracMomLarge (const Real          volOrd,
		     const Real          surfOrd,
		     const Vector<Real>& momFV) const;

  // Collision coefficient for nucleation
  Real getBetaCond (const Real&         convT,
		    const Vector<Real>& momFV) const;

  // Free molecular coagulation functions
  Real psiSL (const Real          x,
	      const Real          y,
	      const Real          a,
	      const Real          b,
	      const Vector<Real>& momFV) const;

  Real psiLL (const Real          x,
	      const Real          y,
	      const Real          a,
	      const Real          b,
	      const Vector<Real>& momFV) const;

  Real FMCoagSL (const int           i,
		 const Vector<Real>& momFV) const;

  Real FMCoagLL (const int           i,
		 const Vector<Real>& momFV) const;

  // Continuum coagulation functions
  Real CNCoagSL (const int           i,
		 const Real&         lambda,
		 const Vector<Real>& momFV) const;

  Real CNCoagLL (const int           i,
		 const Real&         lambda,
		 const Vector<Real>& momFV) const;

  Real CNCoagSLFunc (int                 n[4],
		     const Real          x,
		     const Real          y,
		     const Real&         lambda,
		     const Vector<Real>& momFV) const;

  Real CNCoagLLFunc (const Real          x,
		     const Real          y,
		     const Real&         lambda,
		     const Vector<Real>& momFV) const;

  Real CNCoagLLFunc (const Real          x,
		     const Real          y,
		     const Real          x2,
		     const Real          y2,
		     const Real&         lambda,
		     const Vector<Real>& momFV) const;

  /***********************************************************************
    Inline functions
  ***********************************************************************/
  //
  // Convert from soot index to the PelePhysics species index
  //
  inline int gasIndxtoPeleP (const int gsIndx) const
  {
    return m_gasSpecRefs[gsIndx];
  }

  //
  // Convert moments from CGS to mol of C
  //
  static void convertCGStoMol (Vector<Real>& moments)
  {
    for (int i = 0; i != NUM_SOOT_MOMENTS + 1; ++i)
      {
	moments[i] /= MomUnitConv[i];
      }
  }

  //
  // Convert moments from mol of C to CGS
  //
  static void convertMoltoCGS (Vector<Real>& mom_src)
  {
    for (int i = 0; i != NUM_SOOT_MOMENTS + 1; ++i)
      {
	mom_src[i] *= MomUnitConv[i];
      }
  }

  //
  // Given an exponent numerator, return (dimer_vol)^(exp/6)
  //
  inline Real getDimerExp6 (const int exp) const
  {
    int indx = (exp + 3)/2;
    return m_dimerExp6[indx];
  }

  //
  // Given an exponent numerator, return (nucl_vol)^(exp/3)
  //
  inline Real getNuclExp3 (const int exp) const
  {
    int indx = exp + 3;
    return m_nuclVolExp3[indx];
  }

  //
  // Given an exponent numerator, return (nucl_vol)^(exp/6)
  //
  inline Real getNuclExp6 (const int exp) const
  {
    int indx = exp + 3;
    return m_nuclVolExp6[indx];
  }

  //
  // Return string of the names of each variable
  //
  inline std::string sootVariableName (const int sootIndx) const
  {
    BL_ASSERT(sootIndx >= 0 && sootIndx < NUM_SOOT_MOMENTS + 1);
    return m_sootVarName[sootIndx];
  }

  //
  // Initialize moment values to very small 
  //
  static void initialSmallMomVals (Vector<Real>& moments)
  {
    for (int i = 0; i != NUM_SOOT_MOMENTS; ++i)
      {
	moments[i] = 1.E-26*std::pow(40., 3.*(MomOrderV[i] + 2./3.*MomOrderS[i]));
      }
    moments[NUM_SOOT_MOMENTS] = 1.E-26;
    convertMoltoCGS(moments);
  }

  /***********************************************************************
    Member data
  ***********************************************************************/

public:

  /// Units for moments are cm^3(x + 2/3y)/cm^3
  /// Units for the weight of the delta function are 1/cm^3
  /// Soot density (g/cm^3)
  constexpr static Real m_SootDensity = 1.8;
  /// Surface density (mol of C)
  Real m_SootDensityC;
  /// Soot molar mass (g/mol)
  constexpr static Real m_SootMolarMass = 12.011150; // Carbon 
  /// Soot fractal dimension
  constexpr static Real m_SootDf = 1.8;
  /// Surface density of hydrogenate sites (1/cm^2)
  constexpr static Real m_SootChi = 1.7E15;
  /// Coefficients for fit to small surface area change
  constexpr static Real m_SootFitC = 2./3.;
  constexpr static Real m_SootFitE = -0.2043;
  // Increase in number of carbon atoms for surface reactions
  constexpr static Real m_dVol = 2.;

  /// Gas constant in erg K^-1 mol^-1
  Real Rgas;
  /// Avogadro's number in 1/mol
  // TODO: Should be pulling this value from PeleC/constants
  constexpr static Real avogadros = 6.02214076E23;

  /// Verbosity for soot model
  int m_sootVerbosity;
  /// Bool for checking if parameters have been read from input file
  bool m_readSootParams;
  /// Volume of smallest soot particles
  static Real m_V0;
  /// Surface area of smallest soot particles
  static Real m_S0;
  /// Moment unit conversion from cm^(x+2/3y)/cm^3 to mol of C
  static Real MomUnitConv[NUM_SOOT_MOMENTS + 1];

  /// Free molecular size exponents
  const Real m_SootAv;
  const Real m_SootAs;

  /// Molecular weight for gas species (g/mol)
  Vector<Real> m_gasMW;

  // TODO: Make these vectors in case multiple inception species are used
  /// Species index of PAH
  int m_PAHindx;
  /// Name of PAH used for inception
  std::string m_PAHname;

  /// Sticking coefficient of species used for dimerization
  Real m_gammaStick;
  /// Size of incepient soot particles (# of C)
  Real m_dimerVol;
  Real m_nuclVol;
  Real m_nuclSurf;

  // Name of soot variables to output (moments and weight of delta function)
  Vector<std::string> m_sootVarName;

  /***********************************************************************
    Reaction member data
  ***********************************************************************/

  /// Bool to ensure reaction data has been read and filled
  bool m_reactDataFilled;
  /// Number and names of species for surface reactions
  Vector<std::string> m_gasSpecNames;
  /// Species number in PelePhysics corresponding to gasSpecNames
  Vector<int> m_gasSpecRefs;
  
  /// Surface chemistry data (CGS)
  /// Number of reactions for surface chemistry
  int m_numSurfReacts;
  /// Forward pre-exponential rates
  Vector<Real> A_f;
  /// Forward temperature exponent
  Vector<Real> n_f;
  /// Forward activation energy/R
  Vector<Real> ER_f;
  /// Backward pre-exponential rates
  Vector<Real> A_b;
  /// Backward temperature exponent
  Vector<Real> n_b;
  /// Backward activation energy
  Vector<Real> ER_b;
  
  /// Gas phase reference values
  /// Vector of the number of reactants for each reaction
  Vector<int> rNum;
  /// Vector of the number of products for each reaction
  Vector<int> pNum;
  /// Vector of species index for forward reaction
  /// 3 is the maximum number of reactants per reaction
  Vector<std::array<int, 3> > nIndx_f;
  /// Vector of species index for backward reaction
  Vector<std::array<int, 3> > nIndx_b;
  /// Vector of stoichiometric coefficient for forward reactions
  Vector<std::array<Real, 3> > nu_f;
  /// Vector of stoichiometic coefficient for backward reactions
  Vector<std::array<Real, 3> > nu_b;

  /// Soot surface reaction reference indexing
  /// Forward and backward reaction soot indices
  // TODO: Assumes only 1 mole of soot is on each side
  Vector<int> sIndx_f;
  Vector<int> sIndx_b;

  /************************************************************************
    Factor member data (to reduce computational costs)
  ************************************************************************/  

  /// Frequently used constants
  /// Moment factors
  Real m_momFact[NUM_SOOT_MOMENTS];
  Real m_colFact;     // Pi*R/(2*A*rho_soot)
  Real m_colFact23;   // (M_soot/(A*rho_soot))^(2/3)
  Real m_colFact16;   // (M_soot/(A*rho_soot))^(1/6)  
  Real m_colFactPi23; // (6/pi)^(2/3)
  /// Factors for beta
  Real m_betaDimerFact;
  Real m_betaNuclFact;
  /// Store dimerVol^(n/6), where n = -3,-1,...,13
  Real m_dimerExp6[9];
  /// Store dimerVol^(n/3), where n = -3,-2,...,7
  Real m_nuclVolExp3[11];
  /// Store dimerVol^(n/6), where n = -3,-2,...,7
  Real m_nuclVolExp6[11];
  /// Condensation constant
  Real m_condFact;
  /// Coagulation constants
  Real m_lambdaCoagFact;
  Real m_ssfmCoagFact[NUM_SOOT_MOMENTS + 1];
  Real m_sscnCoagFact[NUM_SOOT_MOMENTS];
  /// Oxidation constants
  Real m_smallOxidFact[NUM_SOOT_MOMENTS + 1];
  /// Fragmentation constants
  Real m_fragFact[NUM_SOOT_MOMENTS];
};

#endif