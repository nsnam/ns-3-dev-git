.. include:: replace.txt
.. highlight:: cpp
.. highlight:: bash

International System of Units
-----------------------------

Motivation
**********
In 1998, NASA's Mars Climate Orbiter crashed because NASA used the metric system while spacecraft
builder Lockheed Martin used US customary units. This USD 328 million project failure exemplifies
the catastrophic consequences of unit mismatch. Similar failures have occurred across government
agencies and industries, underscoring the critical importance of unit coherence.

Coherent use of units — dimensions, quantities, and scales — is imperative for correct physics equations,
robust software APIs, and trustworthy simulation results. Yet, these errors remain pervasive and often
difficult to detect through manual inspection. Common unit-related errors include:

* Adding incompatible units (e.g., dBm_t and mWatt_t)
* Performing invalid operations (e.g., direct addition of dB values)
* Treating units at different scales as equivalent (e.g., MHz as Hz)
* Applying operations to logarithmic units as if they were linear
* Mixing linear scale variables and logarithmic scale variables without proper conversion

`Issue 649 <https://gitlab.com/nsnam/ns-3-dev/-/issues/649>`_ represents one such example that was
detected and fixed in |ns3|, but many similar issues may remain undetected without systematic safeguards.


Background
**********
The International System of Units (SI Units) provides the only internationally recognized system of coherent
measurements. While |ns3| has conceptually adopted SI units, enforcing and guaranteeing API coherence
has remained challenging. Documentation and human vigilance alone are insufficient safeguards against
unit-related errors. The ``si-units`` module addresses this fundamental limitation by introducing
compile-time guarantees for unit coherence at the API level.


Design Philosophy
****************
The ``si-units`` module implements a type-safe representation of physical quantities that enforces
correct operations between compatible units while preventing invalid operations between incompatible units.
The implementation follows several key design principles:

**Type Safety and Correctness**

The module prioritizes compile-time validation of unit operations over runtime performance, though
performance considerations are not neglected. Strong typing ensures that operations between incompatible
units (such as adding frequency to power) result in compilation errors rather than runtime errors or
silent incorrect behavior.

**Standards Compliance**

The implementation follows the "National Institute of Standards and Technology's Guide for the Use of the
International System of Units" while pragmatically accommodating the needs of network simulation. The module
does not strictly distinguish between the idiosyncrasies of NIST, ISO, or IEC standards, but adheres
primarily to `National institute of Standards and Technology's Guide for the Use of the International System of Units <https://physics.nist.gov/cuu/pdf/sp811.pdf>`.

**Notation and Style Guides**

SI rules, a global standard, takes precedence over |ns3|'s coding style or its idiomatic usages.

  ==================== ================ ==========================================
  Unit                 Ok               Not Ok
  ==================== ================ ==========================================
  decibel              dB               DB, Db, db
  decibel-milliwatts   dBm, dBmW        DBM, DBm, DbM, Dbm, dBM, dbM, dbm
  milli                m                M
  mega                 M                m
  milligram            mg, milligram    milli-gram mill_gram, MilliGram, milliGram
  ==================== ================ ==========================================

**Selective Implementation**

Rather than implementing all possible SI units, the module focuses on units most relevant to network
simulation, particularly those used in wireless communications. This includes:

* Energy and power units (dB_t, dBm_t, mWatt_t, Watt_t)
* Frequency units (Hz_t, kHz_t, MHz_t, GHz_t, THz_t)
* Time units (nSEC_t with support for nanoseconds to seconds)
* Angle units (degree_t, radian_t)
* Ratio units (percent_t)
* Spectral density units (dBm_per_Hz_t, dBm_per_MHz_t)

**Intuitive Syntax**

The module provides user-defined literals for natural expression of physical quantities:

::

  auto frequency = 2.4_GHz;
  auto power = 20_dBm;
  auto angle = 90_degree;
  auto duration = 100_nSEC;
  auto percentage = 50_percent;

These literals make the code more readable and self-documenting while maintaining type safety.

**Seamless Conversion**

Each unit type provides methods for conversion to related units:

::

  // Value accessors (return primitive type)
  double value_in_mw = power.in_mWatt();
  double value_in_dbm = power.in_dBm();

  // Type conversion (return strong type)
  dBm_t power_dbm = power.to_dBm();
  mWatt_t power_mw = power.to_mWatt();

The naming convention consistently uses:
* ``in_X()`` methods return the value as a primitive type (double/int)
* ``to_X()`` methods return a new object of the target unit type

**Operator Overloading**

The module carefully defines valid operations between units, allowing intuitive expressions while
preventing physically meaningless operations:

::

  // Valid operations
  auto sum = 10_dBm + 3_dB;            // Adding dB to dBm is valid
  auto product = 2.0 * 10_mWatt;       // Scaling by a unitless value

  // Invalid operations (will not compile)
  auto invalid = 10_dBm + 20_mWatt;    // Error: incompatible units
  auto nonsense = 10_MHz + 20_dBm;     // Error: incompatible dimensions

**Attribute System Integration**

The module integrates with the |ns3| attribute system, allowing SI units to be used as attributes
in simulation objects:

::

  .AddAttribute("TxPower",
                "Transmission power",
                dBmValue(20_dBm),
                MakedBmAccessor(&WifiPhy::m_txPower),
                MakedBmChecker())

Furthermore, this makes possible coherent implementations from configuration files to command-line arguments,
and to attributes.

**Extendability**

The implementation of select SI units exhibits a pattern that can be used to introduce new strong types,
including other than SI units, eg. `Imperial Units <https://en.wikipedia.org/wiki/Imperial_units>` such as
miles, hour, or knots, as long as doing so promotes convergence to `SI Units` and the rigor of the
engineering.


Implementation Approach
**********************
The ``si-units`` module implements strong typing through C++ structures that encapsulate the underlying
value while providing type-safe operations. Each unit type follows a consistent pattern:

1. A structure with an underlying value member (typically ``double`` or ``int64_t``)
2. Constructors for various input types (numeric literals, strings)
3. Conversion methods to related units
4. Overloaded operators for valid operations
5. String representation methods
6. Integration with the |ns3| attribute system

For example, the ``dBm_t`` structure encapsulates power in dBm, providing:

* Conversion to/from linear power units (mWatt_t, Watt_t)
* Addition/subtraction with dB_t (representing relative power, such as power gain or loss)
* Prevention of direct addition/subtraction with other dBm_t values
* String representation for debugging and logging

The module carefully considers the appropriate underlying type for each unit. While some units
could theoretically use integral types (e.g., Hz_t could use int64_t for frequencies above 1 Hz),
the current implementation primarily uses ``double`` to facilitate migration from legacy APIs.

Evolution of Unit Coherence Patterns
***********************************
There are couple of popular patterns with varying degree of robustness. The evolution of unit
handling in code typically follows these patterns, from least to most robust. Below examples
use ``double``, but it could be ``int32_t`` or any useful choice.

**A. Primitive Types with Comments**

::

   double power1;                                   // mWatt. This comment is necessary
   double power2;                                   // dBm.   This comment is necessary
   double result = power1 + FromDbmToMWatt(power2); // mWatt. Note the use of conversion
   double result = FromMWattToDbm(power1) + power2; // dBm.   Note the use of conversion
   double result = power1 + power2;                 // Compiles but physically incorrect!

**B. Naming Conventions**

::

   double powerMWatt;                                          // Unit indicated in name
   double powerDbm;                                            // Unit indicated in name
   double resultMWatt = powerMWatt + FromDbmToMWatt(powerDbm); // Conversion still needed
   double resultDbm   = FromMWattToDbm(powerMWatt) + powerDbm; // Conversion still needed
   double result      = powerMWatt + powerDbm;                 // Compiles but physically incorrect!

**C. Weak Type Aliases**

::

   using mWatt_t = double;                         // Type alias improves readability
   using dBm_u   = double;                         // But doesn't enforce type safety
   mWatt_t power1;
   dBm_u   power2;
   mWatt_t result = power1 + FromDbmToMWatt(power2); // Conversion still needed
   dBm_u   result = FromMWattToDbm(power1) + power2; // Conversion still needed
   auto    result = power1 + power2;                 // Compiles but physically incorrect!

**D. Strong Types (Implemented in this module)**

::

   struct mWatt_t{..};                             // Full encapsulation with operators
   struct dBm_t{..};
   mWatt_t  power1{100.0};
   dBm_t    power2{20.0};

   // Type-safe operations with automatic conversion where appropriate
   mWatt_t result1 = power1 + power2.to_mWatt();   // Explicit conversion
   dBm_t   result2 = power2 + dB_t{3.0};           // Valid: dBm + dB = dBm

   // This would cause a compilation error:
   // auto invalid = power1 + power2;              // Error: no operator+ between mWatt_t and dBm_t

The strong typing approach (D) provides compile-time guarantees against unit errors, eliminating an entire
class of potential bugs while making the code more readable and self-documenting.

Migration Strategy
*****************
While newly written code may adopt ``si-units`` from its inception, bringing its benefits to existing
code is certainly impactful.

A key is gradual, incremental, and testable refactoring. The outcome of the migration shall not
change the behaviors. If the migration efforts discover incumbent bugs, bug fixes should be handled
separately from API migration; Guaranteeing no behavioral changes during refactoring is as important
as fixing bugs.

::
  // Before: Legacy API using primitive types
  double SomeLegacyApi(double freq);

  // After: Modern code using strong types
  dBm_t SomeLegacyApi(MHz_t freq);

**Wrapper Approach**

Preserve legacy APIs while providing strongly-typed alternatives:

::

  // Legacy API (preserved)
  double GetTxPower();

  // New strongly-typed wrapper
  dBm_t GetTxPowerDbm()
  {
      return dBm_t{GetTxPower()};
  }

**Two-Phase Migration**

If directly adopting strong-typed units is prohibitive, a two-phase approach may be more manageable.

1. **Phase 1**: Introduce weak type aliases to improve readability without changing function signatures:

   ::

     // Before
     double CalculatePower(double frequency, double distance);

     // Phase 1
     using MHz_u = double;
     using meter_u = double;
     using dBm_u = double;

     dBm_u CalculatePower(MHz_u frequency, meter_u distance);

2. **Phase 2**: Replace weak aliases with strong types, updating function implementations as needed:

   ::

     // Phase 2
     dBm_t CalculatePower(MHz_t frequency, meter_t distance);

This approach allows gradual migration while maintaining backward compatibility.

Implementation Details
********************
The ``si-units`` module makes several important implementation choices:

**Storage Types**

Most unit types currently use ``double`` as the underlying storage type, though some time-related
units use ``int64_t``. This choice balances precision requirements with compatibility concerns:

* Energy/power units (dBm_t, mWatt_t): ``double`` for wide dynamic range
* Frequency units (Hz_t): ``double`` currently, with potential future migration to ``int64_t``
* Time units (nSEC_t): ``int64_t`` for precise representation of discrete time steps
* Angle units (degree_t, radian_t): ``double`` for fractional angles

**Operator Constraints**

Operations between units are carefully constrained to prevent physically meaningless combinations:

* Addition/subtraction: Only allowed between compatible units (e.g., MHz_t + MHz_t)
* Multiplication/division by scalars: Generally allowed (e.g., 2.0 * 10_mWatt)
* Multiplication/division between units: Selectively implemented where physically meaningful
* Special handling for logarithmic units: dB_t can be added to dBm_t, but dBm_t cannot be directly added to another dBm_t

**String Representation**

Each unit type provides string representation methods that follow SI conventions:

::

  auto power = 20_dBm;
  std::cout << power.str();      // "20.0 dBm"
  std::cout << power.str(false); // "20.0dBm" (without space)

**Vector Operations**

For operations on collections of values, the module provides utility methods:

::

  std::vector<double> values = {1.0, 2.0, 3.0};
  auto powers = dBm_t::from_doubles(values);  // Convert to vector of dBm_t
  auto raw_values = dBm_t::to_doubles(powers); // Convert back to vector of double

Validation
**********
The ``si-units`` module includes comprehensive unit tests that verify:

* Correct behavior of individual unit types
* Valid operations between compatible units
* Proper conversion between related units
* Integration with the |ns3| attribute system
* String parsing and formatting
* Vector operations

These tests can be run using:

::

  $ ./test.py -s si-units-test

or in DEBUG build profile:

::

  $ build/utils/ns3-dev-test-runner-default --verbose --test-name=si-units-test --stop-on-failure --fullness=QUICK

Future Directions
***************
The ``si-units`` module could be extended in several ways:

* Additional unit types (length, mass, etc.) as needed for simulation scenarios
* Compound units (e.g., acceleration as meters per second squared)
* Optimization of storage types for specific use cases
* Enhanced integration with visualization and analysis tools

Conclusion
*********
The ``si-units`` module provides a robust foundation for handling physical quantities in |ns3|
simulations. By enforcing unit coherence at compile time, it eliminates an entire class of potential
errors while making code more readable and self-documenting. The module's design balances theoretical
correctness with practical considerations, providing a path for gradual migration of existing code
while enabling new code to benefit from strong typing immediately.
