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

In each solution loop, the solver roughly performs the following steps:

1. Solve Poisson's equation for the electric potential `phiE`.
2. Calculate the electric field `E` from the electric potential gradient.
3. Solve the velocity and pressure fields with the SIMPLE algorithm, including
   the electric body force from the electric field and space charge density.
4. Solve the conservation equation for the space charge density `rho`, including
   drift and diffusion terms.

Note that `rho` is the space charge density, not the fluid density. The fluid
density is specified as `rho0` in `constant/physicalProperties`.

## Requirements

- OpenFOAM v2406, or a compatible OpenFOAM.com version
- A case prepared for an incompressible steady-state calculation
- Initial and boundary conditions for `U`, `p`, `phiE`, and `rho`

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

The `0/` directory should contain at least the following fields:

- `U`
- `p`
- `phiE`
- `rho`

The solver calculates and writes `E` and `phiEF`. These fields can be read if
the files already exist, but they are normally calculated from `phiE` during the
solution.

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

## Important Mesh Notes

Electric-field calculations in OpenFOAM are strongly affected by the mesh
topology. Around circular electrodes such as wires, an O-grid mesh should be
used so that the mesh follows the electrode shape.

If a circular electrode is meshed with a Cartesian-style mesh generator such as
`cartesianMesh`, the electric field and space charge distribution may become
non-physical, or the calculation may diverge.

## When `rho` Oscillates

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
- Physically appropriate boundary conditions must be supplied for `phiE` and
  `rho`.

## License

This solver is derived from OpenFOAM solver code. The source files retain the
OpenFOAM GPL license headers.
