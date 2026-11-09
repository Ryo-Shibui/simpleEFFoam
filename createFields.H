const Switch ionCalculation
(
    simple.dict().lookupOrDefault<Switch>("ionCalculation", false)
);

Info<< "Reading physicalProperties\n" << endl;

IOdictionary physicalProperties
(
    IOobject
    (
        "physicalProperties",
        runTime.constant(),
        mesh,
        IOobject::MUST_READ_IF_MODIFIED,
        IOobject::NO_WRITE
    )
);

dimensionedScalar epsilon0
(
    "epsilon0",
    dimensionSet(-1, -3, 4, 0, 0, 2, 0),
    physicalProperties
);

dimensionedScalar k
(
    ionCalculation
  ? dimensionedScalar("k", dimensionSet(-1, 0, 2, 0, 0, 1, 0), 0)
  : dimensionedScalar("k", dimensionSet(-1, 0, 2, 0, 0, 1, 0), physicalProperties)
);

dimensionedScalar T
(
    "T",
    dimensionSet(0, 0, 0, 1, 0, 0, 0),
    physicalProperties
);

dimensionedScalar rho0
(
    "rho0",
    dimensionSet(1, -3, 0, 0, 0, 0, 0),
    physicalProperties
);

dimensionedScalar nu
(
    "nu",
    dimensionSet(0, 2, -1, 0, 0, 0, 0),
    physicalProperties
);


Info<< "Reading field p\n" << endl;
volScalarField p
(
    IOobject
    (
        "p",
        runTime.timeName(),
        mesh,
        IOobject::MUST_READ,
        IOobject::AUTO_WRITE
    ),
    mesh
);

Info<< "Reading field U\n" << endl;
volVectorField U
(
    IOobject
    (
        "U",
        runTime.timeName(),
        mesh,
        IOobject::MUST_READ,
        IOobject::AUTO_WRITE
    ),
    mesh
);

#include "createPhi.H"


Info<< "Reading field phiE\n" << endl;
volScalarField phiE
(
    IOobject
    (
        "phiE",
        runTime.timeName(),
        mesh,
        IOobject::MUST_READ,
        IOobject::AUTO_WRITE
    ),
    mesh
);

Info<< "Reading/calculating field E (electric field)\n" << endl;
volVectorField E
(
    IOobject
    (
        "E",
        runTime.timeName(),
        mesh,
        IOobject::READ_IF_PRESENT,
        IOobject::AUTO_WRITE
    ),
    -fvc::grad(phiE)
);


autoPtr<BipolarIons> ions;
if (ionCalculation)
{
    Info<< "Reading positive and negative ion number densities nP and nN\n" << endl;
    ions.reset(new BipolarIons(mesh, physicalProperties));
}
else
{
    Info<< "Reading field rho\n" << endl;
}

const IOobject rhoIO
(
    "rho",
    runTime.timeName(),
    mesh,
    ionCalculation ? IOobject::NO_READ : IOobject::MUST_READ,
    IOobject::AUTO_WRITE
);
autoPtr<volScalarField> rhoStorage
(
    ionCalculation
  ? new volScalarField(rhoIO, pc::e*(ions->nP - ions->nN))
  : new volScalarField(rhoIO, mesh)
);


Info<< "Reading/calculating face flux field of electric field phiEF\n" << endl; 
surfaceScalarField phiEF
(
    IOobject
    (
       "phiEF",
        runTime.timeName(),
        mesh,
        ionCalculation ? IOobject::NO_READ : IOobject::READ_IF_PRESENT,
        IOobject::AUTO_WRITE
    ),
 	fvc::flux(E)
);


label pRefCell = 0;
scalar pRefValue = 0.0;
setRefCell(p, simple.dict(), pRefCell, pRefValue);
mesh.setFluxRequired(p.name());


singlePhaseTransportModel laminarTransport(U, phi);

autoPtr<incompressible::turbulenceModel> turbulence
(
    incompressible::turbulenceModel::New(U, phi, laminarTransport)
);

#include "createMRF.H"
#include "createFvOptions.H"
