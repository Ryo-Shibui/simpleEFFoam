# simpleEFFoam

`simpleEFFoam` is a steady-state OpenFOAM solver for electrohydrodynamic
(EHD) simulations. It is based on OpenFOAM's `simpleFoam`.

The solver calculates electric potential, electric field, and space charge
density, then applies the electric body force caused by the space charge and
electric field to the flow field. It can be used to simulate EHD phenomena such
as ion wind.

This solver supports **steady-state calculations only**. It does not support
transient simulations.

## What This Solver Does

The main fields are:

- `U`: velocity
- `p`: kinematic pressure
- `phiE`: electric potential
- `E`: electric field, calculated as `E = -grad(phiE)`
- `rho`: space charge density

In the default single-carrier mode, each SIMPLE iteration follows these steps:

1. Solve Poisson's equation for the electric potential `phiE`.
2. Calculate the electric field `E` from the electric potential gradient.
3. Solve the velocity and pressure fields with the SIMPLE algorithm, including
   the electric body force from the electric field and space charge density.
4. Solve the conservation equation for the space charge density `rho`, including
   drift and diffusion terms.

The optional bipolar mode solves separate positive and negative small-ion
number densities, then feeds the resulting space charge into Poisson's equation
and the fluid momentum equation.

Note that `rho` is the space charge density, not the fluid density. The fluid
density is specified as `rho0` in `constant/physicalProperties`.

## Requirements

- OpenFOAM v2406, or a compatible OpenFOAM.com version
- A case prepared for an incompressible steady-state calculation
- Initial and boundary conditions for `U`, `p`, and `phiE`, plus `rho` in
  single-carrier mode or `nP` and `nN` in bipolar mode

## Build

Load your OpenFOAM environment, then run `wmake` in this repository directory.

```bash
cd simpleEFFoam
wmake
```

After a successful build, the executable is created at:

```bash
$FOAM_USER_APPBIN/simpleEFFoam
```

## Case Setup

Set the application in `system/controlDict` as follows:

```foam
application     simpleEFFoam;
```

For the default single-carrier mode, supply the following fields at the start time:

- `U`
- `p`
- `phiE`
- `rho`

The solver calculates and writes `E` and `phiEF`. In single-carrier mode,
these fields can also be read from the selected start time if present.

The `constant/physicalProperties` dictionary should define at least the
following properties:

```foam
epsilon0    epsilon0 [ -1 -3 4 0 0 2 0 ] 8.8541878128e-12;
k           k        [ -1 0 2 0 0 1 0 ]   <ion_mobility>;
T           T        [ 0 0 0 1 0 0 0 ]     <temperature>;
rho0        rho0     [ 1 -3 0 0 0 0 0 ]    <fluid_density>;
nu          nu       [ 0 2 -1 0 0 0 0 ]    <kinematic_viscosity>;
```

Set appropriate discretization schemes, linear solvers, and relaxation factors
for `p`, `U`, `phiE`, and `rho` in `system/fvSchemes` and
`system/fvSolution`.

Run the solver from the case directory:

```bash
simpleEFFoam
```

## Bipolar Ion Transport

Add the following entry to the existing `SIMPLE` dictionary in
`system/fvSolution`:

```foam
SIMPLE
{
    ionCalculation true;
    // Keep the other SIMPLE controls for your case.
}
```

Omitting `ionCalculation` is equivalent to `false` and selects the
single-carrier equation. The solver reads the switch at startup. For
`true`, provide both `nP` and `nN` as `volScalarField` files at the
selected start time. Each is a number density in m^-3 with
`dimensions [0 -3 0 0 0 0 0]`. Set the initial and patch concentrations
separately for the two polarities. For an ion source represented by a boundary,
specify the corresponding concentrations on its patch. Where ion backflow
conditions use `inletOutlet`, select `phi phiP;` for `nP` and `phi phiN;`
for `nN` to use the direction of the combined gas and electrical flux.
A `zeroGradient` concentration sets the diffusive flux to zero. Electrical
drift can still carry ions across the patch.

Use the following entries in `constant/physicalProperties`. The numbers are
examples. Set them for the gas and ion populations in your case.

```foam
muP   [ -1 0 2 0 0 1 0 ] 1.33e-4;  // Positive-ion mobility, m^2/(V s)
muN   [ -1 0 2 0 0 1 0 ] 1.84e-4;  // Negative-ion mobility magnitude, m^2/(V s)
DP    [  0 2 -1 0 0 0 0 ] 3.42e-6; // Positive-ion diffusivity, m^2/s
DN    [  0 2 -1 0 0 0 0 ] 4.73e-6; // Negative-ion diffusivity, m^2/s
beta  [  0 3 -1 0 0 0 0 ] 1.60e-12; // Ion-ion recombination coefficient, m^3/s
```

Both mobilities and diffusivities must be positive. Set `beta` to zero to
disable recombination. Bipolar mode uses `muP` and `muN` in place of the
single-carrier `k` and calculates `rho = e*(nP - nN)` from the ion fields
instead of reading it.

Add these entries to `divSchemes` in `system/fvSchemes`:

```foam
div(phiP,nP) Gauss upwind;
div(phiN,nN) Gauss upwind;
```

Set up linear solvers for both ion equations in `system/fvSolution`:

```foam
solvers
{
    "(nP|nN)"
    {
        solver          PBiCGStab;
        preconditioner  DILU;
        tolerance       1e-10;
        relTol          0;
    }
    // Keep the solver entries for the other equations in your case.
}
```

Each ion equation accounts for gas advection, drift in its charge direction,
diffusion and the common recombination loss `beta*nP*nN`. The solver
upwinds the total face flux for each species and treats the recombination
coefficient implicitly for the species being solved. Each SIMPLE iteration
updates `rho`, solves Poisson's equation, and applies the resulting electric
body force to the flow. The bipolar electric face flux `phiEF` is computed
from the face-normal potential gradient. `nP`, `nN` and derived `rho` are
written to disk.

Start with equation relaxation factors of `0.25` for both `nP` and `nN`
in `system/fvSolution`. Check that both number densities stay non-negative.
When using automatic stopping, include both ion equations and `phiE` in
`residualControl` alongside the flow equations. This mode models the ion source through the
boundary conditions. It has no volumetric ion generation or CPM plate
charging model.

## Important Mesh Notes

Electric-field calculations in OpenFOAM are strongly affected by the mesh
topology. Around circular electrodes such as wires, an O-grid mesh should be
used so that the mesh follows the electrode shape.

If a circular electrode is meshed with a Cartesian-style mesh generator such as
`cartesianMesh`, the electric field and space charge distribution may become
non-physical, or the calculation may diverge.

## When `rho` Oscillates in Single-Carrier Mode

Depending on the calculation conditions, the space charge density `rho` may
oscillate.

This can happen when the electric field is strong enough that the space charge
effectively jumps over neighboring cells. This is similar in spirit to why
transient flow simulations control the time step so that the Courant number
remains below 1.

If this oscillation occurs, reducing the relaxation factor for `rho` in
`system/fvSolution` to `0.1` or smaller often improves stability and accuracy.
However, convergence will become slower.

Example:

```foam
relaxationFactors
{
    equations
    {
        U       0.7;
        p       0.3;
        phiE    0.3;
        rho     0.1;
    }
}
```

## Limitations

- Only steady-state calculations are supported.
- Results are highly sensitive to mesh quality, especially the mesh topology
  around electrodes.
- Supply physically appropriate boundary conditions for `phiE` and the
  transported charge or ion fields.

## License

This solver is derived from OpenFOAM solver code. The source files retain the
OpenFOAM GPL license headers.
