// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BipolarIons_H
#define BipolarIons_H

#include "fvCFD.H"

namespace Foam
{

class BipolarIons
{
    const dimensionedScalar muP_;
    const dimensionedScalar muN_;
    const dimensionedScalar DP_;
    const dimensionedScalar DN_;
    const dimensionedScalar beta_;

public:

    volScalarField nP;
    volScalarField nN;

    BipolarIons(const fvMesh& mesh, const dictionary& properties)
    :
        muP_("muP", dimensionSet(-1, 0, 2, 0, 0, 1, 0), properties),
        muN_("muN", muP_.dimensions(), properties),
        DP_("DP", dimViscosity, properties),
        DN_("DN", dimViscosity, properties),
        beta_("beta", dimVolume/dimTime, properties),
        nP
        (
            IOobject
            (
                "nP", mesh.time().timeName(), mesh,
                IOobject::MUST_READ, IOobject::AUTO_WRITE
            ),
            mesh
        ),
        nN
        (
            IOobject
            (
                "nN", mesh.time().timeName(), mesh,
                IOobject::MUST_READ, IOobject::AUTO_WRITE
            ),
            mesh
        )
    {
        if
        (
            muP_.value() <= 0 || muN_.value() <= 0
         || DP_.value() <= 0 || DN_.value() <= 0 || beta_.value() < 0
        )
        {
            FatalIOErrorInFunction(properties)
                << "muP, muN, DP and DN must be positive, and beta must "
                << "be non-negative." << exit(FatalIOError);
        }

        if (nP.dimensions() != dimless/dimVolume || nN.dimensions() != dimless/dimVolume)
        {
            FatalErrorInFunction
                << "nP and nN must be ion number densities in m^-3."
                << exit(FatalError);
        }
    }

    void solve
    (
        const surfaceScalarField& phi,
        const surfaceScalarField& phiEF
    )
    {
        // Upwind the total face flux in the direction of each species' motion.
        const surfaceScalarField phiP("phiP", phi + muP_*phiEF);
        const surfaceScalarField phiN("phiN", phi - muN_*phiEF);

        // Treat recombination implicitly in the species being solved.
        fvScalarMatrix nPEqn
        (
            fvm::div(phiP, nP)
          - fvm::laplacian(DP_, nP)
          + fvm::Sp(beta_*nN, nP)
        );
        nPEqn.relax();
        nPEqn.solve();

        fvScalarMatrix nNEqn
        (
            fvm::div(phiN, nN)
          - fvm::laplacian(DN_, nN)
          + fvm::Sp(beta_*nP, nN)
        );
        nNEqn.relax();
        nNEqn.solve();
    }
};

} // End namespace Foam

#endif
